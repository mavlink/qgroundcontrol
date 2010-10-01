
/* Copyright (c) Mark J. Kilgard, 1994, 1995, 1996, 1997, 1998. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>  /* Some FD_ZERO macros use memset without
                        prototyping memset. */

/* Much of the following #ifdef logic to include the proper
   prototypes for the select system call is based on logic
   from the X11R6.3 version of <X11/Xpoll.h>. */

#if !defined(_WIN32)
# ifdef __sgi
#  include <bstring.h>    /* prototype for bzero used by FD_ZERO */
# endif
# if (defined(SVR4) || defined(CRAY) || defined(AIXV3)) && !defined(FD_SETSIZE)
#  include <sys/select.h> /* select system call interface */
#  ifdef luna
#   include <sysent.h>
#  endif
# endif
  /* AIX 4.2 fubar-ed <sys/select.h>, so go to heroic measures to get it */
# if defined(AIXV4) && !defined(NFDBITS)
#  include <sys/select.h>
# endif
#endif /* !_WIN32 */

#include <sys/types.h>

#if !defined(_WIN32)
# if defined(__vms) && ( __VMS_VER < 70000000 )
#  include <sys/time.h>
# else
#  ifndef __vms
#   include <sys/time.h>
#  endif
# endif
# include <unistd.h>
# include <X11/Xlib.h>
# include <X11/keysym.h>
#else
# ifdef __CYGWIN32__
#  include <sys/time.h>
# else
#  include <sys/timeb.h>
# endif
# ifdef __hpux
   /* XXX Bert Gijsbers <bert@mc.bio.uva.nl> reports that HP-UX
      needs different keysyms for the End, Insert, and Delete keys
      to work on an HP 715.  It would be better if HP generated
      standard keysyms for standard keys. */
#  include <X11/HPkeysym.h>
# endif
#endif /* !_WIN32 */

#if defined(__vms) && ( __VMS_VER < 70000000 )
#include <ssdef.h>
#include <psldef.h>
extern int SYS$CLREF(int efn);
extern int SYS$SETIMR(unsigned int efn, struct timeval *timeout, void *ast,
  unsigned int request_id, unsigned int flags);
extern int SYS$WFLOR(unsigned int efn, unsigned int mask);
extern int SYS$CANTIM(unsigned int request_id, unsigned int mode);
#endif /* __vms, VMs 6.2 or earlier */

#include "glutint.h"

static GLUTtimer *freeTimerList = NULL;

GLUTidleCB __glutIdleFunc = NULL;
GLUTtimer *__glutTimerList = NULL;
#ifdef SUPPORT_FORTRAN
GLUTtimer *__glutNewTimer;
#endif
GLUTwindow *__glutWindowWorkList = NULL;
GLUTmenu *__glutMappedMenu;
GLUTmenu *__glutCurrentMenu = NULL;

void (*__glutUpdateInputDeviceMaskFunc) (GLUTwindow *);
#if !defined(_WIN32)
void (*__glutMenuItemEnterOrLeave)(GLUTmenuItem * item, int num, int type) = NULL;
void (*__glutFinishMenu)(Window win, int x, int y);
void (*__glutPaintMenu)(GLUTmenu * menu);
void (*__glutStartMenu)(GLUTmenu * menu, GLUTwindow * window, int x, int y, int x_win, int y_win);
GLUTmenu * (*__glutGetMenuByNum)(int menunum);
GLUTmenuItem * (*__glutGetMenuItem)(GLUTmenu * menu, Window win, int *which);
GLUTmenu * (*__glutGetMenu)(Window win);
#endif

Atom __glutMotifHints = None;
/* Modifier mask of ~0 implies not in core input callback. */
unsigned int __glutModifierMask = (unsigned int) ~0;
int __glutWindowDamaged = 0;

void APIENTRY
glutIdleFunc(GLUTidleCB idleFunc)
{
  __glutIdleFunc = idleFunc;
}

void APIENTRY
glutTimerFunc(unsigned int interval, GLUTtimerCB timerFunc, int value)
{
  GLUTtimer *timer, *other;
  GLUTtimer **prevptr;
  struct timeval now;

  if (!timerFunc)
    return;

  if (freeTimerList) {
    timer = freeTimerList;
    freeTimerList = timer->next;
  } else {
    timer = (GLUTtimer *) malloc(sizeof(GLUTtimer));
    if (!timer)
      __glutFatalError("out of memory.");
  }

  timer->func = timerFunc;
#if defined(__vms) && ( __VMS_VER < 70000000 )
  /* VMS time is expressed in units of 100 ns */
  timer->timeout.val = interval * TICKS_PER_MILLISECOND;
#else
  timer->timeout.tv_sec = (int) interval / 1000;
  timer->timeout.tv_usec = (int) (interval % 1000) * 1000;
#endif
  timer->value = value;
  timer->next = NULL;
  GETTIMEOFDAY(&now);
  ADD_TIME(timer->timeout, timer->timeout, now);
  prevptr = &__glutTimerList;
  other = *prevptr;
  while (other && IS_AFTER(other->timeout, timer->timeout)) {
    prevptr = &other->next;
    other = *prevptr;
  }
  timer->next = other;
#ifdef SUPPORT_FORTRAN
  __glutNewTimer = timer;  /* for Fortran binding! */
#endif
  *prevptr = timer;
}

void
handleTimeouts(void)
{
  struct timeval now;
  GLUTtimer *timer;

  /* Assumption is that __glutTimerList is already determined
     to be non-NULL. */
  GETTIMEOFDAY(&now);
  while (IS_AT_OR_AFTER(__glutTimerList->timeout, now)) {
    timer = __glutTimerList;
    timer->func(timer->value);
    __glutTimerList = timer->next;
    timer->next = freeTimerList;
    freeTimerList = timer;
    if (!__glutTimerList)
      break;
  }
}

