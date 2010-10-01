
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#ifndef WRAPPERS_ONLY

#include <glutint.h>

extern int __Argc;
extern char **__Argv;

static GLUTmenuStateFCB fortranMenuStateFunc;

void
glutnull_(void)
{
}

void
glutinit_(void)
{
  glutInit(&__Argc, __Argv);
}

static void
fortranMenuStateWrapper(int value)
{
  fortranMenuStateFunc(&value);
}

static void
fortranReshapeWrapper(int w, int h)
{
  (*__glutCurrentWindow->freshape) (&w, &h);
}

#if 0  /* XXX No IRIX joystick support for now. */
static void
fortranJoystickWrapper(unsigned int button, int x, int y, int z)
{
  (*__glutCurrentWindow->fjoystick) (&button, &x, &y, &z);
}
#endif

static void
fortranKeyboardWrapper(unsigned char ch, int x, int y)
{
  int chi = ch;

  (*__glutCurrentWindow->fkeyboard) (&chi, &x, &y);
}

static void
fortranKeyboardUpWrapper(unsigned char ch, int x, int y)
{
  int chi = ch;

  (*__glutCurrentWindow->fkeyboardUp) (&chi, &x, &y);
}

static void
fortranMouseWrapper(int btn, int state, int x, int y)
{
  (*__glutCurrentWindow->fmouse) (&btn, &state, &x, &y);
}

static void
fortranMotionWrapper(int x, int y)
{
  (*__glutCurrentWindow->fmotion) (&x, &y);
}

static void
fortranPassiveMotionWrapper(int x, int y)
{
  (*__glutCurrentWindow->fpassive) (&x, &y);
}

static void
fortranEntryWrapper(int state)
{
  (*__glutCurrentWindow->fentry) (&state);
}

static void
fortranVisibilityWrapper(int state)
{
  (*__glutCurrentWindow->fvisibility) (&state);
}

static void
fortranTimerWrapper(int value)
{
  /* Relies on special knowledge that __glutTimerList points to 
     the GLUTtimer* currently being processed! */
  (*__glutTimerList->ffunc) (&value);
}

static void
fortranSelectWrapper(int value)
{
  (*__glutItemSelected->menu->fselect) (&value);
}

static void
fortranSpecialWrapper(int key, int x, int y)
{
  (*__glutCurrentWindow->fspecial) (&key, &x, &y);
}

static void
fortranSpecialUpWrapper(int key, int x, int y)
{
  (*__glutCurrentWindow->fspecialUp) (&key, &x, &y);
}

static void
fortranSpaceballMotionWrapper(int x, int y, int z)
{
  (*__glutCurrentWindow->fspaceMotion) (&x, &y, &z);
}

static void
fortranSpaceballRotateWrapper(int x, int y, int z)
{
  (*__glutCurrentWindow->fspaceRotate) (&x, &y, &z);
}

static void
fortranSpaceballButtonWrapper(int button, int state)
{
  (*__glutCurrentWindow->fspaceButton) (&button, &state);
}

static void
fortranTabletMotionWrapper(int x, int y)
{
  (*__glutCurrentWindow->ftabletMotion) (&x, &y);
}

static void
fortranTabletButtonWrapper(int button, int state, int x, int y)
{
  (*__glutCurrentWindow->ftabletButton) (&button, &state, &x, &y);
}

static void
fortranDialsWrapper(int dial, int value)
{
  (*__glutCurrentWindow->fdials) (&dial, &value);
}

static void
fortranButtonBoxWrapper(int button, int state)
{
  (*__glutCurrentWindow->fbuttonBox) (&button, &state);
}

#endif /* WRAPPERS_ONLY */

