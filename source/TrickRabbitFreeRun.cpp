#include "TrickRabbitFreeRun.h"
#include "Game/Boss/DinoPackun.h"
#include "Game/LiveActor/ActorStateParamScale.h"
#include "Game/NPC/NPCActor.h"
#include "Game/Player/YoshiLockOnTarget.h"
#include "Game/Util/ActionSoundUtil.h"
#include "Game/Util/ActorAnimUtil.h"
#include "Game/Util/ActorMovementUtil.h"
#include "Game/Util/ActorSensorUtil.h"
#include "Game/Util/ActorShadowLocalUtil.h"
#include "Game/Util/ActorSwitchUtil.h"
#include "Game/Util/BgmUtil.h"
#include "Game/Util/DemoUtil.h"
#include "Game/Util/JMapInfo.h"
#include "Game/Util/JMapUtil.h"
#include "Game/Util/LiveActorUtil.h"
#include "Game/Util/MtxUtil.h"
#include "Game/Util/ObjUtil.h"
#include "Game/Util/SceneUtil.h"
#include "Game/Util/TalkUtil.h"
#include "JSystem/JGeometry/TMatrix.h"
#include "JSystem/JGeometry/TVec.h"
#include "RabbitStateCaught.h"
#include "RabbitStateWaitStart.h"
#include "TrickRabbitFreeRunCollect.h"
#include "WalkerStateBlowDamage.h"
#include "WalkerStateRunaway.h"
#include "revolution/os.h"
#include "revolution/types.h"


namespace {
static MR::ActorMoveParam sStartAwayParam = {1.4f, 1.0f, 0.9f, 4.0f};
static const f32 sBodyRadius = 70.0f;
static const f32 sCatchRadius = 30.0f;
static const f32 sBindRadius = 60.0f;
static const f32 sBindYOffset = 60.0f;
static const f32 sUpVecBlendRate = 0.1f;
static const f32 sFrontVecBlendRate = 0.2f;
// static const f32 sStarPieceHitAccelH = _;
// static const f32 sStarPieceHitAccelV = _;
static const f32 sPressureRange = 600.0f;
static const f32 sNormalAccel = 0.7f;
static const f32 sPressureAccel = 1.5f;
static const f32 sBrakeAccelRate = 0.3f;
}; // namespace

namespace NrvTrickRabbitFreeRun {
FULL_NERVE(TrickRabbitFreeRunNrvVanish, TrickRabbitFreeRun, Vanish);
FULL_NERVE(TrickRabbitFreeRunNrvHide, TrickRabbitFreeRun, Hide);
FULL_NERVE(TrickRabbitFreeRunNrvAppear, TrickRabbitFreeRun, Appear);
FULL_NERVE(TrickRabbitFreeRunNrvWaitStart, TrickRabbitFreeRun, WaitStart);
FULL_NERVE(TrickRabbitFreeRunNrvTryDemo, TrickRabbitFreeRun, TryDemo);
FULL_NERVE(TrickRabbitFreeRunNrvRunawayStart, TrickRabbitFreeRun, RunawayStart);
FULL_NERVE(TrickRabbitFreeRunNrvRunaway, TrickRabbitFreeRun, Runaway);
FULL_NERVE(TrickRabbitFreeRunNrvBlowDamage, TrickRabbitFreeRun, BlowDamage);
FULL_NERVE(TrickRabbitFreeRunNrvCaught, TrickRabbitFreeRun, Caught);
FULL_NERVE(TrickRabbitFreeRunNrvPowerStarDemo, TrickRabbitFreeRun,
           PowerStarDemo);
FULL_NERVE(TrickRabbitFreeRunNrvGiveUp, TrickRabbitFreeRun, GiveUp);

NERVE(TrickRabbitFreeRunNrvTakeOutStar);

void TrickRabbitFreeRunNrvTakeOutStar::execute(Spine *pSpine) const {}

TrickRabbitFreeRunNrvTakeOutStar(TrickRabbitFreeRunNrvTakeOutStar::sInstance);
}; // namespace NrvTrickRabbitFreeRun

