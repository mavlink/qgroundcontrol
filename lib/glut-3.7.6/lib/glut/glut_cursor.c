
/* Copyright (c) Mark J. Kilgard, 1995, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include "glutint.h"

#if !defined(_WIN32)
#include <X11/Xatom.h>  /* For XA_CURSOR */
#include <X11/cursorfont.h>
#endif

typedef struct _CursorTable {
#if defined(_WIN32)
  char* glyph;
#else
  int glyph;
#endif
  Cursor cursor;
} CursorTable;
/* *INDENT-OFF* */

static CursorTable cursorTable[] = {
  {XC_arrow, None},		  /* GLUT_CURSOR_RIGHT_ARROW */
  {XC_top_left_arrow, None},	  /* GLUT_CURSOR_LEFT_ARROW */
  {XC_hand1, None},		  /* GLUT_CURSOR_INFO */
  {XC_pirate, None},		  /* GLUT_CURSOR_DESTROY */
  {XC_question_arrow, None},	  /* GLUT_CURSOR_HELP */
  {XC_exchange, None},		  /* GLUT_CURSOR_CYCLE */
  {XC_spraycan, None},		  /* GLUT_CURSOR_SPRAY */
  {XC_watch, None},		  /* GLUT_CURSOR_WAIT */
  {XC_xterm, None},		  /* GLUT_CURSOR_TEXT */
  {XC_crosshair, None},		  /* GLUT_CURSOR_CROSSHAIR */
  {XC_sb_v_double_arrow, None},	  /* GLUT_CURSOR_UP_DOWN */
  {XC_sb_h_double_arrow, None},	  /* GLUT_CURSOR_LEFT_RIGHT */
  {XC_top_side, None},		  /* GLUT_CURSOR_TOP_SIDE */
  {XC_bottom_side, None},	  /* GLUT_CURSOR_BOTTOM_SIDE */
  {XC_left_side, None},		  /* GLUT_CURSOR_LEFT_SIDE */
  {XC_right_side, None},	  /* GLUT_CURSOR_RIGHT_SIDE */
  {XC_top_left_corner, None},	  /* GLUT_CURSOR_TOP_LEFT_CORNER */
  {XC_top_right_corner, None},	  /* GLUT_CURSOR_TOP_RIGHT_CORNER */
  {XC_bottom_right_corner, None}, /* GLUT_CURSOR_BOTTOM_RIGHT_CORNER */
  {XC_bottom_left_corner, None},  /* GLUT_CURSOR_BOTTOM_LEFT_CORNER */
};
/* *INDENT-ON* */

#if !defined(_WIN32)
static Cursor blankCursor = None;
static Cursor fullCrosshairCusor = None;

/* SGI X server's support a special property called the
   _SGI_CROSSHAIR_CURSOR that when installed as a window's
   cursor, becomes a full screen crosshair cursor.  SGI
   has special cursor generation hardware for this case. */
static Cursor
getFullCrosshairCursor(void)
{
  Cursor cursor;
  Atom crosshairAtom, actualType;
  int rc, actualFormat;
  unsigned long n, left;
  unsigned long *value;

  if (fullCrosshairCusor == None) {
    crosshairAtom = XInternAtom(__glutDisplay,
      "_SGI_CROSSHAIR_CURSOR", True);
    if (crosshairAtom != None) {
      value = 0;        /* Make compiler happy. */
      rc = XGetWindowProperty(__glutDisplay, __glutRoot,
        crosshairAtom, 0, 1, False, XA_CURSOR, &actualType,
        &actualFormat, &n, &left, (unsigned char **) &value);
      if (rc == Success && actualFormat == 32 && n >= 1) {
        cursor = value[0];
        XFree(value);
        return cursor;
      }
    }
  }
  return XCreateFontCursor(__glutDisplay, XC_crosshair);
}

/* X11 forces you to create a blank cursor if you want
   to disable the cursor. */
static Cursor
makeBlankCursor(void)
{
  static char data[1] =
  {0};
  Cursor cursor;
  Pixmap blank;
  XColor dummy;

  blank = XCreateBitmapFromData(__glutDisplay, __glutRoot,
    data, 1, 1);
  if (blank == None)
    __glutFatalError("out of memory.");
  cursor = XCreatePixmapCursor(__glutDisplay, blank, blank,
    &dummy, &dummy, 0, 0);
  XFreePixmap(__glutDisplay, blank);

  return cursor;
}
#endif /* !_WIN32 */

/* Win32 and X11 use this same function to accomplish
   fairly different tasks.  X11 lets you just define the
   cursor for a window and the window system takes care
   of making sure that the window's cursor is installed
   when the mouse is in the window.  Win32 requires the
   application to handle a WM_SETCURSOR message to install
   the right cursor when windows are entered.  Think of
   the Win32 __glutSetCursor (called from __glutWindowProc)
   as "install cursor".  Think of the X11 __glutSetCursor
   (called from glutSetCursor) as "define cursor". */
void 
__glutSetCursor(GLUTwindow *window)
{
  int cursor = window->cursor;
  Cursor xcursor;

  if (cursor >= 0 &&
    cursor < sizeof(cursorTable) / sizeof(cursorTable[0])) {
    if (cursorTable[cursor].cursor == None) {
      cursorTable[cursor].cursor = XCreateFontCursor(__glutDisplay,
        cursorTable[cursor].glyph);
    }
    xcursor = cursorTable[cursor].cursor;
  } else {
    /* Special cases. */
    switch (cursor) {
    case GLUT_CURSOR_INHERIT:
#if defined(_WIN32)
      while (window->parent) {
        window = window->parent;
        if (window->cursor != GLUT_CURSOR_INHERIT) {
          __glutSetCursor(window);
          return;
        }
      }
      /* XXX Default to an arrow cursor.  Is this
         right or should we be letting the default
         window proc be installing some system cursor? */
      xcursor = cursorTable[0].cursor;
      if (xcursor == NULL) {
        xcursor =
          cursorTable[0].cursor =
          LoadCursor(NULL, cursorTable[0].glyph);
      }
#else
      xcursor = None;
#endif
      break;
    case GLUT_CURSOR_NONE:
#if defined(_WIN32)
      xcursor = NULL;
#else
      if (blankCursor == None) {
        blankCursor = makeBlankCursor();
      }
      xcursor = blankCursor;
#endif
      break;
    case GLUT_CURSOR_FULL_CROSSHAIR:
#if defined(_WIN32)
      xcursor = LoadCursor(NULL, IDC_CROSS);
#else
      if (fullCrosshairCusor == None) {
        fullCrosshairCusor = getFullCrosshairCursor();
      }
      xcursor = fullCrosshairCusor;
#endif
      break;
    }
  }
  XDefineCursor(__glutDisplay,
    window->win, xcursor);
  XFlush(__glutDisplay);
}

/* CENTRY */
void APIENTRY 
glutSetCursor(int cursor)
{
#ifdef _WIN32
  POINT point;

  __glutCurrentWindow->cursor = cursor;
  /* Are we in the window right now?  If so,
     install the cursor. */
  GetCursorPos(&point);
  if (__glutCurrentWindow->win == WindowFromPoint(point)) {
    __glutSetCursor(__glutCurrentWindow);
  }
#else
  __glutCurrentWindow->cursor = cursor;
  __glutSetCursor(__glutCurrentWindow);
#endif
}
/* ENDCENTRY */
