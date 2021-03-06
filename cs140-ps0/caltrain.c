#include "pintos_thread.h"

struct station {
  int waitman;//
  int emptyseat;
  int toseatman;// the man on board but not seated
  struct lock lock;
  struct condition hasseat;// the train on this station has seat for waitman
  struct condition readygo;//all man on board, so the train can start
};

void
station_init(struct station *station)
{
  lock_init(&(station->lock));
  cond_init(&(station->hasseat));
  cond_init(&(station->readygo));
  station-> waitman=0;
  station->emptyseat=0;
  station->toseatman=0;
}

void
station_load_train(struct station *station, int count)
{
  lock_acquire(&(station->lock));
  if(count>0){
    station->emptyseat=count;
    if(station->waitman>0){
      cond_broadcast(&(station->hasseat),&(station->lock));
      cond_wait(&(station->readygo),&(station->lock));
    }
    station->emptyseat=0;
  }
  lock_release(&(station->lock));
}

void
station_wait_for_train(struct station *station)
{
  lock_acquire(&(station->lock));
  station->waitman+=1;
  while(station->emptyseat==0){
    cond_wait(&(station->hasseat),&(station->lock));
  }
  station->waitman-=1;
  station->emptyseat-=1;
  station->toseatman+=1;
  lock_release(&(station->lock));
}

void
station_on_board(struct station *station)
{
  lock_acquire(&(station->lock));
  station->toseatman-=1;
  if((station->toseatman==0)&&((station->waitman==0)||(station->emptyseat==0))){
    cond_signal(&(station->readygo),&(station->lock));
  }
  lock_release(&(station->lock));
}

