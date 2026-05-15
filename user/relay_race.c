#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NTEAMS           3
#define RUNNERS_PER_TEAM 5
#define TARGET           30

// *** שנה את הערך הזה בין ריצות: 0, 50, 100 ***
#define FAVORITISM       100

int
main(void)
{
  printf("=== Relay Race Tournament ===\n");
  printf("Teams=%d  Runners/team=%d  Target=%d  Favoritism=%d%%\n\n",
         NTEAMS, RUNNERS_PER_TEAM, TARGET, FAVORITISM);

  // Reset the kernel scoreboard for this race
  if(race_reset(NTEAMS, TARGET) < 0){
    printf("ERROR: race_reset failed\n");
    exit(1);
  }

  int lock_id = israeli_create(FAVORITISM);
  if(lock_id < 0){
    printf("ERROR: israeli_create failed\n");
    exit(1);
  }

  // Fork NTEAMS * RUNNERS_PER_TEAM child processes
  for(int team = 0; team < NTEAMS; team++){
    for(int r = 0; r < RUNNERS_PER_TEAM; r++){
      int pid = fork();
      if(pid < 0){
        printf("ERROR: fork failed\n");
        exit(1);
      }
      if(pid == 0){
        // Child: this is a runner
        setgid(team);

        // Stagger entry so runners from different teams mix in the queue
        // Use pid directly — each process has a unique pid, no shared state issues
        sleep(getpid() % 8);

        while(1){
          israeli_acquire(lock_id);

          // If another team already won, exit gracefully
          if(race_get_winner() >= 0){
            israeli_release(lock_id);
            exit(0);
          }

          // Add a point and get updated score
          int score = race_add_score(team);
          printf("Runner %d (Team %d) acquired the baton. Team %d score = %d\n",
                 getpid(), team, team, score);

          // Check if this team just won
          if(score >= TARGET){
            printf("\n*** Team %d WINS the race! ***\n\n", team);
            israeli_release(lock_id);
            exit(0);
          }

          israeli_release(lock_id);
          sleep(1);
        }
      }
    }
  }

  // Parent: wait for all runners to finish
  for(int i = 0; i < NTEAMS * RUNNERS_PER_TEAM; i++)
    wait(0);

  // Print final scoreboard
  printf("--- Final Scores ---\n");
  for(int t = 0; t < NTEAMS; t++)
    printf("  Team %d: %d points\n", t, race_get_score(t));
  printf("  Winner:  Team %d\n", race_get_winner());
  printf("====================\n");

  israeli_destroy(lock_id);
  exit(0);
}
