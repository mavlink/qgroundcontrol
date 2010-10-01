
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This tests GLUT's glutWindowStatusFunc rotuine. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

int win, subwin, cover;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glutSwapBuffers();
}

/* ARGSUSED */
void
time_end(int value)
{
  printf("PASS: test22\n");
  exit(0);
}

/* ARGSUSED */
void
time5(int value)
{
  glutSetWindow(subwin);
  glutPositionWindow(40, 40);
}

/* ARGSUSED */
void
time4(int value)
{
  glutSetWindow(subwin);
  glutShowWindow();
}

/* ARGSUSED */
void
time3(int value)
{
  glutSetWindow(subwin);
  glutHideWindow();
}

int sub_state = 0;
int cover_state = 0;
int state = 0;

void
coverstat(int status)
{
  if (cover != glutGetWindow()) {
    printf("ERROR: test22\n");
    exit(1);
  }
  printf("%d: cover = %d\n", cover_state, status);
  switch(cover_state) {
  case 0:
    if (status != GLUT_FULLY_RETAINED) {
      printf("ERROR: test22\n");
      exit(1);
    }
    glutTimerFunc(1000, time3, 0);
    break;
  default:
    printf("ERROR: test22\n");
    exit(1);
    break;
  }
  cover_state++;
}

/* ARGSUSED */
void
time2(int value)
{
  cover = glutCreateSubWindow(win, 5, 5, 105, 105);
  glClearColor(0.0, 1.0, 0.0, 0.0);
  glutDisplayFunc(display);
  glutWindowStatusFunc(coverstat);
}

void
substat(int status)
{
  if (subwin != glutGetWindow()) {
    printf("ERROR: test22\n");
    exit(1);
  }
  printf("%d: substat = %d\n", sub_state, status);
  switch(sub_state) {
  case 0:
    if (status != GLUT_FULLY_RETAINED) {
      printf("ERROR: test22\n");
      exit(1);
    }
    glutTimerFunc(1000, time2, 0);
    break;
  case 1:
    if (status != GLUT_FULLY_COVERED) {
      printf("ERROR: test22\n");
      exit(1);
    }
    break;
  case 2:
    if (status != GLUT_HIDDEN) {
      printf("ERROR: test22\n");
      exit(1);
    }
    glutTimerFunc(1000, time4, 0);
    break;
  case 3:
    if (status != GLUT_FULLY_COVERED) {
      printf("ERROR: test22\n");
      exit(1);
    }
    glutTimerFunc(1000, time5, 0);
    break;
  case 4:
    if (status != GLUT_PARTIALLY_RETAINED) {
      printf("ERROR: test22\n");
      exit(1);
    }
    glutTimerFunc(1000, time_end, 0);
    break;
  default:
    printf("ERROR: test22\n");
    exit(1);
    break;
  }
  sub_state++;
}

/* ARGSUSED */
void
time1(int value)
{
  subwin = glutCreateSubWindow(win, 10, 10, 100, 100);
  glClearColor(0.0, 1.0, 1.0, 0.0);
  glutDisplayFunc(display);
  glutWindowStatusFunc(substat);
}

void
winstat(int status)
{
  if (win != glutGetWindow()) {
    printf("ERROR: test22\n");
    exit(1);
  }
  printf("%d: win = %d\n", state, status);
  switch(state) {
  case 0:
    if (status != GLUT_FULLY_RETAINED) {
      printf("ERROR: test22\n");
      exit(1);
    }
    glutTimerFunc(1000, time1, 0);
    break;
  case 1:
    if (status != GLUT_PARTIALLY_RETAINED) {
      printf("ERROR: test22\n");
      exit(1);
    }
    break;
  default:
    printf("ERROR: test22\n");
    exit(1);
    break;
  }
  state++;
}

/* ARGSUSED */
void
visbad(int state)
{
  printf("ERROR: test22\n");
  exit(1);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  win = glutCreateWindow("test22");
  glClearColor(1.0, 0.0, 1.0, 0.0);
  glutDisplayFunc(display);
  glutVisibilityFunc(visbad);
  glutVisibilityFunc(NULL);
  glutWindowStatusFunc(NULL);
  glutVisibilityFunc(NULL);
  glutWindowStatusFunc(winstat);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