TrickRabbitFreeRun::TrickRabbitFreeRun(const char *pName)
    : NPCActor(pName), mRotateQuat(0.0f, 0.0f, 0.0f, 1.0f),
      mFrontVec(0.0f, 0.0f, 1.0f), mStateWaitStart(), mStateRunaway(),
      mStateBlowDamage(), mStateCaught(), mMsgCtrl(), mSpotMarkLight() {}

void TrickRabbitFreeRun::init(const JMapInfoIter &rIter) {
  MR::initDefaultPos(this, rIter);
  MR::getJMapInfoArg0WithInit(rIter, &mStarId);
  mColorFrame = 0;
  MR::getJMapInfoArg1WithInit(rIter, &mColorFrame);
  MR::getJMapInfoArg2WithInit(rIter, &mNearSoundRadius);
  MR::getJMapInfoArg3WithInit(rIter, &mUseProvoke);

  s32 cameraRegisterVec = -1;
  MR::getJMapInfoArg7WithInit(rIter, &cameraRegisterVec);

  if (cameraRegisterVec != -1) {
    MR::declareCameraRegisterVec(this, cameraRegisterVec, &mTranslation);
  }

  MR::initUseStageSwitchReadB(this, rIter);

  MR::registerDemoSimpleCastAll(this);

  MR::tryRegisterNamePosLinkObj(this, rIter, "隠れ位置");
  mUsesVanish = MR::useStageSwitchReadAppear(this, rIter);

  MR::getParamScale(rIter, &mParamScale);

  initModelManagerWithAnm("TrickRabbit", nullptr, nullptr, false);
  MR::startBtpAndSetFrameAndStop(this, "TrickRabbit", mColorFrame);
  MR::connectToSceneNpc(this);
  MR::invalidateClipping(this);
  MR::initLightCtrl(this);
  MR::makeQuatAndFrontFromRotate(&mRotateQuat, &mFrontVec, this);

  initSensor();
  initBinder(::sBindRadius, ::sBindYOffset, 0);
  MR::onCalcGravity(this);

  ActorShadow::initShadowFromCSV(this, "Shadow", true);

  mSpotMarkLight = new SpotMarkLight(this, 100.0f, 1500.0f, nullptr);
  mSpotMarkLight->initWithoutIter();

  s32 genID;
  MR::getGeneratorID(rIter, &genID);

  mIsChildObj = false;
  if (genID > -1)
    mIsChildObj = true;

  initEffectKeeper(0, nullptr, 0);
  MR::onEmitEffectSyncClipping(this, "Light");
  MR::onForceDeleteEffectSyncClipping(this, "Light");

  if (mStarId > 0) {
    if (!mIsChildObj) {
      MR::declarePowerStar(this);

      mTakeOutStar = new TakeOutStar(
          this, "TakeOutStarTrickRabbit", "TakeOutStarTrickRabbit",
          &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvTakeOutStar::sInstance,
          0);
    } else {
      MR::declareStarPiece(this, mStarId);
    }
  }

  mMsgCtrl = MR::createTalkCtrl(this, rIter, "TrickRabbit",
                                TVec3f(0.0f, 120.0f, 0.0f), nullptr);

  MR::offRootNodeAutomatic(mMsgCtrl);

  initState();

  initSound(4, "TrickRabbitFreeRun", &mTranslation, TVec3f(0.0f, 0.0f, 0.0f));

  initNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvWaitStart::sInstance,
            0);

  makeActorAppeared();
}

void TrickRabbitFreeRun::initAfterPlacement() {
  MR::trySetMoveLimitCollision(this);
}

void TrickRabbitFreeRun::initSensor() {
  initHitSensor(2);
  MR::addHitSensorAtJointEnemy(this, "Body", "Spine", 8, ::sBodyRadius,
                               TVec3f(0.0f, 0.0f, 0.0f));
  MR::addHitSensorAtJointEnemy(this, "Catch", "Spine", 8, ::sCatchRadius,
                               TVec3f(0.0f, 0.0f, 0.0f));
  MR::initStarPointerTargetAtJoint(this, "Spine", ::sBodyRadius, TVec3f(0.0f));
}

