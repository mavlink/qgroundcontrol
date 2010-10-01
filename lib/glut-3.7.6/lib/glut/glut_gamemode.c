
/* Copyright (c) Mark J. Kilgard, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glutint.h"

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* SGI optimization introduced in IRIX 6.3 to avoid X server
   round trips for interning common X atoms. */
#if defined(_SGI_EXTRA_PREDEFINES) && !defined(NO_FAST_ATOMS)
#include <X11/SGIFastAtom.h>
#else
#define XSGIFastInternAtom(dpy,string,fast_name,how) XInternAtom(dpy,string,how)
#endif
#endif  /* not _WIN32 */

int __glutDisplaySettingsChanged = 0;
static DisplayMode *dmodes, *currentDm = NULL;
static int ndmodes = -1;
GLUTwindow *__glutGameModeWindow = NULL;
    
#ifdef TEST
static char *compstr[] =
{
  "none", "=", "!=", "<=", ">=", ">", "<", "~"
};
static char *capstr[] =
{
  "width", "height", "bpp", "hertz", "num"
};
#endif

void
__glutCloseDownGameMode(void)
{
  if (__glutDisplaySettingsChanged) {
#ifdef _WIN32
    /* Assumes that display settings have been changed, that
       is __glutDisplaySettingsChanged is true. */
    ChangeDisplaySettings(NULL, 0);
#endif
    __glutDisplaySettingsChanged = 0;
  }
  __glutGameModeWindow = NULL;
}

void APIENTRY
glutLeaveGameMode(void)
{
  if (__glutGameModeWindow == NULL) {
    __glutWarning("not in game mode so cannot leave game mode");
    return;
  }
  __glutDestroyWindow(__glutGameModeWindow,
    __glutGameModeWindow);
  XFlush(__glutDisplay);
  __glutGameModeWindow = NULL;
}

#ifdef _WIN32

/* Same values as from MSDN's SetDisp.c example. */
#define MIN_WIDTH 400
#define MIN_FREQUENCY 60

static void
initGameModeSupport(void)
{
  DEVMODE dm;
  DWORD mode;
  int i;

  if (ndmodes >= 0) {
    /* ndmodes is initially -1 to indicate no
       dmodes allocated yet. */
    return;
  }

  /* Determine how many display modes there are. */
  ndmodes = 0;
  mode = 0;
  while (EnumDisplaySettings(NULL, mode, &dm)) {
    if (dm.dmPelsWidth >= MIN_WIDTH &&
      (dm.dmDisplayFrequency == 0 ||
      dm.dmDisplayFrequency >= MIN_FREQUENCY)) {
      ndmodes++;
    }
    mode++;
  }

  /* Allocate memory for a list of all the display modes. */
  dmodes = (DisplayMode*)
    malloc(ndmodes * sizeof(DisplayMode));

  /* Now that we know how many display modes to expect,
     enumerate them again and save the information in
     the list we allocated above. */
  i = 0;
  mode = 0;
  while (EnumDisplaySettings(NULL, mode, &dm)) {
    /* Try to reject any display settings that seem unplausible. */
    if (dm.dmPelsWidth >= MIN_WIDTH &&
      (dm.dmDisplayFrequency == 0 ||
      dm.dmDisplayFrequency >= MIN_FREQUENCY)) {
      dmodes[i].devmode = dm;
      dmodes[i].valid = 1;  /* XXX Not used for now. */
      dmodes[i].cap[DM_WIDTH] = dm.dmPelsWidth;
      dmodes[i].cap[DM_HEIGHT] = dm.dmPelsHeight;
      dmodes[i].cap[DM_PIXEL_DEPTH] = dm.dmBitsPerPel;
      if (dm.dmDisplayFrequency == 0) {
 	/* Guess a reasonable guess. */
	/* Lame Windows 95 version of EnumDisplaySettings. */
        dmodes[i].cap[DM_HERTZ] = 60;
      } else {
	dmodes[i].cap[DM_HERTZ] = dm.dmDisplayFrequency;
      }
      i++;
    }
    mode++;
  }

  assert(i == ndmodes);
}

#else

/* X Windows version of initGameModeSupport. */
static void
initGameModeSupport(void)
{
  if (ndmodes >= 0) {
    /* ndmodes is initially -1 to indicate no
       dmodes allocated yet. */
    return;
  }

  /* Determine how many display modes there are. */
  ndmodes = 0;
}

#endif

