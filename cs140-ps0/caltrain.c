#include "pintos_thread.h"

struct station {
  int waitman;//
  int emptyseat;
  int toseatman;// the man on board but not seated
  struct lock lock;
  struct condition hasman;// has man wait train on the station
  struct condition hasseat;// the train on this station has seat for waitman
  struct condition readygo;//all man on board, so the train can start
};

void
station_init(struct station *station)
{
  lock_init(&(station->lock));
  cond_init(&(station->hasman));
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
      // printf("train broadcast count= %d\n",count);
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
  // printf("a man enter station, waitman = %d\n",station->waitman);
  while(station->emptyseat==0){
    cond_wait(&(station->hasseat),&(station->lock));
  }
  station->waitman-=1;
  station->emptyseat-=1;
  station->toseatman+=1;
  // printf("a man has a seat, waitman= %d\n",station->waitman);
  lock_release(&(station->lock));
}

void
station_on_board(struct station *station)
{
  lock_acquire(&(station->lock));
  station->toseatman-=1;
  // printf("a man seat down\n");
  if((station->toseatman==0)&&((station->waitman==0)||(station->emptyseat==0))){
    cond_signal(&(station->readygo),&(station->lock));
  }
  lock_release(&(station->lock));
}