void TrickRabbitFreeRun::initState() {
  mStateCaught = new RabbitStateCaught(this, mMsgCtrl);
  if (mStarId <= 0 || mIsChildObj)
    mStateCaught->unusePowerStarModel();
  mStateCaught->init();

  mStateWaitStart = new RabbitStateWaitStart(this, &mFrontVec, mMsgCtrl);
  mStateWaitStart->init();

  mStateRunaway = new WalkerStateRunaway(this, &mFrontVec, nullptr);
  mStateRunaway->init();

  mStateBlowDamage = new WalkerStateBlowDamage(this, &mFrontVec, nullptr);
  mStateBlowDamage->init();
}

void TrickRabbitFreeRun::control() {
  MR::blendQuatUpFront(&mRotateQuat, -mGravity, mFrontVec, ::sUpVecBlendRate,
                       ::sFrontVecBlendRate);

  if (MR::isValidSwitchParam(this)) {
    if (MR::isOnSwitchParam(this)) {
      MR::setBckRate(this, mParamScale);
    } else {
      MR::setBckRate(this, 1.0f);
    }
  }

  if (MR::isValidSwitchB(this)) {
    if (isNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvWaitStart::
                    sInstance)) {
      if (MR::isOnSwitchB(this)) {
        setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvRunawayStart::
                     sInstance);
      }
    }
  }
}

void TrickRabbitFreeRun::calcAndSetBaseMtx() {
  MR::setBaseTRMtx(this, mRotateQuat);
}

void TrickRabbitFreeRun::attackSensor(HitSensor *pSender,
                                      HitSensor *pReceiver) {
  if (!MR::isSensorPlayer(pReceiver)) {
    return;
  }

  if (pSender == getSensor("Catch") && requestCaught()) {
    return;
  }

  if (isEnableCaught()) {
    return;
  }

  MR::sendMsgPush(pReceiver, pSender);
}

bool TrickRabbitFreeRun::receiveMsgPush(HitSensor *pSender,
                                        HitSensor *pReceiver) {
  if (MR::isSensorPlayer(pSender)) {
    return !isEnableCaught();
  }

  return false;
}

bool TrickRabbitFreeRun::receiveMsgEnemyAttack(u32 msg, HitSensor *pSender,
                                               HitSensor *pReceiver) {
  if (MR::isMsgToEnemyAttackBlow(msg)) {
    return receiveMsgBlowDamage(pSender, pReceiver);
  }

  return false;
}

bool TrickRabbitFreeRun::receiveMsgPlayerAttack(u32 msg, HitSensor *pSender,
                                                HitSensor *pReceiver) {
  if (isNerve(
          &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvWaitStart::sInstance) ||
      isNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvGiveUp::sInstance)) {
    return mStateWaitStart->receiveMsgPlayerAttack(msg, pSender, pReceiver);
  }

  if (MR::isMsgStarPieceAttack(msg)) {
    if (receiveMsgBlowDamage(pSender, pReceiver)) {
      return true;
    }
  }

  if (MR::isMsgPlayerSpinAttack(msg)) {
    return requestCaught();
  }

  return false;
}

bool TrickRabbitFreeRun::receiveOtherMsg(u32 msg, HitSensor *pSender,
                                         HitSensor *pReceiver) {
  if (isNerve(
          &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvWaitStart::sInstance) ||
      isNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvGiveUp::sInstance)) {
    return mStateWaitStart->receiveOtherMsg(msg, pSender, pReceiver);
  }

  return false;
}

bool TrickRabbitFreeRun::receiveMsgBlowDamage(HitSensor *pSender,
                                              HitSensor *pReceiver) {
  if (isEnableBlowDamage()) {
    MR::setVelocitySeparateHV(this, pSender, pReceiver, 20.0f, 20.0f);
    setNerve(
        &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvBlowDamage::sInstance);

    return true;
  }

  return false;
}

bool TrickRabbitFreeRun::requestCaught() {
  if (isEnableCaught()) {
    MR::invalidateClipping(this);
    // [NOTE] RequestCaught node forward
    MR::forwardNode(mMsgCtrl);
    setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvCaught::sInstance);

    return true;
  }

  return false;
}