/* This routine is based on similiar code in glut_dstr.c */
static DisplayMode *
findMatch(DisplayMode * dmodes, int ndmodes,
  Criterion * criteria, int ncriteria)
{
  DisplayMode *found;
  int *bestScore, *thisScore;
  int i, j, numok, result, worse, better;

  found = NULL;
  numok = 1;            /* "num" capability is indexed from 1,
                           not 0. */

  /* XXX alloca canidate. */
  bestScore = (int *) malloc(ncriteria * sizeof(int));
  if (!bestScore) {
    __glutFatalError("out of memory.");
  }
  for (j = 0; j < ncriteria; j++) {
    /* Very negative number. */
    bestScore[j] = -32768;
  }

  /* XXX alloca canidate. */
  thisScore = (int *) malloc(ncriteria * sizeof(int));
  if (!thisScore) {
    __glutFatalError("out of memory.");
  }

  for (i = 0; i < ndmodes; i++) {
    if (dmodes[i].valid) {
      worse = 0;
      better = 0;

      for (j = 0; j < ncriteria; j++) {
        int cap, cvalue, dvalue;

        cap = criteria[j].capability;
        cvalue = criteria[j].value;
        if (cap == NUM) {
          dvalue = numok;
        } else {
          dvalue = dmodes[i].cap[cap];
        }
#ifdef TEST
        if (verbose)
          printf("  %s %s %d to %d\n",
            capstr[cap], compstr[criteria[j].comparison], cvalue, dvalue);
#endif
        switch (criteria[j].comparison) {
        case EQ:
          result = cvalue == dvalue;
          thisScore[j] = 1;
          break;
        case NEQ:
          result = cvalue != dvalue;
          thisScore[j] = 1;
          break;
        case LT:
          result = dvalue < cvalue;
          thisScore[j] = dvalue - cvalue;
          break;
        case GT:
          result = dvalue > cvalue;
          thisScore[j] = dvalue - cvalue;
          break;
        case LTE:
          result = dvalue <= cvalue;
          thisScore[j] = dvalue - cvalue;
          break;
        case GTE:
          result = (dvalue >= cvalue);
          thisScore[j] = dvalue - cvalue;
          break;
        case MIN:
          result = dvalue >= cvalue;
          thisScore[j] = cvalue - dvalue;
          break;
        }

#ifdef TEST
        if (verbose)
          printf("                result=%d   score=%d   bestScore=%d\n", result, thisScore[j], bestScore[j]);
#endif

        if (result) {
          if (better || thisScore[j] > bestScore[j]) {
            better = 1;
          } else if (thisScore[j] == bestScore[j]) {
            /* Keep looking. */
          } else {
            goto nextDM;
          }
        } else {
          if (cap == NUM) {
            worse = 1;
          } else {
            goto nextDM;
          }
        }

      }

      if (better && !worse) {
        found = &dmodes[i];
        for (j = 0; j < ncriteria; j++) {
          bestScore[j] = thisScore[j];
        }
      }
      numok++;

    nextDM:;

    }
  }
  free(bestScore);
  free(thisScore);
  return found;
}

/**
 * Parses strings in the form of:
 *  800x600
 *  800x600:16
 *  800x600@60
 *  800x600:16@60
 *  @60
 *  :16
 *  :16@60
 * NOTE that @ before : is not parsed.
 */
static int
specialCaseParse(char *word, Criterion * criterion, int mask)
{
  char *xstr, *response;
  int got;
  int width, height, bpp, hertz;

  switch(word[0]) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    /* The WWWxHHH case. */
    if (mask & (1 << DM_WIDTH)) {
      return -1;
    }
    xstr = strpbrk(&word[1], "x");
    if (xstr) {
      width = (int) strtol(word, &response, 0);
      if (response == word || response[0] != 'x') {
        /* Not a valid number OR needs to be followed by 'x'. */
	return -1;
      }
      height = (int) strtol(&xstr[1], &response, 0);
      if (response == &xstr[1]) {
        /* Not a valid number. */
	return -1;
      }
      criterion[0].capability = DM_WIDTH;
      criterion[0].comparison = EQ;
      criterion[0].value = width;
      criterion[1].capability = DM_HEIGHT;
      criterion[1].comparison = EQ;
      criterion[1].value = height;
      got = specialCaseParse(response,
        &criterion[2], 1 << DM_WIDTH);
      if (got >= 0) {
        return got + 2;
      } else {
        return -1;
      }
    }	
    return -1;
  case ':':
    /* The :BPP case. */
    if (mask & (1 << DM_PIXEL_DEPTH)) {
      return -1;
    }
    bpp = (int) strtol(&word[1], &response, 0);
    if (response == &word[1]) {
      /* Not a valid number. */
      return -1;
    }
    criterion[0].capability = DM_PIXEL_DEPTH;
    criterion[0].comparison = EQ;
    criterion[0].value = bpp;
    got = specialCaseParse(response,
      &criterion[1], (1 << DM_WIDTH) | (1 << DM_PIXEL_DEPTH));
    if (got >= 0) {
      return got + 1;
    } else {
      return -1;
    }
  case '@':
    /* The @HZ case. */
    if (mask & (1 << DM_HERTZ)) {
      return -1;
    }
    hertz = (int) strtol(&word[1], &response, 0);
    if (response == &word[1]) {
      /* Not a valid number. */
      return -1;
    }
    criterion[0].capability = DM_HERTZ;
    criterion[0].comparison = EQ;
    criterion[0].value = hertz;
    got = specialCaseParse(response,
      &criterion[1], ~DM_HERTZ);
    if (got >= 0) {
      return got + 1;
    } else {
      return -1;
    }
  case '\0':
    return 0;
  }
  return -1;
}

