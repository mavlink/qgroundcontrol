
/**
 * My first GLUT prog.
 * Uses most GLUT calls to prove it all works.
 * G Edwards, 30 Aug 95.
 *
 * Notes:
 * Display lists are not shared between windows, and there doesn't seem to be
 *  any provision for this in GLUT. See glxCreateContext.
 *
 * The windows are internally indexed 0,1,2,3,4,5,6. The actual window ids
 * returned by glut are held in winId[0], ...
 *
 * Todo:
 *
 * Could reorder the windows so 0,1,2,3,4,5,6 are the gfx, 7,8 text.
 *
 * 30 Aug 95  GJE  Created. Version 1.00
 * 05 Sep 95  GJE  Version 1.01. 
 * 07 Sep 95  GJE  Version 1.02. More or less complete. All possible GLUT
 *                 calls used, except dials/buttons/tablet/spaceball stuff.
 * 15 Sep 95  GJE  Add "trackball" code.
 *
 *  Calls not used yet: these callbacks are registered but inactive.
 *
 *  glutSpaceball<xxx>Func
 *  glutButtonBoxFunc
 *  glutDialsFunc
 *  glutTabletMotionFunc
 *  glutTabletButtonFunc
 *
 * Tested on:
 *  R3K Indigo Starter
 *  R4K Indigo Elan
 *  R4K Indy XZ
 *  R4K Indy XL
 *  R4K Indigo2 Extreme
 */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Controls */

#define VERSION    "1.00"
#define DATE       "16Sep95"
#define DELAY      1000 /* delay for timer test */
#define MENUDELAY  200  /* hack to fix glutMenuStateFunc bug */
#define MAXWIN     9    /* max no. of windows */

unsigned int AUTODELAY = 1500;   /* delay in demo mode  */

#define VERSIONLONG "Version " VERSION "/" DATE ", compiled " __DATE__ ", " __TIME__ ", file " __FILE__

int pos[MAXWIN][2] =
{
  {50, 150},            /* win 0  */
  {450, 150},           /* win 1  */
  {50, 600},            /* win 2  */
  {450, 600},           /* win 3  */
  {10, 10},             /* subwin 4 (relative to parent win 0) */
  {300, 400},           /* help win 5  */
  {850, 150},           /* cmap win 6  */
  {850, 600},           /* cmap win 7  */
  {250, 450}            /* text win 8  */
};

int size[MAXWIN][2] =
{
  {350, 350},           /* win 0  */
  {350, 350},           /* win 1  */
  {350, 350},           /* win 2  */
  {350, 350},           /* win 3  */
  {200, 200},           /* subwin 4  */
  {700, 300},           /* help win 5  */
  {350, 350},           /* cmap win 6  */
  {350, 350},           /* cmap win 7  */
  {800, 450}            /* text win 8  */
};

/* Macros */

#define PR     if(debug)printf

/* #define GLNEWLIST(a, b)  glNewList(a, b), fprintf(stderr,
   "creating list %d \n", a); */
/* #define GLCALLLIST(a)    glCallList(a), fprintf(stderr,
   "calling list %d \n", a); */
/* #define GLUTSETWINDOW(x) glutSetWindow(x), fprintf(stderr,
   "gsw at %d\n", __LINE__) */

/* Globals */

int winId[MAXWIN] =
{0};                    /* table of glut window id's  */
GLboolean winVis[MAXWIN] =
{GL_FALSE};                /* is window visible  */

GLboolean text[MAXWIN] =
{GL_FALSE};                /* is text on  */
GLboolean winFreeze[MAXWIN] =
{GL_FALSE};                /* user requested menuFreeze  */
GLboolean menuFreeze = GL_FALSE;  /* menuFreeze while menus posted  */
GLboolean timerOn = GL_FALSE;  /* timer active  */
GLboolean animation = GL_TRUE;  /* idle func animation on  */
GLboolean debug = GL_FALSE;  /* dump all events  */
GLboolean showKeys = GL_FALSE;  /* dump key events  */
GLboolean demoMode = GL_FALSE;  /* run automatic demo  */
GLboolean backdrop = GL_FALSE;  /* use backdrop polygon  */
GLboolean passive = GL_FALSE;  /* report passive motions  */
GLboolean leftDown = GL_FALSE;  /* left button down ?  */
GLboolean middleDown = GL_FALSE;  /* middle button down ?  */

int displayMode = GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH;
int currentShape = 0;   /* current glut shape  */
int scrollLine = 0, scrollCol = 0;  /* help scrolling params  */
int lineWidth = 1;      /* line width  */
int angle = 0;      /* global rotation angle  */
char *textPtr[1000] =
{0};                    /* pointers to text window text  */
int textCount = 0, helpCount = 0;  /* text list indexes  */
float scaleFactor = 0.0;  /* window size scale factor  */

#ifdef ALPHA
#undef ALPHA  /* Avoid problems with DEC's ALPHA machines. */
#endif

int menu1, menu2, menu3, menu4, menu5, menu6 = 0, menu7, menu8;
enum {
  RGBA, INDEX, SINGLE, DOUBLEBUFFER, DEPTH, ACCUM, ALPHA, STENCIL, MULTISAMPLE,
  STEREO, MODES
};
char *modeNames[] =
{"RGBA", "INDEX", "SINGLE", "DOUBLE", "DEPTH", "ACCUM",
  "ALPHA", "STENCIL", "MULTISAMPLE", "STEREO"};
int glutMode[] =
{GLUT_RGBA, GLUT_INDEX, GLUT_SINGLE, GLUT_DOUBLE, GLUT_DEPTH,
  GLUT_ACCUM, GLUT_ALPHA, GLUT_STENCIL, GLUT_MULTISAMPLE, GLUT_STEREO};
int modes[MODES] =
{0};
GLboolean menuButton[3] =
{0, 0, 1};
enum {
  MOUSEBUTTON, MOUSEMOTION, APPLY, RESET
};

/* Prototypes */

void gfxInit(int);
void drawScene(void);
void idleFunc(void);
void reshapeFunc(int width, int height);
void visible(int state);
void keyFunc(unsigned char key, int x, int y);
void mouseFunc(int button, int state, int x, int y);
void motionFunc(int x, int y);
void passiveMotionFunc(int x, int y);
void entryFunc(int state);
void specialFunc(int key, int x, int y);
void menuStateFunc(int state);
void timerFunc(int value);
#if 0
void delayedReinstateMenuStateCallback(int value);
#endif
void menuFunc(int value);
void showText(void);
void textString(int x, int y, char *str, void *font);
void strokeString(int x, int y, char *str, void *font);
void makeMenus(void);
int idToIndex(int id);
void setInitDisplayMode(void);
void createMenu6(void);
void removeCallbacks(void);
void addCallbacks(void);
void dumpIds(void);
void updateHelp(void);
void updateAll(void);
void killWindow(int index);
void makeWindow(int index);
void warning(char *msg);
void autoDemo(int);
void positionWindow(int index);
void reshapeWindow(int index);
void iconifyWindow(int index);
void showWindow(int index);
void hideWindow(int index);
void pushWindow(int index);
void popWindow(int index);
void attachMenus(void);
void killAllWindows(void);
void makeAllWindows(void);
void updateText(void);
void updateScrollWindow(int index, char **ptr);
void redefineShapes(int shape);
GLboolean match(char *, char *);
void checkArgs(int argc, char *argv[]);
void scaleWindows(float);
void commandLineHelp(void);
void trackBall(int mode, int button, int state, int x, int y);

void spaceballMotionCB(int, int, int);
void spaceballRotateCB(int, int, int);
void spaceballButtonCB(int, int);
void buttonBoxCB(int, int);
void dialsCB(int, int);
void tabletMotionCB(int, int);
void tabletButtonCB(int, int, int, int);

/* strdup is actually not a standard ANSI C or POSIX routine
   so implement a private one.  OpenVMS does not have a strdup; Linux's
   standard libc doesn't declare strdup by default (unless BSD or SVID
   interfaces are requested). */
static char *
stralloc(const char *string)
{
  char *copy;

  copy = malloc(strlen(string) + 1);
  if (copy == NULL)
    return NULL;
  strcpy(copy, string);
  return copy;
}

/* main */

