#include "pintos_thread.h"

// Forward declaration. This function is implemented in reaction-runner.c,
// but you needn't care what it does. Just be sure it's called when
// appropriate within reaction_o()/reaction_h().
void make_water();

struct reaction {
  int co;//count the O atom number
  int ch;//count the H atom number
  struct lock lock;
  struct condition ro;//to make water, an O atom ready
  struct condition rh;//to make water, two H atoms ready
};

void
reaction_init(struct reaction *reaction)
{
  reaction->co=0;
  reaction->ch=0;
  lock_init(&(reaction->lock));
  cond_init(&(reaction->ro));
  cond_init(&(reaction->rh));
}

void
reaction_h(struct reaction *reaction)
{
  lock_acquire(&(reaction->lock));
  reaction->ch+=1;
  if(reaction->ch>1){
    cond_signal(&(reaction->rh),&(reaction->lock));
  }
  cond_wait(&(reaction->ro),&(reaction->lock));
  lock_release(&(reaction->lock));
}

void
reaction_o(struct reaction *reaction)
{
  lock_acquire(&(reaction->lock));
  reaction->co+=1;
  while(reaction->ch<2){
    cond_wait(&(reaction->rh),&(reaction->lock));
  }
  make_water();
  cond_signal(&(reaction->ro),&(reaction->lock));
  cond_signal(&(reaction->ro),&(reaction->lock));
  reaction->co-=1;
  reaction->ch-=2;
  lock_release(&(reaction->lock));
}
