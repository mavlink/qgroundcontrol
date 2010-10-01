
/* Copyright (c) Nate Robins, 1997. */

/* portions Copyright (c) Mark Kilgard, 1997, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */


#include "glutint.h"
#include "glutstroke.h"
#include "glutbitmap.h"
#if defined(__CYGWIN32__)
typedef MINMAXINFO* LPMINMAXINFO;
#else
#include <sys/timeb.h>
#endif

/* The following added by Paul Garceau <pgarceau@teleport.com> */
#if defined(__MINGW32__)
#include <time.h>
#include <windows.h>
struct timeval;
#endif

extern StrokeFontRec glutStrokeRoman, glutStrokeMonoRoman;
extern BitmapFontRec glutBitmap8By13, glutBitmap9By15, glutBitmapTimesRoman10, glutBitmapTimesRoman24, glutBitmapHelvetica10, glutBitmapHelvetica12, glutBitmapHelvetica18;

int
gettimeofday(struct timeval* tp, void* tzp)
{
  LARGE_INTEGER t;

  if(QueryPerformanceCounter(&t)) {
      /* hardware supports a performance counter */
      LARGE_INTEGER f;
      QueryPerformanceFrequency(&f);
      tp->tv_sec = t.QuadPart/f.QuadPart;
      tp->tv_usec = ((float)t.QuadPart/f.QuadPart*1000*1000) - (tp->tv_sec*1000*1000);
  } else {
      /* hardware doesn't support a performance counter,
         so get the time in a more traditional way. */
      DWORD t;
      t = timeGetTime();
      tp->tv_sec = t / 1000;
      tp->tv_usec = t % 1000;
  }

  /* 0 indicates that the call succeeded. */
  return 0;
}

/* To get around the fact that Microsoft DLLs only allow functions
   to be exported and now data addresses (as Unix DSOs support), the
   GLUT API constants such as GLUT_STROKE_ROMAN have to get passed
   through a case statement to get mapped to the actual data structure
   address. */
void*
__glutFont(void *font)
{
  switch((int)font) {
  case (int)GLUT_STROKE_ROMAN:
    return &glutStrokeRoman;
  case (int)GLUT_STROKE_MONO_ROMAN:
    return &glutStrokeMonoRoman;
  case (int)GLUT_BITMAP_9_BY_15:
    return &glutBitmap9By15;
  case (int)GLUT_BITMAP_8_BY_13:
    return &glutBitmap8By13;
  case (int)GLUT_BITMAP_TIMES_ROMAN_10:
    return &glutBitmapTimesRoman10;
  case (int)GLUT_BITMAP_TIMES_ROMAN_24:
    return &glutBitmapTimesRoman24;
  case (int)GLUT_BITMAP_HELVETICA_10:
    return &glutBitmapHelvetica10;
  case (int)GLUT_BITMAP_HELVETICA_12:
    return &glutBitmapHelvetica12;
  case (int)GLUT_BITMAP_HELVETICA_18:
    return &glutBitmapHelvetica18;
  }
  __glutFatalError("out of memory.");
  /* NOTREACHED */

  return NULL;	/* keep MSVC compiler happy */
}

int
__glutGetTransparentPixel(Display * dpy, XVisualInfo * vinfo)
{
  /* the transparent pixel on Win32 is always index number 0.  So if
     we put this routine in this file, we can avoid compiling the
     whole of layerutil.c which is where this routine normally comes
     from. */
  return 0;
}

void
__glutAdjustCoords(Window parent, int* x, int* y, int* width, int* height)
{
  RECT rect;

  /* adjust the window rectangle because Win32 thinks that the x, y,
     width & height are the WHOLE window (including decorations),
     whereas GLUT treats the x, y, width & height as only the CLIENT
     area of the window. */
  rect.left = *x; rect.top = *y;
  rect.right = *x + *width; rect.bottom = *y + *height;

  /* must adjust the coordinates according to the correct style
     because depending on the style, there may or may not be
     borders. */
  AdjustWindowRect(&rect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		   (parent ? WS_CHILD : WS_OVERLAPPEDWINDOW),
		   FALSE);
  /* FALSE in the third parameter = window has no menu bar */

  /* readjust if the x and y are offscreen */
  if(rect.left < 0) {
    *x = 0;
  } else {
    *x = rect.left;
  }
  
  if(rect.top < 0) {
    *y = 0;
  } else {
    *y = rect.top;
  }

  *width = rect.right - rect.left;	/* adjusted width */
  *height = rect.bottom - rect.top;	/* adjusted height */
}