int
main(int argc, char **argv)
{

/* General init */

  glutInit(&argc, argv);

/* Check args */

  checkArgs(argc, argv);

/* Scale window position/size if needed. Ignore aspect ratios. */

  if (scaleFactor > 0.0)
    scaleWindows(scaleFactor);
  else
    scaleWindows(glutGet(GLUT_SCREEN_WIDTH) / 1280.0);

/* Set initial display mode */

  modes[RGBA] = 1;
  modes[DOUBLEBUFFER] = 1;
  modes[DEPTH] = 1;
  setInitDisplayMode();

/* Set up menus */

  makeMenus();

/* Make some windows */

  makeWindow(0);
  makeWindow(1);

/* Global callbacks */

  glutIdleFunc(idleFunc);
  glutMenuStateFunc(menuStateFunc);

/* Start demo if needed */

  if (demoMode)
    autoDemo(-2);

/* Fall into event loop */

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

/* gfxInit - Init opengl for each window */

void
gfxInit(int index)
{
  GLfloat grey10[] =
  {0.10, 0.10, 0.10, 1.0};
  GLfloat grey20[] =
  {0.2, 0.2, 0.2, 1.0};
  GLfloat black[] =
  {0.0, 0.0, 0.0, 0.0};
  GLfloat diffuse0[] =
  {1.0, 0.0, 0.0, 1.0};
  GLfloat diffuse1[] =
  {0.0, 1.0, 0.0, 1.0};
  GLfloat diffuse2[] =
  {1.0, 1.0, 0.0, 1.0};
  GLfloat diffuse3[] =
  {0.0, 1.0, 1.0, 1.0};
  GLfloat diffuse4[] =
  {1.0, 0.0, 1.0, 1.0};

#define XX  3
#define YY  3
#define ZZ  -2.5

  float vertex[][3] =
  {
    {-XX, -YY, ZZ},
    {+XX, -YY, ZZ},
    {+XX, +YY, ZZ},
    {-XX, +YY, ZZ}
  };

/* warning: This func mixes RGBA and CMAP calls in an ugly
   fashion */

  redefineShapes(currentShape);  /* set up display lists  */
  glutSetWindow(winId[index]);  /* hack - redefineShapes
                                   changes glut win */

/* Shaded backdrop square (RGB or CMAP) */

  glNewList(100, GL_COMPILE);
  glPushAttrib(GL_LIGHTING);
  glDisable(GL_LIGHTING);
  glBegin(GL_POLYGON);

  glColor4fv(black);
  glIndexi(0);
  glVertex3fv(vertex[0]);

  glColor4fv(grey10);
  glIndexi(3);
  glVertex3fv(vertex[1]);

  glColor4fv(grey20);
  glIndexi(4);
  glVertex3fv(vertex[2]);

  glColor4fv(grey10);
  glIndexi(7);
  glVertex3fv(vertex[3]);

  glEnd();
  glPopAttrib();
  glIndexi(9);
  glEndList();

/* Set proj+view */

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, 1.0, 1.0, 20.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.);
  glTranslatef(0.0, 0.0, -1.0);

  if (index == 6 || index == 7)
    goto colorindex;

/* Set basic material, lighting for RGB windows */

  if (index == 0)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse0);
  else if (index == 1)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse1);
  else if (index == 2)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse2);
  else if (index == 3)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse3);
  else if (index == 4)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse4);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);

  if (index == 4)
    glClearColor(0.15, 0.15, 0.15, 1);
  else
    glClearColor(0.1, 0.1, 0.1, 1.0);

  return;

/* Set GL basics for CMAP windows 6,7 */

colorindex:

  glEnable(GL_DEPTH_TEST);
  if (glutGet(GLUT_WINDOW_COLORMAP_SIZE) < 16)
    warning("Color map size too small for color index window");

/* Try to reuse an existing color map */

  if ((index == 6) && (winId[7] != 0)) {
    glutCopyColormap(winId[7]);
  } else if ((index == 7) && (winId[6] != 0)) {
    glutCopyColormap(winId[6]);
  } else {
    glutSetColor(8, 0.1, 0.1, 0.1);
    glutSetColor(9, 1.0, 0.5, 0.0);
    glutSetColor(10, 1.0, 0.6, 0.8);
  }
  glClearIndex(8);
  glIndexi(index + 3);

}

/* makeMenus - Create popup menus */

void
makeMenus(void)
{

/* General control / debug */

  menu2 = glutCreateMenu(menuFunc);
  glutAddMenuEntry("toggle auto demo mode (a)", 312);
  glutAddMenuEntry("toggle freezing in menus", 300);
  glutAddMenuEntry("toggle text per window (t)", 301);
  glutAddMenuEntry("toggle global timer", 302);
  glutAddMenuEntry("toggle global animation", 303);
  glutAddMenuEntry("toggle per window animation", 304);
  glutAddMenuEntry("toggle debug prints (D)", 305);
  glutAddMenuEntry("toggle shaded backdrop", 307);
  glutAddMenuEntry("toggle passive motion callback", 308);
  glutAddMenuEntry("increase line width (l)", 310);
  glutAddMenuEntry("decrease line width  (L)", 311);

/* Shapes */

  menu3 = glutCreateMenu(menuFunc);
  glutAddMenuEntry("sphere", 200);
  glutAddMenuEntry("cube", 201);
  glutAddMenuEntry("cone", 202);
  glutAddMenuEntry("torus", 203);
  glutAddMenuEntry("dodecahedron", 204);
  glutAddMenuEntry("octahedron", 205);
  glutAddMenuEntry("tetrahedron", 206);
  glutAddMenuEntry("icosahedron", 207);
  glutAddMenuEntry("teapot", 208);

/* Open/close windows */

  menu4 = glutCreateMenu(menuFunc);
  glutAddMenuEntry("open all windows", 450);
  glutAddMenuEntry("close all windows", 451);
  glutAddMenuEntry(" ", 9999);
  glutAddMenuEntry("create win 0", 400);
  glutAddMenuEntry("create win 1", 401);
  glutAddMenuEntry("create win 2", 402);
  glutAddMenuEntry("create win 3", 403);
  glutAddMenuEntry("create sub window", 404);
  glutAddMenuEntry("create color index win 6", 406);
  glutAddMenuEntry("create color index win 7", 407);
  glutAddMenuEntry(" ", 9999);
  glutAddMenuEntry("destroy win 0", 410);
  glutAddMenuEntry("destroy win 1", 411);
  glutAddMenuEntry("destroy win 2", 412);
  glutAddMenuEntry("destroy win 3", 413);
  glutAddMenuEntry("destroy sub window", 414);
  glutAddMenuEntry("destroy color index win 6", 416);
  glutAddMenuEntry("destroy color index win 7", 417);

/* Window manager stuff */

  menu5 = glutCreateMenu(menuFunc);
  glutAddMenuEntry("move current win", 430);
  glutAddMenuEntry("resize current win", 431);
  glutAddMenuEntry("iconify current win", 432);
  glutAddMenuEntry("show current win", 433);
  glutAddMenuEntry("hide current win", 434);
  glutAddMenuEntry("push current win", 435);
  glutAddMenuEntry("pop current win", 436);
  glutAddMenuEntry(" ", 9999);
  glutAddMenuEntry("move win 1", 420);
  glutAddMenuEntry("resize win 1", 421);
  glutAddMenuEntry("iconify win 1", 422);
  glutAddMenuEntry("show win 1", 423);
  glutAddMenuEntry("hide win 1", 424);
  glutAddMenuEntry("push win 1", 425);
  glutAddMenuEntry("pop win 1", 426);

/* Gfx modes */

  createMenu6();        /* build dynamically  */

/* Texty reports */

  menu7 = glutCreateMenu(menuFunc);
  glutAddMenuEntry("report current win modes", 700);
  glutAddMenuEntry("report current device data", 701);
  glutAddMenuEntry("check OpenGL extensions", 702);
  glutAddMenuEntry("dump internal data (d)", 703);

/* Play with menus */

  menu8 = glutCreateMenu(menuFunc);
  glutAddMenuEntry("toggle menus on left button", 805);
  glutAddMenuEntry("toggle menus on middle button", 806);
  glutAddMenuEntry("toggle menus on right button", 807);
  glutAddMenuEntry("---------------------------", 9999);
  glutAddMenuEntry("add plain items", 800);
  glutAddMenuEntry("add submenu items", 801);
  glutAddMenuEntry("change new entries to plain items", 802);
  glutAddMenuEntry("change new entries to submenus", 803);
  glutAddMenuEntry("remove all new items", 804);
  glutAddMenuEntry("---------------------------", 9999);

/* Main menu */

  menu1 = glutCreateMenu(menuFunc);
  glutAddSubMenu("control", menu2);
  glutAddSubMenu("shapes", menu3);
  glutAddSubMenu("windows", menu4);
  glutAddSubMenu("window ops", menu5);
  glutAddSubMenu("gfx modes", menu6);
  glutAddSubMenu("reports", menu7);
  glutAddSubMenu("menus", menu8);
  glutAddMenuEntry("help (h)", 101);
  glutAddMenuEntry("quit (esc)", 100);
}