void
__glutPutOnWorkList(GLUTwindow * window, int workMask)
{
  if (window->workMask) {
    /* Already on list; just OR in new workMask. */
    window->workMask |= workMask;
  } else {
    /* Update work mask and add to window work list. */
    window->workMask = workMask;
    /* Assert that if the window does not have a
       workMask already, the window should definitely
       not be the head of the work list. */
    assert(window != __glutWindowWorkList);
    window->prevWorkWin = __glutWindowWorkList;
    __glutWindowWorkList = window;
  }
}

void
__glutPostRedisplay(GLUTwindow * window, int layerMask)
{
  int shown = (layerMask & (GLUT_REDISPLAY_WORK | GLUT_REPAIR_WORK)) ?
    window->shownState : window->overlay->shownState;

  /* Post a redisplay if the window is visible (or the
     visibility of the window is unknown, ie. window->visState
     == -1) _and_ the layer is known to be shown. */
  if (window->visState != GLUT_HIDDEN
    && window->visState != GLUT_FULLY_COVERED && shown) {
    __glutPutOnWorkList(window, layerMask);
  }
}

/* CENTRY */
void APIENTRY
glutPostRedisplay(void)
{
  __glutPostRedisplay(__glutCurrentWindow, GLUT_REDISPLAY_WORK);
}

/* The advantage of this routine is that it saves the cost of a
   glutSetWindow call (entailing an expensive OpenGL context switch),
   particularly useful when multiple windows need redisplays posted at
   the same times.  See also glutPostWindowOverlayRedisplay. */
void APIENTRY
glutPostWindowRedisplay(int win)
{
  __glutPostRedisplay(__glutWindowList[win - 1], GLUT_REDISPLAY_WORK);
}

/* ENDCENTRY */
static GLUTeventParser *eventParserList = NULL;

/* __glutRegisterEventParser allows another module to register
   to intercept X events types not otherwise acted on by the
   GLUT processEventsAndTimeouts routine.  The X Input
   extension support code uses an event parser for handling X
   Input extension events.  */

void
__glutRegisterEventParser(GLUTeventParser * parser)
{
  parser->next = eventParserList;
  eventParserList = parser;
}

static void
markWindowHidden(GLUTwindow * window)
{
  if (GLUT_HIDDEN != window->visState) {
    GLUTwindow *child;

    if (window->windowStatus) {
      window->visState = GLUT_HIDDEN;
      __glutSetWindow(window);
      window->windowStatus(GLUT_HIDDEN);
    }
    /* An unmap is only reported on a single window; its
       descendents need to know they are no longer visible. */
    child = window->children;
    while (child) {
      markWindowHidden(child);
      child = child->siblings;
    }
  }
}

#if !defined(_WIN32)

static void
purgeStaleWindow(Window win)
{
  GLUTstale **pEntry = &__glutStaleWindowList;
  GLUTstale *entry = __glutStaleWindowList;

  /* Tranverse singly-linked stale window list look for the
     window ID. */
  while (entry) {
    if (entry->win == win) {
      /* Found it; delete it. */
      *pEntry = entry->next;
      free(entry);
      return;
    } else {
      pEntry = &entry->next;
      entry = *pEntry;
    }
  }
}

/* Unlike XNextEvent, if a signal arrives,
   interruptibleXNextEvent will return (with a zero return
   value).  This helps GLUT drop out of XNextEvent if a signal
   is delivered.  The intent is so that a GLUT program can call 
   glutIdleFunc in a signal handler to register an idle func
   and then immediately get dropped into the idle func (after
   returning from the signal handler).  The idea is to make
   GLUT's main loop reliably interruptible by signals. */
static int
interruptibleXNextEvent(Display * dpy, XEvent * event)
{
  fd_set fds;
  int rc;

  /* Flush X protocol since XPending does not do this
     implicitly. */
  XFlush(__glutDisplay);
  for (;;) {
    if (XPending(__glutDisplay)) {
      XNextEvent(dpy, event);
      return 1;
    }
    FD_ZERO(&fds);
    FD_SET(__glutConnectionFD, &fds);
    rc = select(__glutConnectionFD + 1, &fds,
      NULL, NULL, NULL);
    if (rc < 0) {
      if (errno == EINTR) {
        return 0;
      } else {
        __glutFatalError("select error.");
      }
    }
  }
}

#endif

static void
processEventsAndTimeouts(void)
{
#if defined ( _WIN32 )
	MSG msg;
	
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
		if ( msg.message == WM_QUIT ) {
			exit( 0 );
		}
	}
	if (__glutTimerList) {
		handleTimeouts();
	}
