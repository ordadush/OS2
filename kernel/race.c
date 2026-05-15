#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"

#define RACE_MAX_TEAMS 8

static int race_scores[RACE_MAX_TEAMS];
static int race_nteams;
static int race_target;
static int race_winner;      // -1 = no winner yet
static struct spinlock race_lock;

void
race_init(void)
{
  initlock(&race_lock, "race");
  race_nteams = 0;
  race_target = 0;
  race_winner = -1;
  for(int i = 0; i < RACE_MAX_TEAMS; i++)
    race_scores[i] = 0;
}

// Called from userspace at the start of each race to configure it
int
race_reset(int nteams, int target)
{
  if(nteams <= 0 || nteams > RACE_MAX_TEAMS || target <= 0)
    return -1;
  acquire(&race_lock);
  race_nteams = nteams;
  race_target = target;
  race_winner = -1;
  for(int i = 0; i < RACE_MAX_TEAMS; i++)
    race_scores[i] = 0;
  release(&race_lock);
  return 0;
}

// Add 1 point to team, returns new score (or -1 on error)
int
race_add_score(int team)
{
  if(team < 0 || team >= race_nteams)
    return -1;
  acquire(&race_lock);
  race_scores[team]++;
  int score = race_scores[team];
  if(score >= race_target && race_winner < 0)
    race_winner = team;
  release(&race_lock);
  return score;
}

// Returns current score of team
int
race_get_score(int team)
{
  if(team < 0 || team >= race_nteams)
    return -1;
  acquire(&race_lock);
  int score = race_scores[team];
  release(&race_lock);
  return score;
}

// Returns winning team id, or -1 if no winner yet
int
race_get_winner(void)
{
  acquire(&race_lock);
  int w = race_winner;
  release(&race_lock);
  return w;
}