/* createMenu6 - Dynamically rebuild menu of display modes to
   show current choices */

void
createMenu6(void)
{
  char str[100];
  int i;

  if (menu6 != 0)
    glutDestroyMenu(menu6);
  menu6 = glutCreateMenu(menuFunc);

  for (i = 0; i < MODES; i++) {
    sprintf(str, "%srequest %s", (modes[i] ? "+ " : "   "), modeNames[i]);
    glutAddMenuEntry(str, 602 + i);
  }
}

/* menuFunc - Process return codes from popup menus */

void
menuFunc(int value)
{
  static int initItems = 10;
  int items, m;

  if (initItems == 0) {
    glutSetMenu(menu8);
    initItems = glutGet(GLUT_MENU_NUM_ITEMS);
  }
  PR("Menu returned value %d \n", value);

  switch (value) {

/* GLUT shapes */

  case 200:
  case 201:
  case 202:
  case 203:
  case 204:
  case 205:
  case 206:
  case 207:
  case 208:
    redefineShapes(value - 200);
    break;

/* Overall controls */

  case 300:
    menuFreeze = !menuFreeze;
    break;

  case 301:
    text[idToIndex(glutGetWindow())] = !(text[idToIndex(glutGetWindow())]);
    break;

  case 302:
    timerOn = !timerOn;
    if (timerOn)
      glutTimerFunc(DELAY, timerFunc, 1);
    break;

  case 303:
    animation = !animation;
    if (animation)
      glutIdleFunc(idleFunc);
    else
      glutIdleFunc(NULL);
    break;

  case 304:
    winFreeze[idToIndex(glutGetWindow())] = !(winFreeze[idToIndex(
          glutGetWindow())]);
    break;

  case 305:
    debug = !debug;
    break;

  case 307:
    backdrop = !backdrop;
    break;

  case 308:
    passive = !passive;
    if (passive)
      glutPassiveMotionFunc(passiveMotionFunc);
    else
      glutPassiveMotionFunc(NULL);
    break;

  case 310:
    lineWidth += 1;
    updateAll();
    break;

  case 311:
    lineWidth -= 1;
    if (lineWidth < 1)
      lineWidth = 1;
    updateAll();
    break;

  case 312:
    demoMode = !demoMode;
    if (demoMode)
      autoDemo(-2);
    break;

/* Window create/destroy. */

/* Creates */

  case 400:
    makeWindow(0);
    break;

  case 401:
    makeWindow(1);
    break;

  case 402:
    makeWindow(2);
    break;

  case 403:
    makeWindow(3);
    break;

  case 404:
    makeWindow(4);
    break;

  case 406:
    makeWindow(6);
    break;

  case 407:
    makeWindow(7);
    break;

/* Destroys */

  case 410:
    killWindow(0);
    break;

  case 411:
    killWindow(1);
    break;

  case 412:
    killWindow(2);
    break;

  case 413:
    killWindow(3);
    break;

  case 414:
    killWindow(4);
    break;

  case 416:
    killWindow(6);
    break;

  case 417:
    killWindow(7);
    break;

  case 450:
    makeAllWindows();
    break;

  case 451:
    killAllWindows();
    break;

/* Window movements etc. */

  case 420:
    positionWindow(1);
    break;

  case 421:
    reshapeWindow(1);
    break;

  case 422:
    iconifyWindow(1);
    break;

  case 423:
    showWindow(1);
    break;

  case 424:
    hideWindow(1);
    break;

  case 425:
    pushWindow(1);
    break;

  case 426:
    popWindow(1);
    break;

  case 430:
    positionWindow(idToIndex(glutGetWindow()));
    break;

  case 431:
    reshapeWindow(idToIndex(glutGetWindow()));
    break;

  case 432:
    iconifyWindow(idToIndex(glutGetWindow()));
    break;

  case 433:
    showWindow(idToIndex(glutGetWindow()));
    break;

  case 434:
    hideWindow(idToIndex(glutGetWindow()));
    break;

  case 435:
    pushWindow(idToIndex(glutGetWindow()));
    break;

  case 436:
    popWindow(idToIndex(glutGetWindow()));
    break;

/* Test gfx modes. */

  case 600:
    makeWindow(3);
    break;

  case 601:
    killWindow(3);
    break;

  case 602:
  case 603:
  case 604:
  case 605:
  case 606:
  case 607:
  case 608:
  case 609:
  case 610:
  case 611:
    modes[value - 602] = !modes[value - 602];
    setInitDisplayMode();
    break;

/* Text reports */

/* This is pretty ugly. */

#define INDENT 30
#define REPORTSTART(text)                          \
        printf("\n" text "\n");                    \
        textPtr[0] = (char *)malloc(strlen(text)+1); \
	strcpy(textPtr[0], text);                  \
        textCount = 1;

#define REPORTEND                                  \
        scrollLine = 0;                            \
        textPtr[textCount] = NULL;                 \
        makeWindow(8);                             \
        updateText();

#define GLUTGET(name)                              \
       {                                           \
          char str[100], str2[100];                \
          int s, len;                              \
          sprintf(str, # name);                    \
          len = (int) strlen(# name);              \
          for(s = 0 ; s < INDENT-len; s++)         \
            strcat(str, " ");                      \
          sprintf(str2, ": %d\n",glutGet(name));   \
	  strcat(str, str2);                       \
	  printf(str);                             \
	  textPtr[textCount] = stralloc(str);      \
 	  textCount++;                             \
       }

  case 700:

    printf("XXXXXX glutGetWindow = %d\n", glutGetWindow());
    REPORTSTART("glutGet():");

    GLUTGET(GLUT_WINDOW_X);
    GLUTGET(GLUT_WINDOW_Y);
    GLUTGET(GLUT_WINDOW_WIDTH);
    GLUTGET(GLUT_WINDOW_HEIGHT);
    GLUTGET(GLUT_WINDOW_BUFFER_SIZE);
    GLUTGET(GLUT_WINDOW_STENCIL_SIZE);
    GLUTGET(GLUT_WINDOW_DEPTH_SIZE);
    GLUTGET(GLUT_WINDOW_RED_SIZE);
    GLUTGET(GLUT_WINDOW_GREEN_SIZE);
    GLUTGET(GLUT_WINDOW_BLUE_SIZE);
    GLUTGET(GLUT_WINDOW_ALPHA_SIZE);
    GLUTGET(GLUT_WINDOW_ACCUM_RED_SIZE);
    GLUTGET(GLUT_WINDOW_ACCUM_GREEN_SIZE);
    GLUTGET(GLUT_WINDOW_ACCUM_BLUE_SIZE);
    GLUTGET(GLUT_WINDOW_ACCUM_ALPHA_SIZE);
    GLUTGET(GLUT_WINDOW_DOUBLEBUFFER);
    GLUTGET(GLUT_WINDOW_RGBA);
    GLUTGET(GLUT_WINDOW_PARENT);
    GLUTGET(GLUT_WINDOW_NUM_CHILDREN);
    GLUTGET(GLUT_WINDOW_COLORMAP_SIZE);
    GLUTGET(GLUT_WINDOW_NUM_SAMPLES);
    GLUTGET(GLUT_STEREO);
    GLUTGET(GLUT_SCREEN_WIDTH);
    GLUTGET(GLUT_SCREEN_HEIGHT);
    GLUTGET(GLUT_SCREEN_HEIGHT_MM);
    GLUTGET(GLUT_SCREEN_WIDTH_MM);
    GLUTGET(GLUT_MENU_NUM_ITEMS);
    GLUTGET(GLUT_DISPLAY_MODE_POSSIBLE);
    GLUTGET(GLUT_INIT_DISPLAY_MODE);
    GLUTGET(GLUT_INIT_WINDOW_X);
    GLUTGET(GLUT_INIT_WINDOW_Y);
    GLUTGET(GLUT_INIT_WINDOW_WIDTH);
    GLUTGET(GLUT_INIT_WINDOW_HEIGHT);
    GLUTGET(GLUT_ELAPSED_TIME);

    REPORTEND;
    break;

#define GLUTDEVGET(name)                         \
        {                                        \
          char str[100], str2[100];              \
          int len, s;                            \
          sprintf(str, # name);                  \
          len = (int) strlen(# name);            \
          for(s = 0 ; s < INDENT-len; s++)       \
            strcat(str, " ");                    \
          sprintf(str2, ": %d\n",                \
	     glutDeviceGet(name));               \
	  strcat(str, str2);                     \
	  printf(str);                           \
	  textPtr[textCount] = stralloc(str);    \
 	  textCount++;                           \
	}

  case 701:
    REPORTSTART("glutDeviceGet():");

    GLUTDEVGET(GLUT_HAS_KEYBOARD);
    GLUTDEVGET(GLUT_HAS_MOUSE);
    GLUTDEVGET(GLUT_HAS_SPACEBALL);
    GLUTDEVGET(GLUT_HAS_DIAL_AND_BUTTON_BOX);
    GLUTDEVGET(GLUT_HAS_TABLET);
    GLUTDEVGET(GLUT_NUM_MOUSE_BUTTONS);
    GLUTDEVGET(GLUT_NUM_SPACEBALL_BUTTONS);
    GLUTDEVGET(GLUT_NUM_BUTTON_BOX_BUTTONS);
    GLUTDEVGET(GLUT_NUM_DIALS);
    GLUTDEVGET(GLUT_NUM_TABLET_BUTTONS);

    REPORTEND;
    break;

#define EXTCHECK(name)                           \
        {                                        \
          char str[100], str2[100];              \
          int len, s;                            \
          sprintf(str, # name);                  \
          len = (int) strlen(# name);            \
          for(s = 0 ; s < INDENT-len; s++)       \
            strcat(str, " ");                    \
          sprintf(str2, ": %s\n",                \
	     glutExtensionSupported(# name)?     \
	       "yes": "no");                     \
	  strcat(str, str2);                     \
	  printf(str);                           \
	  textPtr[textCount] = stralloc(str);    \
 	  textCount++;                           \
        }

  case 702:
    REPORTSTART("glutExtensionSupported():");

    EXTCHECK(GL_EXT_abgr);
    EXTCHECK(GL_EXT_blend_color);
    EXTCHECK(GL_EXT_blend_minmax);
    EXTCHECK(GL_EXT_blend_logic_op);
    EXTCHECK(GL_EXT_blend_subtract);
    EXTCHECK(GL_EXT_polygon_offset);
    EXTCHECK(GL_EXT_texture);
    EXTCHECK(GL_EXT_guaranteed_to_fail);
    EXTCHECK(GLX_SGI_swap_control);
    EXTCHECK(GLX_SGI_video_sync);
    EXTCHECK(GLX_SGIS_multi_sample);

    REPORTEND;
    break;

  case 703:
    dumpIds();
    break;

/* Mess around with menus */

  case 800:
    if (glutGetMenu() != menu8)  /* just a test  */
      printf("glutGetMenu() returned unexpected value\n");
    glutAddMenuEntry("help", 101);
    glutAddMenuEntry("help", 101);
    glutAddMenuEntry("help", 101);
    glutAddMenuEntry("help", 101);
    glutAddMenuEntry("help", 101);
    break;

  case 801:
    glutAddSubMenu("shapes", menu3);
    glutAddSubMenu("shapes", menu3);
    glutAddSubMenu("shapes (a long string to break menus with)", menu3);
    glutAddSubMenu("shapes", menu3);
    glutAddSubMenu("shapes", menu3);
    break;

  case 802:
    items = glutGet(GLUT_MENU_NUM_ITEMS);
    for (m = initItems + 1; m <= items; m++) {
      glutChangeToMenuEntry(m, "help", 101);
    }
    break;

  case 803:
    items = glutGet(GLUT_MENU_NUM_ITEMS);
    for (m = initItems + 1; m <= items; m++) {
      glutChangeToSubMenu(m, "shapes", menu3);
    }
    break;

  case 804:
    items = glutGet(GLUT_MENU_NUM_ITEMS);
    /* reverse order so renumbering not aproblem  */
    for (m = items; m >= initItems + 1; m--) {
      glutRemoveMenuItem(m);
    }
    break;

  case 805:
    menuButton[0] = !menuButton[0];
    attachMenus();
    break;

  case 806:
    menuButton[1] = !menuButton[1];
    attachMenus();
    break;

  case 807:
    menuButton[2] = !menuButton[2];
    attachMenus();
    break;

/* Direct menu items.  */

  case 100:
    exit(0);
    break;

  case 101:
    if (winId[5] == 0)
      makeWindow(5);
    else
      killWindow(5);
    break;

  case 9999:
    break;

  default:
    fprintf(stderr, "\007Unhandled case %d in menu callback\n", value);
  }

}

/* redefineShapes - Remake the shapes display lists */

void
redefineShapes(int shape)
{
  int i;

#define C3                \
   	 switch(i)        \
	 {                \
	     case 0:      \
	     case 3:      \
	       C1;        \
	       break;     \
	                  \
	     case 1:      \
	     case 2:      \
	     case 4:      \
	     case 6:      \
	     case 7:      \
	       C2;        \
	       break;     \
	 }                \
	 currentShape = shape

  for (i = 0; i < MAXWIN; i++) {
    if (winId[i]) {
      glutSetWindow(winId[i]);
      if (glIsList(i + 1))
        glDeleteLists(i + 1, 1);
      glNewList(i + 1, GL_COMPILE);

      switch (shape) {

#undef  C1
#define C1  glutSolidSphere(1.5, 10, 10)
#undef  C2
#define C2  glutWireSphere(1.5, 10, 10)

      case 0:
        C3;
        break;

#undef  C1
#define C1 glutSolidCube(2)
#undef  C2
#define C2 glutWireCube(2)

      case 1:
        C3;
        break;

#undef  C1
#define C1 glutSolidCone(1.5, 1.75, 10, 10);
#undef  C2
#define C2 glutWireCone(1.5, 1.75, 10, 10);

      case 2:
        C3;
        break;

#undef  C1
#define C1 glutSolidTorus(0.5, 1.1, 10, 10)
#undef  C2
#define C2 glutWireTorus(0.5, 1.1, 10, 10)

      case 3:
        C3;
        break;

#undef  C1
#define C1 glScalef(.8, .8, .8);glutSolidDodecahedron()
#undef  C2
#define C2 glScalef(.8, .8, .8);glutWireDodecahedron()

      case 4:
        C3;
        break;

#undef  C1
#define C1 glScalef(1.5, 1.5, 1.5);glutSolidOctahedron()
#undef  C2
#define C2 glScalef(1.5, 1.5, 1.5);glutWireOctahedron()

      case 5:
        C3;
        break;

#undef  C1
#define C1 glScalef(1.8, 1.8, 1.8);glutSolidTetrahedron()
#undef  C2
#define C2 glScalef(1.8, 1.8, 1.8);glutWireTetrahedron()

      case 6:
        C3;
        break;

#undef  C1
#define C1 glScalef(1.5, 1.5, 1.5);glutSolidIcosahedron()
#undef  C2
#define C2 glScalef(1.5, 1.5, 1.5);glutWireIcosahedron()

      case 7:
        C3;
        break;

#undef  C1
#define C1 glutSolidTeapot(1.5);
#undef  C2
#define C2 glutWireTeapot(1.5);

      case 8:
        C3;
        break;
      }
      glEndList();
    }
  }
}

/* positionWindow - Shift a window */

void
positionWindow(int index)
{
  int x, y;

  if (winId[index] == 0)
    return;

  glutSetWindow(winId[index]);
  x = glutGet(GLUT_WINDOW_X);
  y = glutGet(GLUT_WINDOW_Y);
  glutPositionWindow(x + 50, y + 50);
}

/* reshapeWindow - Change window size a little */

void
reshapeWindow(int index)
{
  int x, y;

  if (winId[index] == 0)
    return;
  glutSetWindow(winId[index]);
  x = glutGet(GLUT_WINDOW_WIDTH);
  y = glutGet(GLUT_WINDOW_HEIGHT);
/* glutReshapeWindow(x * (index % 2? 0.8: 1.2), y * (index % 2? 

   1.2: 0.8)); */
  glutReshapeWindow((int) (x * 1.0), (int) (y * 0.8));
}

/* iconifyWindow - Iconify a window */

void
iconifyWindow(int index)
{
  if (winId[index] == 0)
    return;
  glutSetWindow(winId[index]);
  glutIconifyWindow();
}

/* showWindow - Show a window (map or uniconify it) */

void
showWindow(int index)
{
  if (winId[index] == 0)
    return;
  glutSetWindow(winId[index]);
  glutShowWindow();
}

/* hideWindow - Hide a window (unmap it) */

void
hideWindow(int index)
{
  if (winId[index] == 0)
    return;
  glutSetWindow(winId[index]);
  glutHideWindow();
}

/* pushWindow - Push a window */

void
pushWindow(int index)
{
  if (winId[index] == 0)
    return;
  glutSetWindow(winId[index]);
  glutPushWindow();
}

/* popWindow - Pop a window */

void
popWindow(int index)
{
  if (winId[index] == 0)
    return;
  glutSetWindow(winId[index]);
  glutPopWindow();
}

/* drawScene - Draw callback, triggered by expose events etc.
   in GLUT. */

void
drawScene(void)
{
  int winIndex;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  winIndex = idToIndex(glutGetWindow());
  /* printf("drawScene for index %d, id %d\n", winIndex,
     glutGetWindow());  */

  glPushMatrix();
  glLineWidth(lineWidth);
  if (backdrop)
    glCallList(100);

  /* Left button spinning  */

  trackBall(APPLY, 0, 0, 0, 0);

  /* Apply continuous spinning  */

  glRotatef(angle, 0, 1, 0);

  glCallList(winIndex + 1);
  glPopMatrix();

  if (text[winIndex])
    showText();

  glutSwapBuffers();
}

/* showText - Render some text in the current GLUT window */

void
showText(void)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, 100, 0, 100);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor3f(1.0, 1.0, 1.0);
  glIndexi(7);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glLineWidth(lineWidth);

  textString(1, 1, "GLUT_BITMAP_8_BY_13", GLUT_BITMAP_8_BY_13);
  textString(1, 5, "GLUT_BITMAP_9_BY_15", GLUT_BITMAP_9_BY_15);
  textString(1, 10, "GLUT_BITMAP_TIMES_ROMAN_10", GLUT_BITMAP_TIMES_ROMAN_10);
  textString(1, 15, "GLUT_BITMAP_TIMES_ROMAN_24", GLUT_BITMAP_TIMES_ROMAN_24);

  strokeString(1, 25, "GLUT_STROKE_ROMAN", GLUT_STROKE_ROMAN);
  strokeString(1, 35, "GLUT_STROKE_MONO_ROMAN", GLUT_STROKE_MONO_ROMAN);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

}

/* textString - Bitmap font string */

void
textString(int x, int y, char *msg, void *font)
{
  glRasterPos2f(x, y);
  while (*msg) {
    glutBitmapCharacter(font, *msg);
    msg++;
  }
}

/* strokeString - Stroke font string */

void
strokeString(int x, int y, char *msg, void *font)
{
  glPushMatrix();
  glTranslatef(x, y, 0);
  glScalef(.04, .04, .04);
  while (*msg) {
    glutStrokeCharacter(font, *msg);
    msg++;
  }
  glPopMatrix();
}

/* idleFunc - GLUT idle func callback - animates windows */

void
idleFunc(void)
{
  int i;

  if (!leftDown && !middleDown)
    angle += 1;
    angle = angle % 360;

  for (i = 0; i < MAXWIN; i++) {
    if (winId[i] && winVis[i] && !winFreeze[i]) {
      glutSetWindow(winId[i]);
      glutPostRedisplay();
    }
  }
}

/* reshapeFunc - Reshape callback. */

void
reshapeFunc(int width, int height)
{
  int winId;
  float aspect;

  winId = glutGetWindow();
  PR("reshape callback for window id %d \n", winId);

  glViewport(0, 0, width, height);
  aspect = (float) width / height;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, aspect, 1.0, 20.0);
  glMatrixMode(GL_MODELVIEW);
}

/* visible - Visibility callback. Turn off rendering in
   invisible windows */

void
visible(int state)
{
  int winId;
  static GLboolean someVisible = GL_TRUE;

  winId = glutGetWindow();
  /* printf("visible: state = %d \n", state);  */

  if (state == GLUT_VISIBLE) {
    PR("Window id %d visible \n", winId);
    winVis[idToIndex(winId)] = GL_TRUE;
  } else {
    PR("Window %d not visible \n", winId);
    winVis[idToIndex(winId)] = GL_FALSE;
  }

  if ((winVis[0] == GL_FALSE) && (winVis[1] == GL_FALSE) && (winVis[2] == GL_FALSE)
    && (winVis[3] == GL_FALSE) && (winVis[6] == GL_FALSE) && (winVis[7] ==
      GL_FALSE)) {
    glutIdleFunc(NULL);
    PR("All windows not visible; idle func disabled\n");
    someVisible = GL_FALSE;
  } else {
    if (!someVisible) {
      PR("Some windows now visible; idle func enabled\n");
      someVisible = GL_TRUE;
      if (animation)
        glutIdleFunc(idleFunc);
    }
  }
}

/* keyFunc - Ascii key callback */

/* ARGSUSED1 */
void
keyFunc(unsigned char key, int x, int y)
{
  int i, ii;

  if (debug || showKeys)
    printf("Ascii key '%c' 0x%02x\n", key, key);

  switch (key) {
  case 0x1b:
    exit(0);
    break;

  case 'a':
    demoMode = !demoMode;
    if (demoMode)
      autoDemo(-2);
    break;

  case 's':
    AUTODELAY = AUTODELAY * 0.666;
    break;

  case 'S':
    AUTODELAY = AUTODELAY * 1.5;
    break;

  case 'q':
    killWindow(idToIndex(glutGetWindow()));
    break;

  case 'k':
    showKeys = !showKeys;
    break;

  case 'p':
    demoMode = !demoMode;
    if (demoMode)
      autoDemo(-999);
    break;

  case 'D':
    debug = !debug;
    break;

  case 'd':
    dumpIds();
    break;

  case 'h':
    if (winId[5] == 0)
      makeWindow(5);
    else
      killWindow(5);
    break;

  case 't':
    ii = idToIndex(glutGetWindow());
    text[ii] = !text[ii];
    break;

  case 'r':
    trackBall(RESET, 0, 0, 0, 0);
    break;

  case 'l':
    lineWidth += 1;
    updateAll();
    break;

  case 'L':
    lineWidth -= 1;
    if (lineWidth < 1)
      lineWidth = 1;
    updateAll();
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '6':
    i = key - '0';
    winVis[i] = !winVis[i];
    break;

  case ')':
    makeWindow(0);
    break;

  case '!':
    makeWindow(1);
    break;

  case '@':
    makeWindow(2);
    break;

  case '#':
    makeWindow(3);
    break;

  }
}

/* specialFunc - Special keys callback (F keys, cursor keys
   etc. */

/* ARGSUSED1 */
void
specialFunc(int key, int x, int y)
{
  if (debug || showKeys)
    printf("Special key %d\n", key);

  switch (key) {
  case GLUT_KEY_PAGE_DOWN:
    scrollLine += 10;
    updateHelp();
    updateText();
    break;

  case GLUT_KEY_PAGE_UP:
    scrollLine -= 10;
    updateHelp();
    updateText();
    break;

  case GLUT_KEY_DOWN:
    scrollLine += 1;
    updateHelp();
    updateText();
    break;

  case GLUT_KEY_UP:
    scrollLine -= 1;
    updateHelp();
    updateText();
    break;

  case GLUT_KEY_HOME:
    scrollLine = 0;
    updateHelp();
    updateText();
    break;

  case GLUT_KEY_END:
    scrollLine = 9999;
    updateHelp();
    updateText();
    break;

  case GLUT_KEY_RIGHT:
    scrollCol -= 1;
    updateHelp();
    updateText();
    break;

  case GLUT_KEY_LEFT:
    scrollCol += 1;
    updateHelp();
    updateText();
    break;
  }
}

/* mouseFunc - Mouse button callback */

void
mouseFunc(int button, int state, int x, int y)
{
  PR("Mouse button %d, state %d, at pos %d, %d\n", button, state, x, y);

  trackBall(MOUSEBUTTON, button, state, x, y);
}

/* motionFunc - Mouse movement (with a button down) callback */

void
motionFunc(int x, int y)
{
  PR("Mouse motion at %d, %d\n", x, y);

  trackBall(MOUSEMOTION, 0, 0, x, y);

  glutPostRedisplay();
}

/* passiveMotionFunc - Mouse movement (with no button down)
   callback */

void
passiveMotionFunc(int x, int y)
{
  printf("Mouse motion at %d, %d\n", x, y);
}

/* entryFunc - Window entry event callback */

void
entryFunc(int state)
{
  int winId = glutGetWindow();
  PR("Entry event: window id %d (index %d), state %d \n", winId, idToIndex(
      winId), state);
}

/* menuStateFunc - Callback to tell us when menus are popped
   up/down. */

int menu_state = GLUT_MENU_NOT_IN_USE;

void
menuStateFunc(int state)
{
  printf("menu stated = %d\n", state);
  menu_state = state;

  if (glutGetWindow() == 0) {
    PR("menuStateFunc: window invalid\n");
    return;
  }
  PR("Menus are%sin use\n", state == GLUT_MENU_IN_USE ? " " : " not ");

  if ((state == GLUT_MENU_IN_USE) && menuFreeze)
    glutIdleFunc(NULL);
  else if (animation)
    glutIdleFunc(idleFunc);
}

/* timerFunc - General test of global timer */

void
timerFunc(int value)
{
  printf("timer callback: value %d\n", value);
  if (timerOn) {
    glutTimerFunc(DELAY, timerFunc, 1);
  }
}

#if 0
/* delayedReinstateMenuStateCallback - Hack to reinstate
   MenuStateCallback after a while.  */

void
delayedReinstateMenuStateCallback(int state)
{
  glutMenuStateFunc(menuStateFunc);
}

#endif

/* setInitDisplayMode - update display modes from display mode
   menu */

void
setInitDisplayMode(void)
{
  int i;

  displayMode = 0;

  for (i = 0; i < MODES; i++) {
    if (modes[i]) {
      /* printf("Requesting %s \n", modeNames[i]);  */
      displayMode |= glutMode[i];
    }
  }

  glutInitDisplayMode(displayMode);

  createMenu6();
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE))
    warning("This display mode not supported\n");
}

/* makeWindow - Create one of the windows */

void
makeWindow(int index)
{
  char str[99];

  if (winId[index] != 0) {
    /* warning("Attempt to create window which is already
       created");  */
    return;
  }
  switch (index) {

  case 0:              /* ordinary RGB windows  */
  case 1:
  case 2:
  case 3:

    setInitDisplayMode();
    glutInitWindowPosition(pos[index][0], pos[index][1]);
    glutInitWindowSize(size[index][0], size[index][1]);
    winId[index] = glutCreateWindow(" ");
    PR("Window %d id = %d \n", index, winId[index]);
    gfxInit(index);

    addCallbacks();

    sprintf(str, "window %d (RGB)", index);
    glutSetWindowTitle(str);
    sprintf(str, "icon %d", index);
    glutSetIconTitle(str);
    glutSetMenu(menu1);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    break;

  case 4:              /* subwindow  */

    setInitDisplayMode();
    winId[index] = glutCreateSubWindow(winId[0], pos[index][0], pos[index]
      [1], size[index][0], size[index][1]);
    PR("Window %d id = %d \n", index, winId[index]);
    gfxInit(index);
    glutDisplayFunc(drawScene);
    glutVisibilityFunc(visible);
    glutReshapeFunc(reshapeFunc);

    break;

  case 5:              /* help window  */
  case 8:              /* text window  */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(pos[index][0], pos[index][1]);
    glutInitWindowSize(size[index][0], size[index][1]);
    winId[index] = glutCreateWindow(" ");
    PR("Window %d id = %d \n", index, winId[index]);

    /* addCallbacks();  */
    glutKeyboardFunc(keyFunc);
    glutSpecialFunc(specialFunc);

    glClearColor(0.15, 0.15, 0.15, 1.0);
    glColor3f(1, 1, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 300, 0, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (index == 5) {
      glutDisplayFunc(updateHelp);
      glutSetWindowTitle("help (RGB) win 5");
      glutSetIconTitle("help");
    } else {
      glutDisplayFunc(updateText);
      glutSetWindowTitle("text (RGB) win 8");
      glutSetIconTitle("text");
    }
    glutSetMenu(menu1);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    break;

  case 6:              /* color index window  */
  case 7:              /* color index window  */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_INDEX | GLUT_DEPTH);
	 if (glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
		 glutInitWindowPosition(pos[index][0], pos[index][1]);
		 glutInitWindowSize(size[index][0], size[index][1]);
		 winId[index] = glutCreateWindow(" ");
		 PR("Window %d id = %d \n", index, winId[index]);

		 gfxInit(index);

		 addCallbacks();

		 sprintf(str, "window %d (color index)", index);
		 glutSetWindowTitle(str);
		 sprintf(str, "icon %d", index);
		 glutSetIconTitle(str);
		 glutSetMenu(menu1);
		 glutAttachMenu(GLUT_RIGHT_BUTTON);
	 }
    break;

  }
}

/* killWindow - Kill one of the main windows */

void
killWindow(int index)
{
  int i;

  if (winId[index] == 0) {
    /* fprintf(stderr, "Attempt to kill invalid window in
       killWindow\n");  */
    return;
  }
  PR("Killing win %d\n", index);
  glutSetWindow(winId[index]);

  /* Disable all callbacks for safety, although
     glutDestroyWindow  should do this.  */

  removeCallbacks();

  glutDestroyWindow(winId[index]);
  winId[index] = 0;
  winVis[index] = GL_FALSE;

#if 0
  /* If we reinstate the menu state func here, prog breaks.  So
     reinstate it a little later.  */
  glutTimerFunc(MENUDELAY, delayedReinstateMenuStateCallback, 1);
#endif

  if (index == 5) {     /* help  */
    scrollLine = 0;
    scrollCol = 0;
  }
  if (index == 8) {     /* text window  */
    for (i = 0; textPtr[i] != NULL; i++) {
      free(textPtr[i]); /* free the text strings  */
      textPtr[i] = NULL;
    }
  }
}

/* addCallbacks - Add some standard callbacks after creating a
   window */

void
addCallbacks(void)
{
  glutDisplayFunc(drawScene);
  glutVisibilityFunc(visible);
  glutReshapeFunc(reshapeFunc);
  glutKeyboardFunc(keyFunc);
  glutSpecialFunc(specialFunc);
  glutMouseFunc(mouseFunc);
  glutMotionFunc(motionFunc);
  glutEntryFunc(entryFunc);

/* Callbacks for exotic input devices. Must get my dials &
   buttons back. */

  glutSpaceballMotionFunc(spaceballMotionCB);
  glutSpaceballRotateFunc(spaceballRotateCB);
  glutSpaceballButtonFunc(spaceballButtonCB);

  glutButtonBoxFunc(buttonBoxCB);
  glutDialsFunc(dialsCB);

  glutTabletMotionFunc(tabletMotionCB);
  glutTabletButtonFunc(tabletButtonCB);
}

/* removeCallbacks - Remove all callbacks before destroying a
   window. GLUT probably does this  anyway but we'll be safe. */

void
removeCallbacks(void)
{
  glutVisibilityFunc(NULL);
  glutReshapeFunc(NULL);
  glutKeyboardFunc(NULL);
  glutSpecialFunc(NULL);
  glutMouseFunc(NULL);
  glutMotionFunc(NULL);
  glutEntryFunc(NULL);
}

/* updateHelp - Update the help window after user scrolls. */

void
updateHelp(void)
{
  static char *helpPtr[] =
  {
    "(Use PGUP, PGDN, HOME, END, arrows to scroll help text)          ",
    "                                                                ",
    "A demo program for GLUT.                                        ",
    "G Edwards, Aug 95                                               ",
    "Exercises 99% of GLUT calls                                     ",
    VERSIONLONG,
    "                                                                ",
    "This text uses GLUT_STROKE_MONO_ROMAN font, a built-in vector font.",
    "(Try resizing the help window).                                 ",
    "                                                                ",
    "Keys:                                                           ",
    " esc   quit                                                     ",
    " t     toggle text on/off in each window                        ",
    " h     toggle help                                              ",
    " q     quit current window                                      ",
    " a     auto demo                                                ",
    " p     pause/unpause demo                                       ",
    " l     increase line width (gfx & stroke text)                  ",
    " L     decrease line width (gfx & stroke text)                  ",
    " r     reset transforms                                         ",
    " k     show keyboard events                                     ",
    " D     show all events                                          ",
    "                                                                ",
    "Mouse:                                                          ",
    " Left button:    rotate                                         ",
    " Middle button:  pan                                            ",
    " Left + middle:  zoom                                           ",
    NULL};

  updateScrollWindow(5, helpPtr);
}

/* updateText - Update a text window */

void
updateText(void)
{
  int i;

  if (textPtr[0] == NULL) {
    for (i = 0; i < 20; i++) {
      textPtr[i] = (char *) malloc(50);
      strcpy(textPtr[i], "no current text");
    }
    textPtr[20] = NULL;
  }
  updateScrollWindow(8, textPtr);
}

/* updateScrollWindow */

void
updateScrollWindow(int index, char **ptr)
{
  int i, j, lines = 0;

  if (winId[index] == 0)
    return;

  glutSetWindow(winId[index]);

  for (i = 0; ptr[i] != NULL; i++)
    lines++;

  if (scrollLine < 0)
    scrollLine = 0;
  if (scrollLine > (lines - 5))
    scrollLine = lines - 5;

  glClear(GL_COLOR_BUFFER_BIT);

  glLineWidth(lineWidth);

  for (i = scrollLine, j = 1; ptr[i] != NULL; i++, j++)
    strokeString(scrollCol * 50, 100 - j * 6, ptr[i],
      GLUT_STROKE_MONO_ROMAN);

  glutSwapBuffers();

}

/* updateAll - Update all visible windows after soem global
   change, eg. line width */

void
updateAll(void)
{
  int i;

  if (winId[5] != 0)
    updateHelp();

  if (winId[8] != 0)
    updateText();

  for (i = 0; i < MAXWIN; i++)
    if (winId[i]) {
      glutSetWindow(winId[i]);
      glutPostRedisplay();
    }
}

/* idToIndex - Convert GLUT window id to our internal index */

int
idToIndex(int id)
{
  int i;
  for (i = 0; i < MAXWIN; i++) {
    if (winId[i] == id)
      return i;
  }
  fprintf(stderr, "error: id %d not found \n", id);
  return (-1);
}

/* warning - warning messages */

void
warning(char *msg)
{
  fprintf(stderr, "\007");

  if (debug) {
    fprintf(stderr, "%s", msg);
    if (msg[strlen(msg)] != '\n')
      fprintf(stderr, "%s", "\n");
  }
}

/* dumpIds - Debug: dump some internal data  */

void
dumpIds(void)
{
  int i, j;

  printf("\nInternal data:\n");

  for (i = 0; i < MAXWIN; i++)
    printf("Index %d, glut win id %d, visibility %d\n", i, winId[i],
      winVis[i]);

  for (i = 0; i < MAXWIN; i++) {
    if (winId[i])
      glutSetWindow(winId[i]);
    else {
      printf("index %d - no glut window\n", i);
      continue;
    }

    for (j = 1; j <= MAXWIN; j++)
      printf("Index %d, display list %d %s defined\n", i, j, glIsList(j) ?
        "is " : "not");
  }
}

/* autoDemo - Run auto demo/test This is a bit tricky. We need
   to start a timer sequence which progressively orders things
   to be done. The work really gets done when we return from
   our callback. Have to think about the event loop / callback
   design here. */

void
autoDemo(int value)
{

#define STEP(a, b)  \
    case a:         \
        action(a);  \
	glutTimerFunc(AUTODELAY * b, autoDemo, next(a); \
	break;

  static int index = 0;
  static int count = 0;
  static int restartValue = -2;

  if (value == -999)
    value = restartValue;

  restartValue = value;

#define AUTODELAY2 (unsigned int) (AUTODELAY*0.66)

  /* fprintf(stderr, "autoDemo: value %d \n", value);  */

  if (!demoMode)
    return;

  if (menu_state == GLUT_MENU_IN_USE) {
    glutTimerFunc(AUTODELAY / 2, autoDemo, value);
    return;
  }
  switch (value) {

/* Entry point; kill off existing windows. */

  case -2:
    killAllWindows();
    glutTimerFunc(AUTODELAY / 2, autoDemo, 1);
    break;

/* Start making windows */

  case -1:
    makeWindow(0);
    glutTimerFunc(AUTODELAY, autoDemo, 0);  /* skip case 0
                                               first time  */
    break;

/* Change shape & backdrop */

  case 0:
    currentShape = (currentShape + 1) % 9;
    redefineShapes(currentShape);
    count += 1;
    if (count % 2)
      backdrop = !backdrop;
    glutTimerFunc(AUTODELAY, autoDemo, 1);
    break;

/* Keep making windows */

  case 1:
    makeWindow(1);
    glutTimerFunc(AUTODELAY, autoDemo, 2);
    break;

  case 2:
    makeWindow(2);
    glutTimerFunc(AUTODELAY, autoDemo, 3);
    break;

  case 3:
    makeWindow(3);
    glutTimerFunc(AUTODELAY, autoDemo, 4);
    break;

  case 4:
    makeWindow(4);
    glutTimerFunc(AUTODELAY, autoDemo, 5);
    break;

  case 5:
    makeWindow(5);
    glutTimerFunc(AUTODELAY * 2, autoDemo, 51);
    break;

  case 51:
    makeWindow(6);
    glutTimerFunc(AUTODELAY * 2, autoDemo, 52);
    break;

  case 52:
    makeWindow(7);
    glutTimerFunc(AUTODELAY * 2, autoDemo, 53);
    break;

/* Kill last 3 windows, leave 4 up. */

  case 53:
    killWindow(7);
    glutTimerFunc(AUTODELAY, autoDemo, 54);
    break;

  case 54:
    killWindow(6);
    glutTimerFunc(AUTODELAY, autoDemo, 6);
    break;

  case 6:
    killWindow(5);
    glutTimerFunc(AUTODELAY, autoDemo, 7);
    break;

  case 7:
    killWindow(4);
    glutTimerFunc(AUTODELAY, autoDemo, 700);
    break;

/* Change shape again */

  case 700:
    currentShape = (currentShape + 1) % 9;
    redefineShapes(currentShape);
    glutTimerFunc(AUTODELAY, autoDemo, 701);
    break;

/* Cycle 4 main windows through various window ops.  */

  case 701:
    positionWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 701 : 702);
    break;

  case 702:
    reshapeWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 702 : 703);
    break;

  case 703:
    iconifyWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 703 : 704);
    break;

  case 704:
    showWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 704 : 705);
    break;

  case 705:
    hideWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 705 : 706);
    break;

  case 706:
    showWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 706 : 707);
    break;

  case 707:
    pushWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 707 : 708);
    break;

  case 708:
    popWindow(index);
    index = (index + 1) % 4;
    glutTimerFunc(AUTODELAY2, autoDemo, index > 0 ? 708 : 8);
    break;