#else  /* _WIN32 */

  do {
    static int mappedMenuButton;
    GLUTeventParser *parser;
    XEvent event, ahead;
    GLUTwindow *window;
    GLUTkeyboardCB keyboard;
    GLUTspecialCB special;
    int gotEvent, width, height;

    gotEvent = interruptibleXNextEvent(__glutDisplay, &event);
    if (gotEvent) {
      switch (event.type) {
      case MappingNotify:
        XRefreshKeyboardMapping((XMappingEvent *) & event);
        break;
      case ConfigureNotify:
        window = __glutGetWindow(event.xconfigure.window);
        if (window) {
          if (window->win != event.xconfigure.window) {
            /* Ignore ConfigureNotify sent to the overlay
               planes. GLUT could get here because overlays
               select for StructureNotify events to receive
               DestroyNotify. */
            break;
          }
          width = event.xconfigure.width;
          height = event.xconfigure.height;
          if (width != window->width || height != window->height) {
            if (window->overlay) {
              XResizeWindow(__glutDisplay, window->overlay->win, width, height);
            }
            window->width = width;
            window->height = height;
            __glutSetWindow(window);
            /* Do not execute OpenGL out of sequence with
               respect to the XResizeWindow request! */
            glXWaitX();
            window->reshape(width, height);
            window->forceReshape = False;
            /* A reshape should be considered like posting a
               repair; this is necessary for the "Mesa
               glXSwapBuffers to repair damage" hack to operate
               correctly.  Without it, there's not an initial
               back buffer render from which to blit from when
               damage happens to the window. */
            __glutPostRedisplay(window, GLUT_REPAIR_WORK);
          }
        }
        break;
      case Expose:
        /* compress expose events */
        while (XEventsQueued(__glutDisplay, QueuedAfterReading)
          > 0) {
          XPeekEvent(__glutDisplay, &ahead);
          if (ahead.type != Expose ||
            ahead.xexpose.window != event.xexpose.window) {
            break;
          }
          XNextEvent(__glutDisplay, &event);
        }
        if (event.xexpose.count == 0) {
          GLUTmenu *menu;

          if (__glutMappedMenu &&
            (menu = __glutGetMenu(event.xexpose.window))) {
            __glutPaintMenu(menu);
          } else {
            window = __glutGetWindow(event.xexpose.window);
            if (window) {
              if (window->win == event.xexpose.window) {
                __glutPostRedisplay(window, GLUT_REPAIR_WORK);
              } else if (window->overlay && window->overlay->win == event.xexpose.window) {
                __glutPostRedisplay(window, GLUT_OVERLAY_REPAIR_WORK);
              }
            }
          }
        } else {
          /* there are more exposes to read; wait to redisplay */
        }
        break;
      case ButtonPress:
      case ButtonRelease:
        if (__glutMappedMenu && event.type == ButtonRelease
          && mappedMenuButton == event.xbutton.button) {
          /* Menu is currently popped up and its button is
             released. */
          __glutFinishMenu(event.xbutton.window, event.xbutton.x, event.xbutton.y);
        } else {
          window = __glutGetWindow(event.xbutton.window);
          if (window) {
            GLUTmenu *menu;
	    int menuNum;

            menuNum = window->menu[event.xbutton.button - 1];
            /* Make sure that __glutGetMenuByNum is only called if there
	       really is a menu present. */
            if ((menuNum > 0) && (menu = __glutGetMenuByNum(menuNum))) {
              if (event.type == ButtonPress && !__glutMappedMenu) {
                __glutStartMenu(menu, window,
                  event.xbutton.x_root, event.xbutton.y_root,
                  event.xbutton.x, event.xbutton.y);
                mappedMenuButton = event.xbutton.button;
              } else {
                /* Ignore a release of a button with a menu
                   attatched to it when no menu is popped up,
                   or ignore a press when another menu is
                   already popped up. */
              }
            } else if (window->mouse) {
              __glutSetWindow(window);
              __glutModifierMask = event.xbutton.state;
              window->mouse(event.xbutton.button - 1,
                event.type == ButtonRelease ?
                GLUT_UP : GLUT_DOWN,
                event.xbutton.x, event.xbutton.y);
              __glutModifierMask = ~0;
            } else {
              /* Stray mouse events.  Ignore. */
            }
          } else {
            /* Window might have been destroyed and all the 
               events for the window may not yet be received. */
          }
        }
        break;
      case MotionNotify:
        if (!__glutMappedMenu) {
          window = __glutGetWindow(event.xmotion.window);
          if (window) {
            /* If motion function registered _and_ buttons held 
               * down, call motion function...  */
            if (window->motion && event.xmotion.state &
              (Button1Mask | Button2Mask | Button3Mask)) {
              __glutSetWindow(window);
              window->motion(event.xmotion.x, event.xmotion.y);
            }
            /* If passive motion function registered _and_
               buttons not held down, call passive motion
               function...  */
            else if (window->passive &&
                ((event.xmotion.state &
                    (Button1Mask | Button2Mask | Button3Mask)) ==
                0)) {
              __glutSetWindow(window);
              window->passive(event.xmotion.x,
                event.xmotion.y);
            }
          }
        } else {
          /* Motion events are thrown away when a pop up menu
             is active. */
        }
        break;
      case KeyPress:
      case KeyRelease:
        window = __glutGetWindow(event.xkey.window);
        if (!window) {
          break;
        }
	if (event.type == KeyPress) {
	  keyboard = window->keyboard;
	} else {

	  /* If we are ignoring auto repeated keys for this window,
	     check if the next event in the X event queue is a KeyPress
	     for the exact same key (and at the exact same time) as the
	     key being released.  The X11 protocol will send auto
	     repeated keys as such KeyRelease/KeyPress pairs. */

	  if (window->ignoreKeyRepeat) {
	    if (XEventsQueued(__glutDisplay, QueuedAfterReading)) {
	      XPeekEvent(__glutDisplay, &ahead);
	      if (ahead.type == KeyPress
	        && ahead.xkey.window == event.xkey.window
	        && ahead.xkey.keycode == event.xkey.keycode
	        && ahead.xkey.time == event.xkey.time) {
		/* Pop off the repeated KeyPress and ignore
		   the auto repeated KeyRelease/KeyPress pair. */
	        XNextEvent(__glutDisplay, &event);
	        break;
	      }
	    }
	  }
	  keyboard = window->keyboardUp;
	}
        if (keyboard) {
          char tmp[1];
          int rc;

          rc = XLookupString(&event.xkey, tmp, sizeof(tmp),
            NULL, NULL);
          if (rc) {
            __glutSetWindow(window);
            __glutModifierMask = event.xkey.state;
            keyboard(tmp[0],
              event.xkey.x, event.xkey.y);
            __glutModifierMask = ~0;
            break;
          }
        }
	if (event.type == KeyPress) {
	  special = window->special;
        } else {
	  special = window->specialUp;
	}
        if (special) {
          KeySym ks;
          int key;

/* Introduced in X11R6:  (Partial list of) Keypad Functions.  Define
   in place in case compiling against an older pre-X11R6
   X11/keysymdef.h file. */
#ifndef XK_KP_Home
#define XK_KP_Home              0xFF95
#endif
#ifndef XK_KP_Left
#define XK_KP_Left              0xFF96
#endif
#ifndef XK_KP_Up
#define XK_KP_Up                0xFF97
#endif
#ifndef XK_KP_Right
#define XK_KP_Right             0xFF98
#endif
#ifndef XK_KP_Down
#define XK_KP_Down              0xFF99
#endif
#ifndef XK_KP_Prior
#define XK_KP_Prior             0xFF9A
#endif
#ifndef XK_KP_Next
#define XK_KP_Next              0xFF9B
#endif
#ifndef XK_KP_End
#define XK_KP_End               0xFF9C
#endif
#ifndef XK_KP_Insert
#define XK_KP_Insert            0xFF9E
#endif
#ifndef XK_KP_Delete
#define XK_KP_Delete            0xFF9F
#endif

          ks = XLookupKeysym((XKeyEvent *) & event, 0);
          /* XXX Verbose, but makes no assumptions about keysym
             layout. */
          switch (ks) {
/* *INDENT-OFF* */
          /* function keys */
          case XK_F1:    key = GLUT_KEY_F1; break;
          case XK_F2:    key = GLUT_KEY_F2; break;
          case XK_F3:    key = GLUT_KEY_F3; break;
          case XK_F4:    key = GLUT_KEY_F4; break;
          case XK_F5:    key = GLUT_KEY_F5; break;
          case XK_F6:    key = GLUT_KEY_F6; break;
          case XK_F7:    key = GLUT_KEY_F7; break;
          case XK_F8:    key = GLUT_KEY_F8; break;
          case XK_F9:    key = GLUT_KEY_F9; break;
          case XK_F10:   key = GLUT_KEY_F10; break;
          case XK_F11:   key = GLUT_KEY_F11; break;
          case XK_F12:   key = GLUT_KEY_F12; break;
          /* directional keys */
	  case XK_KP_Left:
          case XK_Left:  key = GLUT_KEY_LEFT; break;
	  case XK_KP_Up: /* Introduced in X11R6. */
          case XK_Up:    key = GLUT_KEY_UP; break;
	  case XK_KP_Right: /* Introduced in X11R6. */
          case XK_Right: key = GLUT_KEY_RIGHT; break;
	  case XK_KP_Down: /* Introduced in X11R6. */
          case XK_Down:  key = GLUT_KEY_DOWN; break;
/* *INDENT-ON* */

	  case XK_KP_Prior: /* Introduced in X11R6. */
          case XK_Prior:
            /* XK_Prior same as X11R6's XK_Page_Up */
            key = GLUT_KEY_PAGE_UP;
            break;
	  case XK_KP_Next: /* Introduced in X11R6. */
          case XK_Next:
            /* XK_Next same as X11R6's XK_Page_Down */
            key = GLUT_KEY_PAGE_DOWN;
            break;
	  case XK_KP_Home: /* Introduced in X11R6. */
          case XK_Home:
            key = GLUT_KEY_HOME;
            break;
#ifdef __hpux
          case XK_Select:
#endif
	  case XK_KP_End: /* Introduced in X11R6. */
          case XK_End:
            key = GLUT_KEY_END;
            break;
#ifdef __hpux
          case XK_InsertChar:
#endif
	  case XK_KP_Insert: /* Introduced in X11R6. */
          case XK_Insert:
            key = GLUT_KEY_INSERT;
            break;
#ifdef __hpux
          case XK_DeleteChar:
#endif
	  case XK_KP_Delete: /* Introduced in X11R6. */
            /* The Delete character is really an ASCII key. */
            __glutSetWindow(window);
            keyboard(127,  /* ASCII Delete character. */
              event.xkey.x, event.xkey.y);
            goto skip;
          default:
            goto skip;
          }
          __glutSetWindow(window);
          __glutModifierMask = event.xkey.state;
          special(key, event.xkey.x, event.xkey.y);
          __glutModifierMask = ~0;
        skip:;
        }
        break;
      case EnterNotify:
      case LeaveNotify:
        if (event.xcrossing.mode != NotifyNormal ||
          event.xcrossing.detail == NotifyNonlinearVirtual ||
          event.xcrossing.detail == NotifyVirtual) {

          /* Careful to ignore Enter/LeaveNotify events that
             come from the pop-up menu pointer grab and ungrab. 
             Also, ignore "virtual" Enter/LeaveNotify events
             since they represent the pointer passing through
             the window hierarchy without actually entering or
             leaving the actual real estate of a window.  */

          break;
        }
        if (__glutMappedMenu) {
          GLUTmenuItem *item;
          int num;

          item = __glutGetMenuItem(__glutMappedMenu,
            event.xcrossing.window, &num);
          if (item) {
            __glutMenuItemEnterOrLeave(item, num, event.type);
            break;
          }
        }
        window = __glutGetWindow(event.xcrossing.window);
        if (window) {
          if (window->entry) {
            if (event.type == EnterNotify) {

              /* With overlays established, X can report two
                 enter events for both the overlay and normal
                 plane window. Do not generate a second enter
                 callback if we reported one without an
                 intervening leave. */

              if (window->entryState != EnterNotify) {
                int num = window->num;
                Window xid = window->win;

                window->entryState = EnterNotify;
                __glutSetWindow(window);
                window->entry(GLUT_ENTERED);

                if (__glutMappedMenu) {

                  /* Do not generate any passive motion events
                     when menus are in use. */

                } else {

                  /* An EnterNotify event can result in a
                     "compound" callback if a passive motion
                     callback is also registered. In this case,
                     be a little paranoid about the possibility
                     the window could have been destroyed in the
                     entry callback. */

                  window = __glutWindowList[num];
                  if (window && window->passive && window->win == xid) {
                    __glutSetWindow(window);
                    window->passive(event.xcrossing.x, event.xcrossing.y);
                  }
                }
              }
            } else {
              if (window->entryState != LeaveNotify) {

                /* When an overlay is established for a window
                   already mapped and with the pointer in it,
                   the X server will generate a leave/enter
                   event pair as the pointer leaves (without
                   moving) from the normal plane X window to
                   the newly mapped overlay  X window (or vice
                   versa). This enter/leave pair should not be
                   reported to the GLUT program since the pair
                   is a consequence of creating (or destroying) 
                   the overlay, not an actual leave from the
                   GLUT window. */

                if (XEventsQueued(__glutDisplay, QueuedAfterReading)) {
                  XPeekEvent(__glutDisplay, &ahead);
                  if (ahead.type == EnterNotify &&
                    __glutGetWindow(ahead.xcrossing.window) == window) {
                    XNextEvent(__glutDisplay, &event);
                    break;
                  }
                }
                window->entryState = LeaveNotify;
                __glutSetWindow(window);
                window->entry(GLUT_LEFT);
              }
            }
          } else if (window->passive) {
            __glutSetWindow(window);
            window->passive(event.xcrossing.x, event.xcrossing.y);
          }
        }
        break;
      case UnmapNotify:
        /* MapNotify events are not needed to maintain
           visibility state since VisibilityNotify events will
           be delivered when a window becomes visible from
           mapping.  However, VisibilityNotify events are not
           delivered when a window is unmapped (for the window
           or its children). */
        window = __glutGetWindow(event.xunmap.window);
        if (window) {
          if (window->win != event.xconfigure.window) {
            /* Ignore UnmapNotify sent to the overlay planes.
               GLUT could get here because overlays select for
               StructureNotify events to receive DestroyNotify. 
             */
            break;
          }
          markWindowHidden(window);
        }
        break;
      case VisibilityNotify:
        window = __glutGetWindow(event.xvisibility.window);
        if (window) {
          /* VisibilityUnobscured+1 = GLUT_FULLY_RETAINED,
             VisibilityPartiallyObscured+1 =
             GLUT_PARTIALLY_RETAINED, VisibilityFullyObscured+1 
             =  GLUT_FULLY_COVERED. */
          int visState = event.xvisibility.state + 1;

          if (visState != window->visState) {
            if (window->windowStatus) {
              window->visState = visState;
              __glutSetWindow(window);
              window->windowStatus(visState);
            }
          }
        }
        break;
      case ClientMessage:
        if (event.xclient.data.l[0] == __glutWMDeleteWindow)
          exit(0);
        break;
      case DestroyNotify:
        purgeStaleWindow(event.xdestroywindow.window);
        break;
      case CirculateNotify:
      case CreateNotify:
      case GravityNotify:
      case ReparentNotify:
        /* Uninteresting to GLUT (but possible for GLUT to
           receive). */
        break;
      default:
        /* Pass events not directly handled by the GLUT main
           event loop to any event parsers that have been
           registered.  In this way, X Input extension events
           are passed to the correct handler without forcing
           all GLUT programs to support X Input event handling. 
         */
        parser = eventParserList;
        while (parser) {
          if (parser->func(&event))
            break;
          parser = parser->next;
        }
        break;
      }
    }
    if (__glutTimerList) {
      handleTimeouts();
    }
  }
  while (XPending(__glutDisplay));