#define glutfunc(Name,name,mixed,type) \
void \
glut##name##func(GLUT##type##FCB mixed) \
{ \
    if(mixed == (GLUT##type## FCB) glutnull_) { \
	glut##Name ## Func(NULL); \
    } else { \
	glut##Name##Func(fortran##Name##Wrapper); \
	__glutCurrentWindow->f##mixed = mixed; \
    } \
}

glutfunc(Reshape, reshape, reshape, reshape)
glutfunc(Keyboard, keyboard, keyboard, keyboard)
glutfunc(KeyboardUp, keyboardup, keyboardUp, keyboard)
glutfunc(Mouse, mouse, mouse, mouse)
glutfunc(Motion, motion, motion, motion)
glutfunc(Entry, entry, entry, entry)
glutfunc(Visibility, visibility, visibility, visibility)
glutfunc(Special, special, special, special)
glutfunc(SpecialUp, specialup, specialUp, special)
glutfunc(Dials, dials, dials, dials)
glutfunc(SpaceballMotion, spaceballmotion, spaceMotion, spaceMotion)
glutfunc(SpaceballRotate, spaceballrotate, spaceRotate, spaceRotate)
glutfunc(SpaceballButton, spaceballbutton, spaceButton, spaceButton)
glutfunc(PassiveMotion, passivemotion, passive, passive)
glutfunc(ButtonBox, buttonbox, buttonBox, buttonBox)
glutfunc(TabletMotion, tabletmotion, tabletMotion, tabletMotion)
glutfunc(TabletButton, tabletbutton, tabletButton, tabletButton)

/* Special callback cases. */

/* The display has no parameters passed so no need for wrapper. */
void
glutdisplayfunc(GLUTdisplayFCB display)
{
  glutDisplayFunc((GLUTdisplayCB) display);
}

int
glutcreatemenu(GLUTselectFCB select)
{
  int menu;

  menu = glutCreateMenu(fortranSelectWrapper);
  __glutCurrentMenu->fselect = select;
  return menu;
}

void
gluttimerfunc(unsigned long interval, GLUTtimerFCB timer, int value)
{
  glutTimerFunc((unsigned int) interval, fortranTimerWrapper, value);
  /* Relies on special __glutNewTimer variable side effect to
     establish Fortran timer func! */
  __glutNewTimer->ffunc = timer;
}

/* ARGSUSED */
void
glutjoystickfunc(GLUTjoystickFCB joystick, int pollInterval)
{
#if 0  /* XXX No IRIX joystick support for now. */
  if(joystick == (GLUTjoystickFCB) glutnull_) {
    glutJoystickFunc(NULL, pollInterval);
  } else {
    glutJoystickFunc(fortranJoystickWrapper, pollInterval);
    __glutCurrentWindow->fjoystick = joystick;
  }
#endif
}

void
glutidlefunc(GLUTidleFCB idleFunc)
{
  if (idleFunc == (GLUTidleFCB) glutnull_) {
    glutIdleFunc(NULL);
  } else {
    glutIdleFunc(idleFunc);
  }
}

void
glutmenustatefunc(GLUTmenuStateFCB menuStateFunc)
{
  if (menuStateFunc == (GLUTmenuStateFCB) glutnull_) {
    glutMenuStateFunc(NULL);
  } else {
    glutMenuStateFunc(fortranMenuStateWrapper);
    fortranMenuStateFunc = menuStateFunc;
  }
}

void
glutbitmapcharacter(int *font, int ch)
{
  /* 
   * mkf2c gets confused by double pointers and void* pointers.
   * So mkf2c does not complain, we consider the font handle to
   * be an int*.  But we really get an int** since Fortran
   * passes by reference.  So to "pedantically decode" the 
   * pointer, cast it first to int**, then dereference it,
   * then cast the result to a void*.
   */
  void *trueFont = (void *) *((int **) font);

  glutBitmapCharacter(trueFont, ch);
}

void
glutstrokecharacter(int *font, int ch)
{
  /* 
   * mkf2c gets confused by double pointers and void* pointers.
   * So mkf2c does not complain, we consider the font handle to
   * be an int*.  But we really get an int** since Fortran
   * passes by reference.  So to "pedantically decode" the 
   * pointer, cast it first to int**, then dereference it,
   * then cast the result to a void*.
   */
  void *trueFont = (void *) *((int **) font);

  glutStrokeCharacter(trueFont, ch);
}