/* Kill all windows */

  case 8:
    killWindow(3);
    glutTimerFunc(AUTODELAY, autoDemo, 9);
    break;

  case 9:
    killWindow(2);
    glutTimerFunc(AUTODELAY, autoDemo, 10);
    break;

  case 10:
    killWindow(1);
    glutTimerFunc(AUTODELAY, autoDemo, 11);
    break;

  case 11:
    killWindow(0);
    glutTimerFunc(AUTODELAY, autoDemo, -1);  /* back to start  */
    break;
  }

}

/* attachMenus - Attach/detach menus to/from mouse buttons */

void
attachMenus(void)
{
  int i, b;
  int button[3] =
  {GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON, GLUT_RIGHT_BUTTON};

  for (i = 0; i < MAXWIN; i++) {
    if (winId[i] != 0) {
      for (b = 0; b < 3; b++) {
        glutSetWindow(winId[i]);
        glutSetMenu(menu1);
        if (menuButton[b])
          glutAttachMenu(button[b]);
        else
          glutDetachMenu(button[b]);
      }
    }
  }
}

/* killAllWindows - Kill all windows (except 0) */

void
killAllWindows(void)
{
  int w;

  for (w = 1; w < MAXWIN; w++)
    if (winId[w])
      killWindow(w);
}