void TrickRabbitFreeRun::exeWaitStart() {
  if (MR::isFirstStep(this)) {
    mStateWaitStart->setTalkActionName("StartEvent");
    mStateWaitStart->_1C = true;
  }

  if (MR::updateActorState(this, mStateWaitStart)) {
    MR::requestStartDemoMarioPuppetable(
        this, "逃走",
        &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvRunawayStart::sInstance,
        &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvTryDemo::sInstance);
  }
}

void TrickRabbitFreeRun::exeTryDemo() {}

void TrickRabbitFreeRun::exeRunawayStart() {
  if (MR::isFirstStep(this)) {
    if (MR::isValidSwitchB(this) && !MR::isOnSwitchB(this)) {
      MR::onSwitchB(this);
    }

    MR::startBckPlayer("BattleWait", nullptr);
    MR::startAction(this, "Run");
    if (!mIsChildObj) {
      MR::startMinigameBGM();
    }
    // [NOTE] RunawayStart node forward
    MR::forwardNode(mMsgCtrl);
  }

  MR::escapeFromPlayer(this, &mFrontVec, ::sStartAwayParam._0,
                       ::sStartAwayParam._4, ::sStartAwayParam._8,
                       ::sStartAwayParam._C);

  if (MR::checkPassBckFrame(this, 3.0f)) {
    startJumpSound();
  }

  if (MR::isGreaterStep(this, 120)) {
    MR::endDemo(this, "逃走");

    if (mUsesVanish) {
      setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvVanish::sInstance);
    } else {
      setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvRunaway::sInstance);
    }
  }
}

void TrickRabbitFreeRun::exeRunaway() {
  MR::updateActorState(this, mStateRunaway);

  f32 distToPlayer = MR::calcDistanceToPlayer(this);

  if (distToPlayer < 10000.0f && distToPlayer > 1300.0f && mUseProvoke) {
    // This message has to be a "Shout" type message!
    MR::tryTalkForce(mMsgCtrl);
    turnToPlayer();
  }

  if (mStateRunaway->isRunning() && MR::checkPassBckFrame(this, 3.0f)) {
    startJumpSound();
  }

  f32 speed = ::sNormalAccel;

  if (MR::isNearPlayerHorizontal(this, ::sPressureRange)) {
    speed = ::sPressureAccel;
  }

  if (MR::isBindedGroundWater(this) || MR::isBindedGroundBrake(this)) {
    speed *= ::sBrakeAccelRate;
  }

  if (MR::isValidSwitchParam(this)) {
    if (MR::isOnSwitchParam(this)) {
      speed *= mParamScale;
    }
  }

  mStateRunaway->mRunawaySpeed = speed;
}

void TrickRabbitFreeRun::exeBlowDamage() {
  MR::updateActorStateAndNextNerve(
      this, mStateBlowDamage,
      &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvRunaway::sInstance);
}

void TrickRabbitFreeRun::exeCaught() {
  if (MR::isFirstStep(this)) {
    if (!mIsChildObj) {
      MR::stopStageBGM(60);
    }
    MR::startSystemSE("SE_SY_RUNAWAY_RABBIT_GET_1", -1, -1);
  }

  if (!MR::updateActorState(this, mStateCaught)) {
    return;
  }

  if (mStarId > 0 && !mIsChildObj) {
    setNerve(
        &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvPowerStarDemo::sInstance);
    return;
  }

  MR::endDemo(this, "捕まり");

  if (mStarId > 0) {
    MR::appearStarPiece(this, mTranslation, mStarId, 20.0f, 60.0f, false);
  } else {
    if (!mIsChildObj) {
      MR::startCurrentStageBGM(true);
    }
    // [NOTE] Caught node forward
    MR::forwardNode(mMsgCtrl);
  }

  setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvGiveUp::sInstance);
}

void TrickRabbitFreeRun::exePowerStarDemo() {
  if (MR::isFirstStep(this)) {
    MR::endDemo(this, "捕まり");
    MR::startStarChanceBGM(true);
    if (mStarId > 0)
      MR::requestAppearPowerStar(this, mTranslation - mGravity * 200.0f);
    MR::startAction(this, "Wait");
    mSpotMarkLight->kill();
  }

  MR::zeroVelocity(this);

  if (MR::isEndPowerStarAppearDemo(this)) {
    // [NOTE] PowerStarDemo node forward
    MR::forwardNode(mMsgCtrl);
    setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvGiveUp::sInstance);
  }
}

