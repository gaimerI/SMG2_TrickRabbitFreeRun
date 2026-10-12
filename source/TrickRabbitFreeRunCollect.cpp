#include "TrickRabbitFreeRunCollect.h"
#include "Game/LiveActor/LiveActor.h"
#include "Game/Util/ActorInitUtil.h"
#include "Game/Util/ActorSwitchUtil.h"
#include "Game/Util/BgmUtil.h"
#include "Game/Util/JMapUtil.h"
#include "Game/Util/LiveActorUtil.h"
#include "Game/Util/ObjUtil.h"
#include "Game/Util/PlayerUtil.h"
#include "Game/Util/SceneUtil.h"
#include "TrickRabbitFreeRun.h"

namespace NrvTrickRabbitFreeRunCollect {
FULL_NERVE(TrickRabbitFreeRunCollectNrvWait, TrickRabbitFreeRunCollect, Wait);
FULL_NERVE(TrickRabbitFreeRunCollectNrvOnGoing, TrickRabbitFreeRunCollect,
           OnGoing);
}; // namespace NrvTrickRabbitFreeRunCollect

TrickRabbitFreeRunCollect::TrickRabbitFreeRunCollect(const char *pName)
    : LiveActor(pName) {};

void TrickRabbitFreeRunCollect::init(const JMapInfoIter &rIter) {
  MR::initDefaultPos(this, rIter);
  MR::getJMapInfoMatrixFromRT(rIter, &mBaseMtx);
  MR::connectToSceneNpcMovement(this);
  MR::invalidateClipping(this);

  // just use a CometTimerObj lmfao
  // mTimeLimit = 60;
  // MR::getJMapInfoArg0NoInit(rIter, &mTimeLimit);

  // mTimerLayaout = new BombTimerLayout();
  // mTimerLayaout->initWithoutIter();

  MR::initUseStageSwitchReadB(this, rIter);

  MR::registerDemoSimpleCastAll(this);

  initNerve(
      &NrvTrickRabbitFreeRunCollect::TrickRabbitFreeRunCollectNrvWait::sInstance,
      0);

  initRabbits(rIter);
}

void TrickRabbitFreeRunCollect::initRabbits(const JMapInfoIter &rIter) {
  mRabbitNum = MR::getChildObjNum(rIter);
  mRabbit = new TrickRabbitFreeRun *[mRabbitNum];

  for (s32 i = 0; i < mRabbitNum; i++) {
    const char *childObjName;
    MR::getChildObjName(&childObjName, rIter, i);

    mRabbit[i] = new TrickRabbitFreeRun("雪ウサギ");
    mRabbit[i]->mCollectObj = this;
    MR::initChildObj(mRabbit[i], rIter, i);
  }
}

void TrickRabbitFreeRunCollect::exeWait() {
  if (MR::isValidSwitchB(this)) {
    if (MR::isOnSwitchB(this)) {
      setNerve(&NrvTrickRabbitFreeRunCollect::TrickRabbitFreeRunCollectNrvOnGoing::sInstance);
    }
  }
}

void TrickRabbitFreeRunCollect::exeOnGoing() {
  // if (MR::isFirstStep(this)) {
  //   MR::startMinigameBGM();

  //   mTimerLayaout->setTimeLimit(mTimeLimit);
  //   mTimerLayaout->appear();
  // }

  // if (mTimerLayaout->isReadyToTimeUp()) {
  //   MR::forceKillPlayerByGroundRace();
  // }
}