/* makeAllWindows - Make all windows */

void
makeAllWindows(void)
{
  int w;

  for (w = 0; w < MAXWIN; w++)
    if (!winId[w])
      makeWindow(w);
}

/* checkArgs - Check command line args */

void
checkArgs(int argc, char *argv[])
{
  int argp;
  GLboolean quit = GL_FALSE;
  GLboolean error = GL_FALSE;

#define AA argv[argp]

#if 0
#define NEXT argp++;      \
	    if(argp >= argc) \
	    {                \
	       Usage();      \
	       Exit(1);      \
	    }
#endif

  argp = 1;
  while (argp < argc) {
    if (match(AA, "-help")) {
      commandLineHelp();
      quit = GL_TRUE;
    } else if (match(AA, "-version")) {
      printf(VERSIONLONG "\n");
      quit = GL_TRUE;
    } else if (match(AA, "-auto")) {
      demoMode = GL_TRUE;
    } else if (match(AA, "-scale")) {
      argp++;
      scaleFactor = atof(argv[argp]);
    } else {
      fprintf(stderr, "Unknown arg: %s\n", AA);
      error = GL_TRUE;
      quit = GL_TRUE;
    }
    argp++;
  }

  if (error) {
    commandLineHelp();
    exit(1);
  }
  if (quit)
    exit(0);
}

