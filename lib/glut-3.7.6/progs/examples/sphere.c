
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Don't take this program too seriously.  It is just a hack. */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <GL/glut.h>

GLfloat light_diffuse[] =
{1.0, 0.0, 0.0, 1.0};
GLfloat light_position[] =
{1.0, 1.0, 1.0, 0.0};
GLUquadricObj *qobj;

int win1, win2, submenu1, submenu2;

int list = 1;

float thetime = 0.0;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (glutGetWindow() == win1) {
    glCallList(list);   /* render sphere display list */
  } else {
    glCallList(1);      /* render sphere display list */
  }
  glutSwapBuffers();
}

void
display_win1(void)
{
  glPushMatrix();
  glTranslatef(0.0, 0.0, -1 - 2 * sin(thetime));
  display();
  glPopMatrix();
}

void
idle(void)
{
  GLfloat light_position[] =
  {1.0, 1.0, 1.0, 0.0};

  glutSetWindow(win1);
  thetime += 0.05;
  light_position[1] = 1 + sin(thetime);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  display_win1();
}

/* ARGSUSED */
void
delayed_stop(int value)
{
  glutIdleFunc(NULL);
}

void
it(int value)
{
  glutDestroyWindow(glutGetWindow());
  printf("menu selection: win=%d, menu=%d\n", glutGetWindow(), glutGetMenu());
  switch (value) {
  case 1:
    if (list == 1) {
      list = 2;
    } else {
      list = 1;
    }
    break;
  case 2:
    exit(0);
    break;
  case 3:
    glutAddMenuEntry("new entry", value + 9);
    break;
  case 4:
    glutChangeToMenuEntry(1, "toggle it for drawing", 1);
    glutChangeToMenuEntry(3, "motion done", 3);
    glutIdleFunc(idle);
    break;
  case 5:
    glutIdleFunc(NULL);
    break;
  case 6:
    glutTimerFunc(2000, delayed_stop, 0);
    break;
  default:
    printf("value = %d\n", value);
  }
}

void
init(void)
{
  gluQuadricDrawStyle(qobj, GLU_FILL);
  glNewList(1, GL_COMPILE);  /* create sphere display list */
  gluSphere(qobj, /* radius */ 1.0, /* slices */ 20,  /* stacks 

                                                       */ 20);
  glEndList();
  gluQuadricDrawStyle(qobj, GLU_LINE);
  glNewList(2, GL_COMPILE);  /* create sphere display list */
  gluSphere(qobj, /* radius */ 1.0, /* slices */ 20,  /* stacks 

                                                       */ 20);
  glEndList();
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */
  glTranslatef(0.0, 0.0, -1.0);
}

void
menustate(int inuse)
{
  printf("menu is %s\n", inuse ? "INUSE" : "not in use");
  if (!inuse) {
  }
}

void
keyboard(unsigned char key, int x, int y)
{
  if (isprint(key)) {
    printf("key: `%c' %d,%d\n", key, x, y);
  } else {
    printf("key: 0x%x %d,%d\n", key, x, y);
  }
}

void
special(int key, int x, int y)
{
  char *name;

  switch (key) {
  case GLUT_KEY_F1:
    name = "F1";
    break;
  case GLUT_KEY_F2:
    name = "F2";
    break;
  case GLUT_KEY_F3:
    name = "F3";
    break;
  case GLUT_KEY_F4:
    name = "F4";
    break;
  case GLUT_KEY_F5:
    name = "F5";
    break;
  case GLUT_KEY_F6:
    name = "F6";
    break;
  case GLUT_KEY_F7:
    name = "F7";
    break;
  case GLUT_KEY_F8:
    name = "F8";
    break;
  case GLUT_KEY_F9:
    name = "F9";
    break;
  case GLUT_KEY_F10:
    name = "F11";
    break;
  case GLUT_KEY_F11:
    name = "F12";
    break;
  case GLUT_KEY_LEFT:
    name = "Left";
    break;
  case GLUT_KEY_UP:
    name = "Up";
    break;
  case GLUT_KEY_RIGHT:
    name = "Right";
    break;
  case GLUT_KEY_DOWN:
    name = "Down";
    break;
  case GLUT_KEY_PAGE_UP:
    name = "Page up";
    break;
  case GLUT_KEY_PAGE_DOWN:
    name = "Page down";
    break;
  case GLUT_KEY_HOME:
    name = "Home";
    break;
  case GLUT_KEY_END:
    name = "End";
    break;
  case GLUT_KEY_INSERT:
    name = "Insert";
    break;
  default:
    name = "UNKONW";
    break;
  }
  printf("special: %s %d,%d\n", name, x, y);
}

void
mouse(int button, int state, int x, int y)
{
  printf("button: %d %s %d,%d\n", button, state == GLUT_UP ? "UP" : "down", x, y);
}

void
motion(int x, int y)
{
  printf("motion: %d,%d\n", x, y);
}

void
visible(int status)
{
  printf("visible: %s\n", status == GLUT_VISIBLE ? "YES" : "no");
}

void
enter_leave(int state)
{
  printf("enter/leave %d = %s\n",
    glutGetWindow(),
    state == GLUT_LEFT ? "left" : "entered");
}

int
main(int argc, char **argv)
{
  qobj = gluNewQuadric();
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  win1 = glutCreateWindow("sphere");
  glutEntryFunc(enter_leave);
  init();
  glutDisplayFunc(display_win1);
  glutCreateMenu(it);
  glutAddMenuEntry("toggle draw mode", 1);
  glutAddMenuEntry("exit", 2);
  glutAddMenuEntry("new menu entry", 3);
  glutAddMenuEntry("motion", 4);
  glutAttachMenu(GLUT_LEFT_BUTTON);
  glutCreateMenu(it);
  glutAddMenuEntry("yes", 1);
  glutAddMenuEntry("no", 2);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  win2 = glutCreateWindow("second window");
  glutEntryFunc(enter_leave);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);
  glutMouseFunc(mouse);
#if 0
  glutMotionFunc(motion);
#endif
  glutVisibilityFunc(visible);
  init();
  light_diffuse[1] = 1;
  light_diffuse[2] = 1;
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glutDisplayFunc(display);
  submenu1 = glutCreateMenu(it);
  glutAddMenuEntry("submenu a", 666);
  glutAddMenuEntry("submenu b", 777);
  submenu2 = glutCreateMenu(it);
  glutAddMenuEntry("submenu 1", 25);
  glutAddMenuEntry("submenu 2", 26);
  glutAddSubMenu("submenuXXX", submenu1);
  glutCreateMenu(it);
  glutAddSubMenu("submenu", submenu2);
  glutAddMenuEntry("stop motion", 5);
  glutAddMenuEntry("delayed stop motion", 6);
  glutAddSubMenu("submenu", submenu2);
  glutAttachMenu(GLUT_LEFT_BUTTON);
  glutMenuStateFunc(menustate);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
