#ifndef _TIME_FUNCTIONS
#define _TIME_FUNCTIONS
#include <time.h>
#include <Windows.h>
int gettimeofday(struct timeval * tp, struct timezone * tzp);
void createTimeval(struct timeval *result, float timeInMilis);
int timeval_subtract(struct timeval *result, struct timeval x, struct timeval y);
void timeval_add(struct timeval *result, struct timeval x, struct timeval y);
float computeTimeElapsed(struct timeval start, struct timeval end);
float computeTimeElapsedInMilis(struct timeval start, struct timeval end);

#endif