/* commandLineHelp - Command line help */

void
commandLineHelp(void)
{
  printf("Usage:\n");
  printf(" -h[elp]            this stuff\n");
  printf(" -v[ersion]         show version\n");
  printf(" -a[uto]            start in auto demo mode\n");
  printf(" -s[cale] f         scale windows by f\n");
  printf("Standard GLUT args:\n");
  printf(" -iconic            start iconic\n");
  printf(" -display DISP      use display DISP\n");
  printf(" -direct            use direct rendering (default)\n");
  printf(" -indirect          use indirect rendering\n");
  printf(" -sync              use synchronous X protocol\n");
  printf(" -gldebug           check OpenGL errors\n");
  printf(" -geometry WxH+X+Y  standard X window spec (overridden here) \n");
}

/* match - Match a string (any unique substring). */

GLboolean
match(char *arg, char *t)
{
  if (strstr(t, arg))
    return GL_TRUE;
  else
    return GL_FALSE;
}

/* scaleWindows - Scale initial window sizes ansd positions */

void
scaleWindows(float scale)
{
  int i;

  for (i = 0; i < MAXWIN; i++) {
    pos[i][0] = pos[i][0] * scale;
    pos[i][1] = pos[i][1] * scale;
    size[i][0] = size[i][0] * scale;
    size[i][1] = size[i][1] * scale;
  }
}