/* This routine is based on similiar code in glut_dstr.c */
static int
parseCriteria(char *word, Criterion * criterion)
{
  char *cstr, *vstr, *response;
  int comparator, value;

  cstr = strpbrk(word, "=><!~");
  if (cstr) {
    switch (cstr[0]) {
    case '=':
      comparator = EQ;
      vstr = &cstr[1];
      break;
    case '~':
      comparator = MIN;
      vstr = &cstr[1];
      break;
    case '>':
      if (cstr[1] == '=') {
        comparator = GTE;
        vstr = &cstr[2];
      } else {
        comparator = GT;
        vstr = &cstr[1];
      }
      break;
    case '<':
      if (cstr[1] == '=') {
        comparator = LTE;
        vstr = &cstr[2];
      } else {
        comparator = LT;
        vstr = &cstr[1];
      }
      break;
    case '!':
      if (cstr[1] == '=') {
        comparator = NEQ;
        vstr = &cstr[2];
      } else {
        return -1;
      }
      break;
    default:
      return -1;
    }
    value = (int) strtol(vstr, &response, 0);
    if (response == vstr) {
      /* Not a valid number. */
      return -1;
    }
    *cstr = '\0';
  } else {
    comparator = NONE;
  }
  switch (word[0]) {
  case 'b':
    if (!strcmp(word, "bpp")) {
      criterion[0].capability = DM_PIXEL_DEPTH;
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        return 1;
      }
    }
    return -1;
  case 'h':
    if (!strcmp(word, "height")) {
      criterion[0].capability = DM_HEIGHT;
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        return 1;
      }
    }
    if (!strcmp(word, "hertz")) {
      criterion[0].capability = DM_HERTZ;
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        return 1;
      }
    }
    return -1;
  case 'n':
    if (!strcmp(word, "num")) {
      criterion[0].capability = DM_NUM;
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        return 1;
      }
    }
    return -1;
  case 'w':
    if (!strcmp(word, "width")) {
      criterion[0].capability = DM_WIDTH;
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        return 1;
      }
    }
    return -1;
  }
  if (comparator == NONE) {
    return specialCaseParse(word, criterion, 0);
  }
  return -1;
}

/* This routine is based on similiar code in glut_dstr.c */
static Criterion *
parseDisplayString(const char *display, int *ncriteria)
{
  Criterion *criteria = NULL;
  int n, parsed;
  char *copy, *word;

  copy = __glutStrdup(display);
  /* Attempt to estimate how many criteria entries should be
     needed. */
  n = 0;
  word = strtok(copy, " \t");
  while (word) {
    n++;
    word = strtok(NULL, " \t");
  }
  /* Allocate number of words of criteria.  A word
     could contain as many as four criteria in the
     worst case.  Example: 800x600:16@60 */
  criteria = (Criterion *) malloc(4 * n * sizeof(Criterion));
  if (!criteria) {
    __glutFatalError("out of memory.");
  }

  /* Re-copy the copy of the display string. */
  strcpy(copy, display);

  n = 0;
  word = strtok(copy, " \t");
  while (word) {
    parsed = parseCriteria(word, &criteria[n]);
    if (parsed >= 0) {
      n += parsed;
    } else {
      __glutWarning("Unrecognized game mode string word: %s (ignoring)\n", word);
    }
    word = strtok(NULL, " \t");
  }

  free(copy);
  *ncriteria = n;
  return criteria;
}

