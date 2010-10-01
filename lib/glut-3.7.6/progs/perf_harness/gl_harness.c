
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

extern testInit(int argc, char **argv, int width, int height);
extern testRender(void);

int testIterationsStep, testDisplayMode, testMinimumTestTime;
float timeEnd, timeStart;
int error;
int renders = 0, damaged = 0;

/* ARGSUSED */
void
report(int value)
{
  float duration;

  duration = (timeEnd - timeStart) / 1000.0;
  printf("Renders/second = %g\n",
    renders / duration);
  printf("  after %d iterations over %g seconds\n",
    renders, duration);
  if (error != GL_NO_ERROR)
    printf("OpenGL errors occurred during test; RESULTS ARE DUBIOUS.\n");
  if (damaged != 1)
    printf("Window disturbed during test; RESULTS ARE DUBIOUS.\n");
  printf("\n");
  exit(damaged != 1);
}

/* ARGSUSED */
void
ensureEventsGotten(int value)
{
  /* Hack.  Creating a new window _ensures_ any outstanding
     expose event from popping the window will be retrieved. */
  glutCreateWindow("dummy");
  glutHideWindow();
  glutTimerFunc(1, report, 0);
}

void
displayDone(void)
{
  if (glutLayerGet(GLUT_NORMAL_DAMAGED))
    damaged++;
}

/* ARGSUSED */
void
done(int value)
{
  glFinish();
  timeEnd = glutGet(GLUT_ELAPSED_TIME);
  error = glGetError();

  /* Pop the window.  If the window was obscured by another
     window during the test, raising the window should generate
     an expose event we want to catch. */
  glutPopWindow();

  /* The test is over so only notice an expose and do not run
     the testRender routine. */
  glutDisplayFunc(displayDone);
  glutTimerFunc(1, ensureEventsGotten, 0);
}

void
display(void)
{
  int i;

  if (glutLayerGet(GLUT_NORMAL_DAMAGED)) {
    damaged++;
    if (damaged == 1) {
      glutTimerFunc(testMinimumTestTime * 1000, done, 0);
      timeStart = glutGet(GLUT_ELAPSED_TIME);
    }
  }
  for (i = 0; i < testIterationsStep; i++) {
    testRender();
    renders++;
  }
  glutPostRedisplay();
}

void
visible(int state)
{
  if (state == GLUT_NOT_VISIBLE)
    damaged++;
}

int
main(int argc, char **argv)
{
  char *newArgv[100];
  int newArgc, i;

  /* Defaults; testInit may override these. */
  testIterationsStep = 5;
  testDisplayMode = GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH;
  testMinimumTestTime = 10;  /* seconds */

  glutInit(&argc, argv);
  newArgc = 1;
  newArgv[0] = argv[0];
  for (i = 1; i < argc; i++) {
    if (!strcmp("-time", argv[i])) {
      i++;
      if (argv[i] == NULL) {
        fprintf(stderr, "%s: -time option needs argument\n", argv[0]);
        exit(1);
      }
      testMinimumTestTime = (int) strtol(argv[i], NULL, 0);
    } else if (!strcmp("-mode", argv[i])) {
      i++;
      if (argv[i] == NULL) {
        fprintf(stderr, "%s: -mode option needs argument\n", argv[0]);
        exit(1);
      }
      testDisplayMode = (int) strtol(argv[i], NULL, 0);
    } else if (!strcmp("-iters", argv[i])) {
      i++;
      if (argv[i] == NULL) {
        fprintf(stderr, "%s: -mode option needs argument\n", argv[0]);
        exit(1);
      }
      testIterationsStep = (int) strtol(argv[i], NULL, 0);
    } else {
      newArgv[newArgc] = argv[i];
      newArgc++;
    }
  }
  newArgv[newArgc] = NULL;

  glutInitDisplayMode(testDisplayMode);
  glutCreateWindow("OpenGL performance test");
  glutDisplayFunc(display);
  glutVisibilityFunc(visible);
  testInit(newArgc, newArgv,
    glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