void TrickRabbitFreeRun::exeGiveUp() {
  if (MR::isFirstStep(this)) {
    mStateWaitStart->setTalkActionName("GiveUpTalk");
    mStateWaitStart->_1C = false;
  }

  MR::updateActorState(this, mStateWaitStart);
}

bool TrickRabbitFreeRun::isEnableCaught() const {
  if (isNerve(
          &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvRunaway::sInstance) ||
      isNerve(
          &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvBlowDamage::sInstance)) {
    return true;
  }

  return false;
}

bool TrickRabbitFreeRun::isEnableBlowDamage() const {
  return isNerve(
      &NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvRunaway::sInstance);
}

void TrickRabbitFreeRun::startJumpSound() {
  if (MR::isBindedGroundWater(this)) {
    MR::startSound(this, "SE_SM_RABBIT_JUMP_WATER");
  } else if (MR::isBindedGroundBrake(this)) {
    MR::startSound(this, "SE_SM_RABBIT_JUMP_WATER");
  } else {
    MR::startSound(this, "SE_SM_RABBIT_JUMP");
  }
}

void TrickRabbitFreeRun::exeVanish() {
  if (MR::isFirstStep(this)) {
    MR::startAction(this, "Change");
    MR::startSound(this, "SE_SM_RABBIT_HIDE");
    MR::zeroVelocity(this);
  }

  if (!MR::isBckStopped(this)) {
    return;
  }

  MR::startSound(this, "SE_SM_METAMORPHOSE_SMOKE");

  MR::stopBck(this);
  MR::zeroVelocity(this);
  MR::hideModelAndOnCalcAnim(this);
  setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvHide::sInstance);
}

void TrickRabbitFreeRun::exeHide() {
  if (MR::isFirstStep(this)) {
    MR::deleteEffectAll(this);
  }

  MtxPtr mtx;

  // [TODO] Does not work
  if (MR::tryFindLinkNamePos(this, "隠れ位置", mtx)) {
    setBaseMtx(mtx);
    MR::makeQuatAndFrontFromRotate(&mRotateQuat, &mFrontVec, this);
    MR::resetPosition(this);
  }

  if (mNearSoundRadius > 0.0f) {
    f32 distToPlayer = MR::calcDistanceToPlayer(this);

    if (distToPlayer <= mNearSoundRadius) {
      s32 volume = 100.0f;
      f32 fadeStartDist = mNearSoundRadius - 250.0f;

      if (distToPlayer >= fadeStartDist) {
        volume = MR::getLinerValueFromMinMax(distToPlayer, fadeStartDist,
                                             mNearSoundRadius, 100.0f, 35.0f);
      }

      MR::startLevelSound(this, "SE_SV_LV_RABBIT_NEAR2", volume, -1);
    }
  }

  MR::zeroVelocity(this);
  MR::turnDirectionFromPlayerDegree(this, &mFrontVec, 15.0f);

  if (MR::isValidSwitchAppear(this) && MR::isOnSwitchAppear(this)) {
    MR::showModel(this);
    setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvAppear::sInstance);
  }
}

void TrickRabbitFreeRun::exeAppear() {
  if (MR::isFirstStep(this)) {
    MR::emitEffect(this, "AppearSmoke");
    MR::startAction(this, "Jump");
    MR::setVelocityJump(this, 25.0f);
    MR::addVelocityMoveToDirection(this, mFrontVec, 20.0f);
    MR::startSound(this, "SE_SM_RABBIT_APPEAR");
    MR::startSystemSE("SE_SM_RUNAWAY_RABBIT_APP_ME", -1, -1);
  }

  MR::addVelocityToGravity(this, 1.0f);
  MR::attenuateVelocity(this, 0.99f);

  if (MR::isGreaterStep(this, 5)) {
    if (MR::isBindedGround(this)) {
      setNerve(&NrvTrickRabbitFreeRun::TrickRabbitFreeRunNrvRunaway::sInstance);
    }
  }
}