void APIENTRY
glutGameModeString(const char *string)
{
  Criterion *criteria;
  int ncriteria;

  initGameModeSupport();
  criteria = parseDisplayString(string, &ncriteria);
  currentDm = findMatch(dmodes, ndmodes, criteria, ncriteria);
  free(criteria);
}

int APIENTRY
glutEnterGameMode(void)
{
  GLUTwindow *window;
  int width, height;
  Window win;

  if (__glutMappedMenu) {
    __glutFatalUsage("entering game mode not allowed while menus in use");
  }
  if (__glutGameModeWindow) {
    /* Already in game mode, so blow away game mode
       window so apps can change resolutions. */
    window = __glutGameModeWindow;
    /* Setting the game mode window to NULL tricks
       the window destroy code into not undoing the
       screen display change since we plan on immediately
       doing another mode change. */
    __glutGameModeWindow = NULL;
    __glutDestroyWindow(window, window);
  }

  /* Assume default screen size until we find out if we
     can actually change the display settings. */
  width = __glutScreenWidth;
  height = __glutScreenHeight;

  if (currentDm) {
#ifdef _WIN32
    LONG status;
    static int registered = 0;

    status = ChangeDisplaySettings(&currentDm->devmode,
      CDS_FULLSCREEN);
    if (status == DISP_CHANGE_SUCCESSFUL) {
      __glutDisplaySettingsChanged = 1;
      width = currentDm->cap[DM_WIDTH];
      height = currentDm->cap[DM_HEIGHT];
      if (!registered) {
        atexit(__glutCloseDownGameMode);
        registered = 1;
      }
    } else {
      /* Switch back to default resolution. */
      ChangeDisplaySettings(NULL, 0);
    }
#endif
  }

  window = __glutCreateWindow(NULL, 0, 0,
    width, height, /* game mode */ 1);
  win = window->win;

#if !defined(_WIN32)
  if (__glutMotifHints == None) {
    __glutMotifHints = XSGIFastInternAtom(__glutDisplay, "_MOTIF_WM_HINTS",
      SGI_XA__MOTIF_WM_HINTS, 0);
    if (__glutMotifHints == None) {
      __glutWarning("Could not intern X atom for _MOTIF_WM_HINTS.");
    }
  }

  /* Game mode window is a toplevel window. */
  XSetWMProtocols(__glutDisplay, win, &__glutWMDeleteWindow, 1);
#endif

  /* Schedule the fullscreen property to be added and to
     make sure the window is configured right.  Win32
     doesn't need this. */
  window->desiredX = 0;
  window->desiredY = 0;
  window->desiredWidth = width;
  window->desiredHeight = height;
  window->desiredConfMask |= CWX | CWY | CWWidth | CWHeight;
#ifdef _WIN32
  /* Win32 does not want to use GLUT_FULL_SCREEN_WORK
     for game mode because we need to be maximizing
     the window in game mode, not just sizing it to
     take up the full screen.  The Win32-ness of game
     mode happens when you pass 1 in the gameMode parameter
     to __glutCreateWindow above.  A gameMode of creates
     a WS_POPUP window, not a standard WS_OVERLAPPEDWINDOW
     window.  WS_POPUP ensures the taskbar is hidden. */
  __glutPutOnWorkList(window,
    GLUT_CONFIGURE_WORK);
#else
  __glutPutOnWorkList(window,
    GLUT_CONFIGURE_WORK | GLUT_FULL_SCREEN_WORK);
#endif

  __glutGameModeWindow = window;
  return window->num + 1;
}

int APIENTRY
glutGameModeGet(GLenum mode)
{
  switch (mode) {
  case GLUT_GAME_MODE_ACTIVE:
    return __glutGameModeWindow != NULL;
  case GLUT_GAME_MODE_POSSIBLE:
    return currentDm != NULL;
  case GLUT_GAME_MODE_WIDTH:
    return currentDm ? currentDm->cap[DM_WIDTH] : -1;
  case GLUT_GAME_MODE_HEIGHT:
    return currentDm ? currentDm->cap[DM_HEIGHT] : -1;
  case GLUT_GAME_MODE_PIXEL_DEPTH:
    return currentDm ? currentDm->cap[DM_PIXEL_DEPTH] : -1;
  case GLUT_GAME_MODE_REFRESH_RATE:
    return currentDm ? currentDm->cap[DM_HERTZ] : -1;
  case GLUT_GAME_MODE_DISPLAY_CHANGED:
    return __glutDisplaySettingsChanged;
  default:
    return -1;
  }
}
