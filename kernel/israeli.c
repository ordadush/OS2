#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#define NILOCK          15
#define ILOCK_QUEUE_MAX 16

struct israeli_lock {
  int active;                        // 1 if this slot is in use
  int locked;                        // 1 if the lock is held (or being transferred)
  int holder_gid;                    // gid of the current holder
  int favoritism;                    // favoritism coefficient 0-100
  struct proc *queue[ILOCK_QUEUE_MAX]; // FIFO queue of waiting processes
  int q_size;                        // number of processes in the queue
  struct proc *selected;             // process chosen to receive the lock next
  struct spinlock lk;                // protects this struct
};

struct israeli_lock israeli_locks[NILOCK];

void
israeli_locks_init(void)
{
  for(int i = 0; i < NILOCK; i++){
    initlock(&israeli_locks[i].lk, "israeli");
    israeli_locks[i].active   = 0;
    israeli_locks[i].locked   = 0;
    israeli_locks[i].q_size   = 0;
    israeli_locks[i].selected = 0;
  }
}

int
israeli_create(int favoritism)
{
  if(favoritism < 0 || favoritism > 100)
    return -1;

  for(int i = 0; i < NILOCK; i++){
    acquire(&israeli_locks[i].lk);
    if(!israeli_locks[i].active){
      israeli_locks[i].active      = 1;
      israeli_locks[i].locked      = 0;
      israeli_locks[i].holder_gid  = -1;
      israeli_locks[i].favoritism  = favoritism;
      israeli_locks[i].q_size      = 0;
      israeli_locks[i].selected    = 0;
      release(&israeli_locks[i].lk);
      return i;
    }
    release(&israeli_locks[i].lk);
  }
  return -1;
}

int
israeli_acquire(int lock_id)
{
  if(lock_id < 0 || lock_id >= NILOCK)
    return -1;

  struct israeli_lock *il = &israeli_locks[lock_id];

  acquire(&il->lk);

  if(!il->active){
    release(&il->lk);
    return -1;
  }

  // Lock is free — take it immediately without queuing
  if(!il->locked){
    il->locked     = 1;
    il->holder_gid = myproc()->gid;
    release(&il->lk);
    return 0;
  }

  // Lock is held — join the queue
  if(il->q_size >= ILOCK_QUEUE_MAX){
    release(&il->lk);
    return -1;
  }
  il->queue[il->q_size++] = myproc();

  // Sleep until this process is selected as the next owner
  while(il->selected != myproc()){
    sleep(il, &il->lk);
  }

  // Take ownership
  il->locked     = 1;
  il->holder_gid = myproc()->gid;
  il->selected   = 0;

  release(&il->lk);
  return 0;
}

int
israeli_release(int lock_id)
{
  if(lock_id < 0 || lock_id >= NILOCK)
    return -1;

  struct israeli_lock *il = &israeli_locks[lock_id];

  acquire(&il->lk);

  if(!il->active || !il->locked){
    release(&il->lk);
    return -1;
  }

  int G = il->holder_gid;

  if(il->q_size == 0){
    // Nobody waiting — simply release
    il->locked     = 0;
    il->holder_gid = -1;
    release(&il->lk);
    return 0;
  }

  // Find the earliest friend (same gid as current holder) in the queue
  int friend_idx = -1;
  for(int i = 0; i < il->q_size; i++){
    if(il->queue[i]->gid == G){
      friend_idx = i;
      break;
    }
  }

  // Decide which process to hand the lock to
  int chosen_idx;
  if(friend_idx != -1 && (int)(lcg_rand() % 100) < il->favoritism){
    chosen_idx = friend_idx;   // favoritism wins
  } else {
    chosen_idx = 0;            // FIFO head
  }

  struct proc *next = il->queue[chosen_idx];

  // Remove chosen process from the queue
  for(int i = chosen_idx; i < il->q_size - 1; i++)
    il->queue[i] = il->queue[i + 1];
  il->q_size--;

  // Transfer lock to the chosen process (locked stays 1)
  il->selected = next;
  wakeup(il);

  release(&il->lk);
  return 0;
}

int
israeli_destroy(int lock_id)
{
  if(lock_id < 0 || lock_id >= NILOCK)
    return -1;

  struct israeli_lock *il = &israeli_locks[lock_id];

  acquire(&il->lk);

  if(!il->active){
    release(&il->lk);
    return -1;
  }

  il->active   = 0;
  il->locked   = 0;
  il->q_size   = 0;
  il->selected = 0;

  release(&il->lk);
  return 0;
}
