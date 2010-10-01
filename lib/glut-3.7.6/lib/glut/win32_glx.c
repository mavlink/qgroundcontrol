
/* Copyright (c) Nate Robins, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include "glutint.h"

/* global current HDC */
extern HDC XHDC;

GLXContext
glXCreateContext(Display * display, XVisualInfo * visinfo,
  GLXContext share, Bool direct)
{
  /* KLUDGE: GLX really expects a display pointer to be passed
     in as the first parameter, but Win32 needs an HDC instead,
     so BE SURE that the global XHDC is set before calling this
     routine. */
  HGLRC context;

  context = wglCreateContext(XHDC);

#if 0
  /* XXX GLUT doesn't support it now, so don't worry about display list
     and texture object sharing. */
  if (share) {
    wglShareLists(share, context);
  }
#endif

  /* Since direct rendering is implicit, the direct flag is
     ignored. */

  return context;
}

int
glXGetConfig(Display * display, XVisualInfo * visual, int attrib, int *value)
{
  if (!visual)
    return GLX_BAD_VISUAL;

  switch (attrib) {
  case GLX_USE_GL:
    if ((visual->dwFlags & PFD_SUPPORT_OPENGL ) && (visual->dwFlags & PFD_DRAW_TO_WINDOW)) {
      /* XXX Brad's Matrix Millenium II has problems creating
         color index windows in 24-bit mode (lead to GDI crash)
         and 32-bit mode (lead to black window).  The cColorBits
         filed of the PIXELFORMATDESCRIPTOR returned claims to
         have 24 and 32 bits respectively of color indices. 2^24
         and 2^32 are ridiculously huge writable colormaps.
         Assume that if we get back a color index
         PIXELFORMATDESCRIPTOR with 24 or more bits, the
         PIXELFORMATDESCRIPTOR doesn't really work and skip it.
         -mjk */
      if (visual->iPixelType == PFD_TYPE_COLORINDEX
        && visual->cColorBits >= 24) {
        *value = 0;
      } else {
	*value = 1;
      }
    } else {
      *value = 0;
    }
    break;
  case GLX_BUFFER_SIZE:
    /* KLUDGE: if we're RGBA, return the number of bits/pixel,
       otherwise, return 8 (we guessed at 256 colors in CI
       mode). */
    if (visual->iPixelType == PFD_TYPE_RGBA)
      *value = visual->cColorBits;
    else
      *value = 8;
    break;
  case GLX_LEVEL:
    /* The bReserved flag of the pfd contains the
       overlay/underlay info. */
    *value = visual->bReserved;
    break;
  case GLX_RGBA:
    *value = visual->iPixelType == PFD_TYPE_RGBA;
    break;
  case GLX_DOUBLEBUFFER:
    *value = visual->dwFlags & PFD_DOUBLEBUFFER;
    break;
  case GLX_STEREO:
    *value = visual->dwFlags & PFD_STEREO;
    break;
  case GLX_AUX_BUFFERS:
    *value = visual->cAuxBuffers;
    break;
  case GLX_RED_SIZE:
    *value = visual->cRedBits;
    break;
  case GLX_GREEN_SIZE:
    *value = visual->cGreenBits;
    break;
  case GLX_BLUE_SIZE:
    *value = visual->cBlueBits;
    break;
  case GLX_ALPHA_SIZE:
    *value = visual->cAlphaBits;
    break;
  case GLX_DEPTH_SIZE:
    *value = visual->cDepthBits;
    break;
  case GLX_STENCIL_SIZE:
    *value = visual->cStencilBits;
    break;
  case GLX_ACCUM_RED_SIZE:
    *value = visual->cAccumRedBits;
    break;
  case GLX_ACCUM_GREEN_SIZE:
    *value = visual->cAccumGreenBits;
    break;
  case GLX_ACCUM_BLUE_SIZE:
    *value = visual->cAccumBlueBits;
    break;
  case GLX_ACCUM_ALPHA_SIZE:
    *value = visual->cAccumAlphaBits;
    break;
  default:
    return GLX_BAD_ATTRIB;
  }
  return 0;
}