/* trackBall - A simple trackball (not with proper rotations). */

/** A simple trackball with spin = left button
                           pan  = middle button
                           zoom = left + middle
   Doesn't have proper trackball rotation, ie axes which remain fixed in
   the scene. We should use the trackball code from 4Dgifts. */

#define STARTROTATE(x, y)     \
{                             \
    startMX = x;              \
    startMY = y;              \
}

#define STOPROTATE(x, y)      \
{                             \
    steadyXangle = varXangle; \
    steadyYangle = varYangle; \
}

#define STARTPAN(x, y)        \
{                             \
    startMX = x;              \
    startMY = y;              \
}

#define STOPPAN(x, y)         \
{                             \
    steadyX = varX;           \
    steadyY = varY;           \
}

#define STARTZOOM(x, y)       \
{                             \
    startMX = x;              \
    startMY = y;              \
}

#define STOPZOOM(x, y)        \
{                             \
    steadyZ = varZ;           \
}

static float
fixAngle(float angle)
{
  return angle - floor(angle / 360.0) * 360.0;
}

void
trackBall(int mode, int button, int state, int x, int y)
{
  static int startMX = 0, startMY = 0;  /* initial mouse pos  */
  static int deltaMX = 0, deltaMY = 0;  /* initial mouse pos  */
  static float steadyXangle = 0.0, steadyYangle = 0.0;
  static float varXangle = 0.0, varYangle = 0.0;
  static float steadyX = 0.0, steadyY = 0.0, steadyZ = 0.0;
  static float varX = 0.0, varY = 0.0, varZ = 0.0;

  switch (mode) {

  case RESET:
    steadyXangle = steadyYangle = steadyX = steadyY = steadyZ = 0.0;
    break;

  case MOUSEBUTTON:

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !middleDown) {
      STARTROTATE(x, y);
      leftDown = GL_TRUE;
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN &&
      middleDown) {
      STOPPAN(x, y);
      STARTZOOM(x, y);
      leftDown = GL_TRUE;
    } else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN &&
      !leftDown) {
      STARTPAN(x, y);
      middleDown = GL_TRUE;
    } else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN &&
      leftDown) {
      STOPROTATE(x, y);
      STARTZOOM(x, y);
      middleDown = GL_TRUE;
    } else if (state == GLUT_UP && button == GLUT_LEFT_BUTTON && !middleDown) {
      STOPROTATE(x, y);
      leftDown = GL_FALSE;
    } else if (state == GLUT_UP && button == GLUT_LEFT_BUTTON && middleDown) {
      STOPZOOM(x, y);
      STARTROTATE(x, y);
      leftDown = GL_FALSE;
    } else if (state == GLUT_UP && button == GLUT_MIDDLE_BUTTON && !leftDown) {
      STOPPAN(x, y);
      middleDown = GL_FALSE;
    } else if (state == GLUT_UP && button == GLUT_MIDDLE_BUTTON && leftDown) {
      STOPZOOM(x, y);
      STARTROTATE(x, y);
      middleDown = GL_FALSE;
    }
    break;

  case APPLY:

    if (leftDown && !middleDown) {
      glTranslatef(steadyX, steadyY, steadyZ);
      glRotatef(varXangle, 0, 1, 0);
      glRotatef(varYangle, 1, 0, 0);
    }
    /* Middle button pan  */

    else if (middleDown && !leftDown) {
      glTranslatef(varX, varY, steadyZ);
      glRotatef(steadyXangle, 0, 1, 0);
      glRotatef(steadyYangle, 1, 0, 0);
    }
    /* Left + middle zoom.  */

    else if (leftDown && middleDown) {
      glTranslatef(steadyX, steadyY, varZ);
      glRotatef(steadyXangle, 0, 1, 0);
      glRotatef(steadyYangle, 1, 0, 0);
    }
    /* Nothing down.  */

    else {
      glTranslatef(steadyX, steadyY, steadyZ);
      glRotatef(steadyXangle, 0, 1, 0);
      glRotatef(steadyYangle, 1, 0, 0);
    }
    break;

  case MOUSEMOTION:

    deltaMX = x - startMX;
    deltaMY = startMY - y;

    if (leftDown && !middleDown) {
      varXangle = fixAngle(steadyXangle + deltaMX);
      varYangle = fixAngle(steadyYangle + deltaMY);
    } else if (middleDown && !leftDown) {
      varX = steadyX + deltaMX / 100.0;
      varY = steadyY + deltaMY / 100.0;
    } else if (leftDown && middleDown) {
      varZ = steadyZ - deltaMY / 50.0;
    }
    break;
  }

}

