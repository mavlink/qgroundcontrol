
/* fullscreen_stereo.c  --  GLUT support for full screen stereo mode  on SGI
   workstations. */

/* 24-Oct-95 Mike Blackwell  mkb@cs.cmu.edu */

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/extensions/SGIStereo.h>

/* We need to access some GLUT internal variables - this include file  is
   found in the GLUT source code distribution. */

/* XXX I do not normally encourage programs to use GLUT internals.  Programs
   that do (like this one) are inherently unportable GLUT programs.  In the
   case of SGI's low-end stereo there was enough demand to warrant supplying
   an example, and the low-end stereo is not clean enough to be supported
   directly in GLUT. -mjk */

#include "glutint.h"

#include "fullscreen_stereo.h"

/* XXX Video display modes for stereo are selected by running
   /usr/gfx/setmon; in IRIX 6.2 and later releases, the XSGIvc API supplies
   the functionality of setmon and more. */

void
start_fullscreen_stereo(void)
{
  int event, error;

  if (!XSGIStereoQueryExtension(__glutDisplay, &event, &error)) {
    fprintf(stderr, "Stereo not supported on this display!\n");
    exit(0);
  }
  if (XSGIQueryStereoMode(__glutDisplay, __glutCurrentWindow->win) < 0) {
    fprintf(stderr, "Stereo not supported on this window!\n");
    exit(0);
  }
  if (system("/usr/gfx/setmon -n STR_BOT") != 0) {
    fprintf(stderr, "setmon attempt failed!\n");
    stop_fullscreen_stereo();
    exit(0);
  }
}

void
stop_fullscreen_stereo(void)
{
  system("/usr/gfx/setmon -n 72hz");
}

void
stereo_left_buffer(void)
{
  XSGISetStereoBuffer(__glutDisplay, __glutCurrentWindow->win, STEREO_BUFFER_LEFT);
  XSync(__glutDisplay, False);
  glViewport(0, 0, XMAXSCREEN, YSTEREO);
}

void
stereo_right_buffer(void)
{
  XSGISetStereoBuffer(__glutDisplay, __glutCurrentWindow->win, STEREO_BUFFER_RIGHT);
  XSync(__glutDisplay, False);
  glViewport(0, 0, XMAXSCREEN, YSTEREO);
}
