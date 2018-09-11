#include "timer.h"

#include <stdio.h>
#include <time.h>

#if defined(__WIN32__) || defined(WIN32)
#include <Windows.h>
DWORD tvs, tve;
#else
#include <sys/time.h>
struct timeval	tvs, tve;
#endif


void StartTimer(void)
{
#if defined(__WIN32__) || defined(WIN32)
  tvs = GetTickCount();
#else
  if (gettimeofday(&tvs,0)) fprintf(stderr,"cant get time!\n");
#endif
}

double StopTimer(void)
{
#if defined(__WIN32__) || defined(WIN32)
  tve = GetTickCount();
  return tve - tvs;
#else
  if (gettimeofday(&tve,0)) fprintf(stderr,"cant get time!\n");
  return 1000 * (tve.tv_sec - tvs.tv_sec + (double)(tve.tv_usec - tvs.tv_usec) / 1000000.0);
#endif
}

double PrintTimer(void)
{
  double t;

#if defined(__WIN32__) || defined(WIN32)
  tve = GetTickCount();
  t = (double)(tve - tvs) / 1000.0;
  printf("%.3f ",t);
  return t;
#else
  if (gettimeofday(&tve,0)) fprintf(stderr,"cant get time!\n");
  t = 1000 * (tve.tv_sec - tvs.tv_sec + (double)(tve.tv_usec - tvs.tv_usec)/1000000.0);
  printf("%.3f ",t);
  return t;
#endif
}

