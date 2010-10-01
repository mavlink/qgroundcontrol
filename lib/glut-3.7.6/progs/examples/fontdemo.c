
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <GL/glut.h>

void
bitmap_output(int x, int y, char *string, void *font)
{
  int len, i;

  glRasterPos2f(x, y);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(font, string[i]);
  }
}

void
stroke_output(GLfloat x, GLfloat y, char *format,...)
{
  va_list args;
  char buffer[200], *p;

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  glPushMatrix();
  glTranslatef(x, y, 0);
  glScalef(0.005, 0.005, 0.005);
  for (p = buffer; *p; p++)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  glPopMatrix();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  bitmap_output(40, 35, "This is written in a GLUT bitmap font.",
    GLUT_BITMAP_TIMES_ROMAN_24);
  bitmap_output(30, 210, "More bitmap text is a fixed 9 by 15 font.",
    GLUT_BITMAP_9_BY_15);
  bitmap_output(70, 240, "                Helvetica is yet another bitmap font.",
    GLUT_BITMAP_HELVETICA_18);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluPerspective(40.0, 1.0, 0.1, 20.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  gluLookAt(0.0, 0.0, 4.0,  /* eye is at (0,0,30) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glPushMatrix();
  glTranslatef(0, 0, -4);
  glRotatef(50, 0, 1, 0);
  stroke_output(-2.5, 1.1, "  This is written in a");
  stroke_output(-2.5, 0, " GLUT stroke font.");
  stroke_output(-2.5, -1.1, "using 3D perspective.");
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glFlush();
}

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glScalef(1, -1, 1);
  glTranslatef(0, -h, 0);
  glMatrixMode(GL_MODELVIEW);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(465, 250);
  glutCreateWindow("GLUT bitmap & stroke font example");
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glColor3f(0, 0, 0);
  glLineWidth(3.0);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