XVisualInfo *
glXChooseVisual(Display * display, int screen, int *attribList)
{
  /* KLUDGE: since we need the HDC, MAKE SURE to set XHDC
     before calling this routine. */

  int *p = attribList;
  int pf;
  PIXELFORMATDESCRIPTOR pfd;
  PIXELFORMATDESCRIPTOR *match = NULL;
  int stereo = 0;

  /* Avoid seg-faults. */
  if (!p)
    return NULL;

  memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nSize = (sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nVersion = 1;

  /* Defaults. */
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
  pfd.iPixelType = PFD_TYPE_COLORINDEX;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 0;

  while (*p) {
    switch (*p) {
    case GLX_USE_GL:
      pfd.dwFlags |= PFD_SUPPORT_OPENGL;
      break;
    case GLX_BUFFER_SIZE:
      pfd.cColorBits = *(++p);
      break;
    case GLX_LEVEL:
      /* the bReserved flag of the pfd contains the
         overlay/underlay info. */
      pfd.bReserved = *(++p);
      break;
    case GLX_RGBA:
      pfd.iPixelType = PFD_TYPE_RGBA;
      break;
    case GLX_DOUBLEBUFFER:
      pfd.dwFlags |= PFD_DOUBLEBUFFER;
      break;
    case GLX_STEREO:
      stereo = 1;
      pfd.dwFlags |= PFD_STEREO;
      break;
    case GLX_AUX_BUFFERS:
      pfd.cAuxBuffers = *(++p);
      break;
    case GLX_RED_SIZE:
      pfd.cRedBits = 8; /* Try to get the maximum. */
      ++p;
      break;
    case GLX_GREEN_SIZE:
      pfd.cGreenBits = 8;
      ++p;
      break;
    case GLX_BLUE_SIZE:
      pfd.cBlueBits = 8;
      ++p;
      break;
    case GLX_ALPHA_SIZE:
      pfd.cAlphaBits = 8;
      ++p;
      break;
    case GLX_DEPTH_SIZE:
      pfd.cDepthBits = 32;
      ++p;
      break;
    case GLX_STENCIL_SIZE:
      pfd.cStencilBits = *(++p);
      break;
    case GLX_ACCUM_RED_SIZE:
    case GLX_ACCUM_GREEN_SIZE:
    case GLX_ACCUM_BLUE_SIZE:
    case GLX_ACCUM_ALPHA_SIZE:
      /* I believe that WGL only used the cAccumRedBits,
	 cAccumBlueBits, cAccumGreenBits, and cAccumAlphaBits fields
	 when returning info about the accumulation buffer precision.
	 Only cAccumBits is used for requesting an accumulation
	 buffer. */
      pfd.cAccumBits = 1;
      ++p;
      break;
    }
    ++p;
  }

  /* Let Win32 choose one for us. */
  pf = ChoosePixelFormat(XHDC, &pfd);
  if (pf > 0) {
    match = (PIXELFORMATDESCRIPTOR *) malloc(sizeof(PIXELFORMATDESCRIPTOR));
    DescribePixelFormat(XHDC, pf, sizeof(PIXELFORMATDESCRIPTOR), match);

    /* ChoosePixelFormat is dumb in that it will return a pixel
       format that doesn't have stereo even if it was requested
       so we need to make sure that if stereo was selected, we
       got it. */
    if (stereo) {
      if (!(match->dwFlags & PFD_STEREO)) {
        free(match);
	return NULL;
      }
    }
    /* XXX Brad's Matrix Millenium II has problems creating
       color index windows in 24-bit mode (lead to GDI crash)
       and 32-bit mode (lead to black window).  The cColorBits
       filed of the PIXELFORMATDESCRIPTOR returned claims to
       have 24 and 32 bits respectively of color indices. 2^24
       and 2^32 are ridiculously huge writable colormaps.
       Assume that if we get back a color index
       PIXELFORMATDESCRIPTOR with 24 or more bits, the
       PIXELFORMATDESCRIPTOR doesn't really work and skip it.
       -mjk */
    if (match->iPixelType == PFD_TYPE_COLORINDEX
      && match->cColorBits >= 24) {
      free(match);
      return NULL;
    }
  }
  return match;
}
