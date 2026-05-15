#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  printf("--- PRNG Test ---\n");

  lcg_srand(42);
  printf("Seeded with 42. Five random numbers:\n");
  for(int i = 0; i < 5; i++)
    printf("  lcg_rand() = %d\n", lcg_rand());

  lcg_srand(42);
  printf("Re-seeded with 42. Should match above:\n");
  for(int i = 0; i < 5; i++)
    printf("  lcg_rand() = %d\n", lcg_rand());

  lcg_srand(99);
  printf("Seeded with 99. Should differ:\n");
  for(int i = 0; i < 5; i++)
    printf("  lcg_rand() = %d\n", lcg_rand());

  lcg_srand(getpid());
  printf("Groups test (4 groups, 10 draws):\n");
  for(int i = 0; i < 10; i++)
    printf("  group = %d\n", lcg_rand() % 4);

  printf("--- Done ---\n");
  exit(0);
}
