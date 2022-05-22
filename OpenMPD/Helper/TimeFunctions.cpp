#include "Helper\TimeFunctions.h"

//HIGH LEVEL FUNCTIONS (PRECISE UP TO ONE MILISECOND)
/**
	Replacement for an accurate real tyme clock for windows with minimum dependencies...
	I use it for profiling performance of various parts of the method.
*/
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	//This version is precise up to the microsecond level
	LARGE_INTEGER nFreq, currentTime; DWORD dwTime;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&currentTime);
	dwTime = (DWORD)(currentTime.QuadPart * 1000000 / nFreq.QuadPart);
	tp->tv_sec  = (long) (dwTime/ 1000000L);//Dividing integer numbers always rounds down
    tp->tv_usec = (long) (dwTime%1000000L);
    return 0;

    /*// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);*/	
}

void createTimeval(struct timeval *result, float timeInMilis) {
	int timeInUsec = (int)(timeInMilis * 1000);
	result->tv_usec = timeInUsec % 1000000;
	result->tv_sec = (timeInUsec - result->tv_usec) / 1000000;
}
/**
	This method substracts the time elapsed between two timestampts recorded using the method above.
*/
int timeval_subtract (struct timeval *result, struct timeval x, struct timeval y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x.tv_usec < y.tv_usec) {
    int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
    y.tv_usec -= 1000000 * nsec;
    y.tv_sec += nsec;
  }
  if (x.tv_usec - y.tv_usec > 1000000) {
    int nsec = (y.tv_usec - x.tv_usec) / 1000000;
    y.tv_usec += 1000000 * nsec;
    y.tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x.tv_sec - y.tv_sec;
  result->tv_usec = x.tv_usec - y.tv_usec;

  /* Return 1 if result is negative. */
  //return x->tv_sec < y->tv_sec;
  
  //Return time difference in usecs
  return (x.tv_sec - y.tv_sec) * 1000000 + (x.tv_usec - y.tv_usec);
}

void timeval_add(struct timeval *result, struct timeval x, struct timeval y)
{
	result->tv_usec = x.tv_usec + y.tv_usec;
	result->tv_sec = x.tv_sec + y.tv_sec;
	//Carry over if usec > 1 second
	if (result->tv_usec > 1000000) {
		result->tv_usec -= 1000000;
		result->tv_sec++;
	}
}

//Get times for each stage in seconds:
float computeTimeElapsed(struct timeval start, struct timeval end) {
	struct timeval timeElapsed;
	timeval_subtract(&timeElapsed,end,start);
	return timeElapsed.tv_sec + 0.000001f*timeElapsed.tv_usec;
}

float computeTimeElapsedInMilis(struct timeval start, struct timeval end) {
	struct timeval timeElapsed;
	timeval_subtract(&timeElapsed,end,start);
	return 1000*timeElapsed.tv_sec + 0.001f*timeElapsed.tv_usec;
}
