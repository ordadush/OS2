#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NPROC      15
#define GROUPS     4
#define FAVORITISM 100 

int
main(void)
{
  printf("=== Israeli Lock Test ===\n");
  printf("NPROC=%d  GROUPS=%d  FAVORITISM=%d\n\n", NPROC, GROUPS, FAVORITISM);

  int lock_id = israeli_create(FAVORITISM);
  if(lock_id < 0){
    printf("ERROR: israeli_create failed\n");
    exit(1);
  }

  lcg_srand(getpid());

  for(int i = 0; i < NPROC; i++){
    int pid = fork();
    if(pid == 0){
      int gid = lcg_rand() % GROUPS;
      setgid(gid);
      israeli_acquire(lock_id);
      printf("Process %d (gid=%d) acquired the lock\n", getpid(), getgid());
      sleep(10);
      israeli_release(lock_id);
      exit(0);
    }
  }

  for(int i = 0; i < NPROC; i++)
    wait(0);

  israeli_destroy(lock_id);
  printf("\n=== Done ===\n");
  exit(0);
}
