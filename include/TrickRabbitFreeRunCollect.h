#pragma once

#include "Game/Screen/BombTimerLayout.h"
#include "TrickRabbitFreeRun.h"
#include "syati.h"


class TrickRabbitFreeRunCollect : public LiveActor {
public:
  TrickRabbitFreeRunCollect(const char *pName);
  virtual void init(const JMapInfoIter &rIter);

  void initRabbits(const JMapInfoIter &rIter);

  void exeWait();
  void exeOnGoing();

  TPos3f mBaseMtx;
  s32 mRabbitNum;
  TrickRabbitFreeRun **mRabbit;
};