#endif /* _WIN32 */
}

static void
waitForSomething(void)
{
#if defined(__vms) && ( __VMS_VER < 70000000 )
  static struct timeval zerotime =
  {0};
  unsigned int timer_efn;
#define timer_id 'glut' /* random :-) number */
  unsigned int wait_mask;
#else
  static struct timeval zerotime =
  {0, 0};
#if !defined(_WIN32)
  fd_set fds;
#endif
#endif
  struct timeval now, timeout, waittime;
#if !defined(_WIN32)
  int rc;
#endif

  /* Flush X protocol since XPending does not do this
     implicitly. */
  XFlush(__glutDisplay);
  if (XPending(__glutDisplay)) {
    /* It is possible (but quite rare) that XFlush may have
       needed to wait for a writable X connection file
       descriptor, and in the process, may have had to read off
       X protocol from the file descriptor. If XPending is true,
       this case occured and we should avoid waiting in select
       since X protocol buffered within Xlib is due to be
       processed and potentially no more X protocol is on the
       file descriptor, so we would risk waiting improperly in
       select. */
    goto immediatelyHandleXinput;
  }
#if defined(__vms) && ( __VMS_VER < 70000000 )
  timeout = __glutTimerList->timeout;
  GETTIMEOFDAY(&now);
  wait_mask = 1 << (__glutConnectionFD & 31);
  if (IS_AFTER(now, timeout)) {
    /* We need an event flag for the timer. */
    /* XXX The `right' way to do this is to use LIB$GET_EF, but
       since it needs to be in the same cluster as the EFN for
       the display, we will have hack it. */
    timer_efn = __glutConnectionFD - 1;
    if ((timer_efn / 32) != (__glutConnectionFD / 32)) {
      timer_efn = __glutConnectionFD + 1;
    }
    rc = SYS$CLREF(timer_efn);
    rc = SYS$SETIMR(timer_efn, &timeout, NULL, timer_id, 0);
    wait_mask |= 1 << (timer_efn & 31);
  } else {
    timer_efn = 0;
  }
  rc = SYS$WFLOR(__glutConnectionFD, wait_mask);
  if (timer_efn != 0 && SYS$CLREF(timer_efn) == SS$_WASCLR) {
    rc = SYS$CANTIM(timer_id, PSL$C_USER);
  }
  /* XXX There does not seem to be checking of "rc" in the code
     above.  Can any of the SYS$ routines above fail? */
#else /* not vms6.2 or lower */
#if !defined(_WIN32)
  FD_ZERO(&fds);
  FD_SET(__glutConnectionFD, &fds);
#endif
  timeout = __glutTimerList->timeout;
  GETTIMEOFDAY(&now);
  if (IS_AFTER(now, timeout)) {
    TIMEDELTA(waittime, timeout, now);
  } else {
    waittime = zerotime;
  }
#if !defined(_WIN32)
  rc = select(__glutConnectionFD + 1, &fds,
    NULL, NULL, &waittime);
  if (rc < 0 && errno != EINTR)
    __glutFatalError("select error.");
#else
  MsgWaitForMultipleObjects(0, NULL, FALSE, waittime.tv_sec*1000 + waittime.tv_usec/1000, QS_ALLEVENTS);
#endif
#endif /* not vms6.2 or lower */
  /* Without considering the cause of select unblocking, check
     for pending X events and handle any timeouts (by calling
     processEventsAndTimeouts).  We always look for X events
     even if select returned with 0 (indicating a timeout);
     otherwise we risk starving X event processing by continous
     timeouts. */
  if (XPending(__glutDisplay)) {
  immediatelyHandleXinput:
    processEventsAndTimeouts();
  } else {
    if (__glutTimerList)
      handleTimeouts();
  }
}

