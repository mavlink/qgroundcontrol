
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

int ch = -2;
void *font = GLUT_STROKE_ROMAN;

void
tick(void)
{
  ch += 1;
  if (ch > 180) {
    if (font == GLUT_STROKE_MONO_ROMAN) {
      printf("PASS: test4\n");
      exit(0);
    }
    ch = -2;
    font = GLUT_STROKE_MONO_ROMAN;
  }
  glutPostRedisplay();
}

void
display(void)
{
  glutIdleFunc(tick);
  glClear(GL_COLOR_BUFFER_BIT);
  glPushMatrix();
  glutStrokeCharacter(font, ch);
  glPopMatrix();
  glutSwapBuffers();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(200, 200);
  glutCreateWindow("Test stroke fonts");
  if (glutGet(GLUT_WINDOW_COLORMAP_SIZE) != 0) {
    printf("FAIL: bad RGBA colormap size\n");
    exit(1);
  }
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-50, 150, -50, 150);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glColor3f(1.0, 1.0, 1.0);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
