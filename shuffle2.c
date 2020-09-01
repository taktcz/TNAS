/*
//MAX - number of elements (starts with 1);
//compile with gcc shuffle2.c -o shuffle2 && ./shuffle2.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define randrange(N) rand() / (RAND_MAX/(N) + 1)

#define MAX 15        // Values will be in the range (1 .. MAX) 
static int vektor[254];
int candidates[MAX];

int main (void) {
  u_int16_t i;

  srand(time(NULL));   //Seed the random number generator. 

  for (i=0; i<MAX; i++)
    candidates[i] = i;

  for (i = 0; i < MAX-1; i++) {
    int c = randrange(MAX-i);
    int t = candidates[i];
    candidates[i] = candidates[i+c];
    candidates[i+c] = t;
  }

  for (i=0; i<MAX; i++)
    vektor[i] = candidates[i] + 1;

  for (i=0; i<MAX; i++)
    printf("%i, ", vektor[i]);

  return 0;
}
*/