static void
idleWait(void)
{
  if (XPending(__glutDisplay)) {
    processEventsAndTimeouts();
  } else {
    if (__glutTimerList) {
      handleTimeouts();
    }
  }
  /* Make sure idle func still exists! */
  if (__glutIdleFunc) {
    __glutIdleFunc();
  }
}

static GLUTwindow **beforeEnd;

static GLUTwindow *
processWindowWorkList(GLUTwindow * window)
{
  int workMask;

  if (window->prevWorkWin) {
    window->prevWorkWin = processWindowWorkList(window->prevWorkWin);
	if (beforeEnd == 0)
      beforeEnd = &window->prevWorkWin;
  } else {
    beforeEnd = &window->prevWorkWin;
  }

  /* Capture work mask for work that needs to be done to this
     window, then clear the window's work mask (excepting the
     dummy work bit, see below).  Then, process the captured
     work mask.  This allows callbacks in the processing the
     captured work mask to set the window's work mask for
     subsequent processing. */

  workMask = window->workMask;
  assert((workMask & GLUT_DUMMY_WORK) == 0);

  /* Set the dummy work bit, clearing all other bits, to
     indicate that the window is currently on the window work
     list _and_ that the window's work mask is currently being
     processed.  This convinces __glutPutOnWorkList that this
     window is on the work list still. */
  window->workMask = GLUT_DUMMY_WORK;

  /* Optimization: most of the time, the work to do is a
     redisplay and not these other types of work.  Check for
     the following cases as a group to before checking each one
     individually one by one. This saves about 25 MIPS
     instructions in the common redisplay only case. */
  if (workMask & (GLUT_EVENT_MASK_WORK | GLUT_DEVICE_MASK_WORK |
      GLUT_CONFIGURE_WORK | GLUT_COLORMAP_WORK | GLUT_MAP_WORK)) {
#if !defined(_WIN32)
    /* Be sure to set event mask BEFORE map window is done. */
    if (workMask & GLUT_EVENT_MASK_WORK) {
      long eventMask;

      /* Make sure children are not propogating events this
         window is selecting for.  Be sure to do this before
         enabling events on the children's parent. */
      if (window->children) {
        GLUTwindow *child = window->children;
        unsigned long attribMask = CWDontPropagate;
        XSetWindowAttributes wa;

        wa.do_not_propagate_mask = window->eventMask & GLUT_DONT_PROPAGATE_FILTER_MASK;
        if (window->eventMask & GLUT_HACK_STOP_PROPAGATE_MASK) {
          wa.event_mask = child->eventMask | (window->eventMask & GLUT_HACK_STOP_PROPAGATE_MASK);
          attribMask |= CWEventMask;
        }
        do {
          XChangeWindowAttributes(__glutDisplay, child->win,
            attribMask, &wa);
          child = child->siblings;
        } while (child);
      }
      eventMask = window->eventMask;
      if (window->parent && window->parent->eventMask & GLUT_HACK_STOP_PROPAGATE_MASK)
        eventMask |= (window->parent->eventMask & GLUT_HACK_STOP_PROPAGATE_MASK);
      XSelectInput(__glutDisplay, window->win, eventMask);
      if (window->overlay)
        XSelectInput(__glutDisplay, window->overlay->win,
          window->eventMask & GLUT_OVERLAY_EVENT_FILTER_MASK);
    }
#endif /* !_WIN32 */
    /* Be sure to set device mask BEFORE map window is done. */
    if (workMask & GLUT_DEVICE_MASK_WORK) {
      __glutUpdateInputDeviceMaskFunc(window);
    }
    /* Be sure to configure window BEFORE map window is done. */
    if (workMask & GLUT_CONFIGURE_WORK) {
#if defined(_WIN32)
        if ( workMask & GLUT_FULL_SCREEN_WORK ) {
            DWORD s;
            RECT r;

            GetWindowRect(GetDesktopWindow(), &r);
            s = GetWindowLong(window->win, GWL_STYLE);
            s &= ~WS_OVERLAPPEDWINDOW;
            s |= WS_POPUP;
            SetWindowLong(window->win, GWL_STYLE, s);
            SetWindowPos(window->win, 
                HWND_TOP, /* safer - a lot of people use windows atop a fullscreen GLUT window. */
				//HWND_TOPMOST, /* is better, but no windows atop it */
                r.left, r.top, 
                r.right-r.left, r.bottom-r.top, 
                SWP_FRAMECHANGED);
            
            /* This hack causes the window to go back to the right position
            when it is taken out of fullscreen mode. */
            {
                POINT p;

                p.x = 0;
                p.y = 0;
                ClientToScreen(window->win, &p);
                window->desiredConfMask |= CWX | CWY;
                window->desiredX = p.x;
                window->desiredY = p.y;
            }
        } else {
            RECT changes;
            POINT point;
            UINT flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_NOZORDER;
            DWORD style;
			
            GetClientRect(window->win, &changes);
            style = GetWindowLong(window->win, GWL_STYLE);
            
            /* Get rid of fullscreen mode, if it exists */
            if ( style & WS_POPUP ) {
                style &= ~WS_POPUP;
                style |= WS_OVERLAPPEDWINDOW;
                SetWindowLong(window->win, GWL_STYLE, style);
                flags |= SWP_FRAMECHANGED;
            }

            /* If this window is a toplevel window, translate the 0,0 client
            coordinate into a screen coordinate for proper placement. */
            if (!window->parent) {
                point.x = 0;
                point.y = 0;
                ClientToScreen(window->win, &point);
                changes.left = point.x;
                changes.top = point.y;
            }
            if (window->desiredConfMask & (CWX | CWY)) {
                changes.left = window->desiredX;
                changes.top = window->desiredY;
                flags &= ~SWP_NOMOVE;
            }
            if (window->desiredConfMask & (CWWidth | CWHeight)) {
                changes.right = changes.left + window->desiredWidth;
                changes.bottom = changes.top + window->desiredHeight;
                flags &= ~SWP_NOSIZE;
                /* XXX If overlay exists, resize the overlay here, ie.
                if (window->overlay) ... */
            }
            if (window->desiredConfMask & CWStackMode) {
                flags &= ~SWP_NOZORDER;
                /* XXX Overlay support might require something special here. */
            }
            
            /* Adjust the window rectangle because Win32 thinks that the x, y,
            width & height are the WHOLE window (including decorations),
            whereas GLUT treats the x, y, width & height as only the CLIENT
            area of the window.  Only do this to top level windows
            that are not in game mode (since game mode windows do
            not have any decorations). */
            if (!window->parent && window != __glutGameModeWindow) {
                AdjustWindowRect(&changes, style, FALSE);
            }
            
            /* Do the repositioning, moving, and push/pop. */
            SetWindowPos(window->win,
                window->desiredStack == Above ? HWND_TOP : HWND_BOTTOM,
                changes.left, changes.top,
                changes.right - changes.left, changes.bottom - changes.top,
                flags);
            
            /* Zero out the mask. */
            window->desiredConfMask = 0;
        }
#else /* !_WIN32 */
      XWindowChanges changes;

      changes.x = window->desiredX;
      changes.y = window->desiredY;
      if (window->desiredConfMask & (CWWidth | CWHeight)) {
        changes.width = window->desiredWidth;
        changes.height = window->desiredHeight;
        if (window->overlay)
          XResizeWindow(__glutDisplay, window->overlay->win,
            window->desiredWidth, window->desiredHeight);
        if (__glutMotifHints != None) {
          if (workMask & GLUT_FULL_SCREEN_WORK) {
            MotifWmHints hints;

            hints.flags = MWM_HINTS_DECORATIONS;
            hints.decorations = 0;  /* Absolutely no
                                       decorations. */
            XChangeProperty(__glutDisplay, window->win,
              __glutMotifHints, __glutMotifHints, 32,
              PropModeReplace, (unsigned char *) &hints, 4);
            if (workMask & GLUT_MAP_WORK) {
              /* Handle case where glutFullScreen is called
                 before the first time that the window is
                 mapped. Some window managers will randomly or
                 interactively position the window the first
                 time it is mapped if the window's
                 WM_NORMAL_HINTS property does not request an
                 explicit position. We don't want any such
                 window manager interaction when going
                 fullscreen.  Overwrite the WM_NORMAL_HINTS
                 property installed by glutCreateWindow's
                 XSetWMProperties property with one explicitly
                 requesting a fullscreen window. */
              XSizeHints hints;

              hints.flags = USPosition | USSize;
              hints.x = 0;
              hints.y = 0;
              hints.width = window->desiredWidth;
              hints.height = window->desiredHeight;
              XSetWMNormalHints(__glutDisplay, window->win, &hints);
            }
          } else {
            XDeleteProperty(__glutDisplay, window->win, __glutMotifHints);
          }
        }
      }
      if (window->desiredConfMask & CWStackMode) {
        changes.stack_mode = window->desiredStack;
        /* Do not let glutPushWindow push window beneath the
           underlay. */
        if (window->parent && window->parent->overlay
          && window->desiredStack == Below) {
          changes.stack_mode = Above;
          changes.sibling = window->parent->overlay->win;
          window->desiredConfMask |= CWSibling;
        }
      }
      XConfigureWindow(__glutDisplay, window->win,
        window->desiredConfMask, &changes);
      window->desiredConfMask = 0;
#endif
    }
#if !defined(_WIN32)
    /* Be sure to establish the colormaps BEFORE map window is
       done. */
    if (workMask & GLUT_COLORMAP_WORK) {
      __glutEstablishColormapsProperty(window);
    }
#endif
    if (workMask & GLUT_MAP_WORK) {
      switch (window->desiredMapState) {
      case WithdrawnState:
        if (window->parent) {
          XUnmapWindow(__glutDisplay, window->win);
        } else {
          XWithdrawWindow(__glutDisplay, window->win,
            __glutScreen);
        }
        window->shownState = 0;
        break;
      case NormalState:
        XMapWindow(__glutDisplay, window->win);
        window->shownState = 1;
        break;
#ifdef _WIN32
      case GameModeState:  /* Not an Xlib value. */
        ShowWindow(window->win, SW_SHOW);
        window->shownState = 1;
        break;
#endif
      case IconicState:
        XIconifyWindow(__glutDisplay, window->win, __glutScreen);
        window->shownState = 0;
        break;
      }
    }
  }
  if (workMask & (GLUT_REDISPLAY_WORK | GLUT_OVERLAY_REDISPLAY_WORK | GLUT_REPAIR_WORK | GLUT_OVERLAY_REPAIR_WORK)) {
    if (window->forceReshape) {
      /* Guarantee that before a display callback is generated
         for a window, a reshape callback must be generated. */
      __glutSetWindow(window);
      window->reshape(window->width, window->height);
      window->forceReshape = False;

      /* Setting the redisplay bit on the first reshape is
         necessary to make the "Mesa glXSwapBuffers to repair
         damage" hack operate correctly.  Without indicating a
         redisplay is necessary, there's not an initial back
         buffer render from which to blit from when damage
         happens to the window. */
      workMask |= GLUT_REDISPLAY_WORK;
    }
    /* The code below is more involved than otherwise necessary
       because it is paranoid about the overlay or entire window
       being removed or destroyed in the course of the callbacks.
       Notice how the global __glutWindowDamaged is used to record
       the layers' damage status.  See the code in glutLayerGet for
       how __glutWindowDamaged is used. The  point is to not have to
       update the "damaged" field after  the callback since the
       window (or overlay) may be destroyed (or removed) when the
       callback returns. */

    if (window->overlay && window->overlay->display) {
      int num = window->num;
      Window xid = window->overlay ? window->overlay->win : None;

      /* If an overlay display callback is registered, we
         differentiate between a redisplay needed for the
         overlay and/or normal plane.  If there is no overlay
         display callback registered, we simply use the
         standard display callback. */

      if (workMask & (GLUT_REDISPLAY_WORK | GLUT_REPAIR_WORK)) {
        if (__glutMesaSwapHackSupport) {
          if (window->usedSwapBuffers) {
            if ((workMask & (GLUT_REPAIR_WORK | GLUT_REDISPLAY_WORK)) == GLUT_REPAIR_WORK) {
	      SWAP_BUFFERS_WINDOW(window);
              goto skippedDisplayCallback1;
            }
          }
        }
        /* Render to normal plane. */
#ifdef _WIN32
        window->renderDc = window->hdc;
#endif
        window->renderWin = window->win;
        window->renderCtx = window->ctx;
        __glutWindowDamaged = (workMask & GLUT_REPAIR_WORK);
        __glutSetWindow(window);
        window->usedSwapBuffers = 0;
        window->display();
        __glutWindowDamaged = 0;

      skippedDisplayCallback1:;
      }
      if (workMask & (GLUT_OVERLAY_REDISPLAY_WORK | GLUT_OVERLAY_REPAIR_WORK)) {
        window = __glutWindowList[num];
        if (window && window->overlay &&
          window->overlay->win == xid && window->overlay->display) {

          /* Render to overlay. */
#ifdef _WIN32
          window->renderDc = window->overlay->hdc;
#endif
          window->renderWin = window->overlay->win;
          window->renderCtx = window->overlay->ctx;
          __glutWindowDamaged = (workMask & GLUT_OVERLAY_REPAIR_WORK);
          __glutSetWindow(window);
          window->overlay->display();
          __glutWindowDamaged = 0;
        } else {
          /* Overlay may have since been destroyed or the
             overlay callback may have been disabled during
             normal display callback. */
        }
      }
    } else {
      if (__glutMesaSwapHackSupport) {
        if (!window->overlay && window->usedSwapBuffers) {
          if ((workMask & (GLUT_REPAIR_WORK | GLUT_REDISPLAY_WORK)) == GLUT_REPAIR_WORK) {
	    SWAP_BUFFERS_WINDOW(window);
            goto skippedDisplayCallback2;
          }
        }
      }
      /* Render to normal plane (and possibly overlay). */
      __glutWindowDamaged = (workMask & (GLUT_OVERLAY_REPAIR_WORK | GLUT_REPAIR_WORK));
      __glutSetWindow(window);
      window->usedSwapBuffers = 0;
      window->display();
      __glutWindowDamaged = 0;

    skippedDisplayCallback2:;
    }
  }
  /* Combine workMask with window->workMask to determine what
     finish and debug work there is. */
  workMask |= window->workMask;

  if (workMask & GLUT_FINISH_WORK) {
    /* Finish work makes sure a glFinish gets done to indirect
       rendering contexts.  Indirect contexts tend to have much 
       longer latency because lots of OpenGL extension requests 
       can queue up in the X protocol stream. __glutSetWindow
       is where the finish works gets queued for indirect
       contexts. */
    __glutSetWindow(window);
    glFinish();
  }
  if (workMask & GLUT_DEBUG_WORK) {
    __glutSetWindow(window);
    glutReportErrors();
  }
  /* Strip out dummy, finish, and debug work bits. */
  window->workMask &= ~(GLUT_DUMMY_WORK | GLUT_FINISH_WORK | GLUT_DEBUG_WORK);
  if (window->workMask) {
    /* Leave on work list. */
    return window;
  } else {
	if (beforeEnd == &window->prevWorkWin)
	  beforeEnd = 0;
    /* Remove current window from work list. */
    return window->prevWorkWin;
  }
}

/* CENTRY */
void APIENTRY
glutMainLoop(void)
{
#if !defined(_WIN32)
  if (!__glutDisplay)
    __glutFatalUsage("main loop entered with out proper initialization.");
#endif
  if (!__glutWindowListSize)
    __glutFatalUsage(
      "main loop entered with no windows created.");
  for (;;) {
    if (__glutWindowWorkList) {
      GLUTwindow *remainder, *work;

      work = __glutWindowWorkList;
      __glutWindowWorkList = NULL;
      if (work) {
        remainder = processWindowWorkList(work);
        if (remainder) {
          *beforeEnd = __glutWindowWorkList;
          __glutWindowWorkList = remainder;
        }
      }
    }
    if (__glutIdleFunc || __glutWindowWorkList) {
      idleWait();
    } else {
      if (__glutTimerList) {
        waitForSomething();
      } else {
        processEventsAndTimeouts();
      }
#if defined(_WIN32)
	  // If there is no idle function, go to sleep for a millisecond (we 
	  // still need to possibly service timers) or until there is some 
	  // event in our queue.
      MsgWaitForMultipleObjects(0, NULL, FALSE, 1, QS_ALLEVENTS);
#endif
    }
 }
}
/* ENDCENTRY */