/* Callbacks for exotic input devices. These have not been
   tested yet owing to the usual complete absence of such
   devices in the UK support group. */

/* spaceballMotionCB */

void
spaceballMotionCB(int x, int y, int z)
{
  printf("spaceballMotionCB: translations are X %d, Y %d, Z %d\n", x, y, z);
}

/* spaceballRotateCB */

void
spaceballRotateCB(int x, int y, int z)
{
  printf("spaceballRotateCB: rotations are X %d, Y %d, Z %d\n", x, y, z);
}

/* spaceballButtonCB */

void
spaceballButtonCB(int button, int state)
{
  printf("spaceballButtonCB: button %d, state %d\n", button, state);
}

/* buttonBoxCB */

void
buttonBoxCB(int button, int state)
{
  printf("buttonBoxCB: button %d, state %d\n", button, state);
}

/* dialsCB */

void
dialsCB(int dial, int value)
{
  printf("dialsCB: dial %d, value %d\n", dial, value);
}

/* tabletMotionCB */

void
tabletMotionCB(int x, int y)
{
  printf("tabletMotionCB: X %d, Y %d\n", x, y);
}

/* tabletButtonCB */

/* ARGSUSED2 */
void
tabletButtonCB(int button, int state, int dummy1, int dummy2)
{
  printf("tabletButtonCB: button %d, state %d\n", button, state);
}
