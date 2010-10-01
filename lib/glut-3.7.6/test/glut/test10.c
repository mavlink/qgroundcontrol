
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

/* XXX As a test of 16-bit font support in capturexfont, I made
   a font out of the 16-bit Japanese font named
   '-jis-fixed-medium-r-normal--24-230-75-75-c-240-jisx0208.1'
   and tried it out.  Defining JIS_FONT uses it in this test. */
/* #define JIS_FONT */

#ifdef JIS_FONT
extern void *glutBitmapJis;
#endif

int ch = -2;
void *fonts[] =
{
  GLUT_BITMAP_TIMES_ROMAN_24,
  GLUT_BITMAP_TIMES_ROMAN_10,
  GLUT_BITMAP_9_BY_15,
  GLUT_BITMAP_8_BY_13,
  GLUT_BITMAP_HELVETICA_10,
  GLUT_BITMAP_HELVETICA_12,
  GLUT_BITMAP_HELVETICA_18,
#ifdef JIS_FONT
  &glutBitmapJis
#endif
};
void *names[] =
{
  "Times Roman 24",
  " Times Roman 10",
  "  9 by 15",
  "   8 by 13",
  "    Helvetica 10",
  "     Helvetica 12",
  "      Helvetica 18",
#ifdef JIS_FONT
  "    Mincho JIS"
#endif
};
#define NUM_FONTS (sizeof(fonts)/sizeof(void*))
int font = 0;

void
tick(void)
{
  static int limit = 270;

  ch += 5;
  if (ch > limit) {
    ch = -2;
    font++;
#ifdef JIS_FONT
    if (font == 4) {
      limit = 0x747e;
      ch = 0x2121;
    }
#endif
    if (font == NUM_FONTS) {
      printf("PASS: test10\n");
      exit(0);
    }
  }
  glutPostRedisplay();
}

void
output(int x, int y, char *msg)
{
  glRasterPos2f(x, y);
  while (*msg) {
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *msg);
    msg++;
  }
}

void
display(void)
{
  glutIdleFunc(tick);
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2f(0, 0);
  glutBitmapCharacter(fonts[font], ch);
  glRasterPos2f(30, 30);
  glutBitmapCharacter(fonts[font], ch + 1);
  glRasterPos2f(-30, -30);
  glutBitmapCharacter(fonts[font], ch + 2);
  glRasterPos2f(30, -30);
  glutBitmapCharacter(fonts[font], ch + 3);
  glRasterPos2f(-30, 30);
  glutBitmapCharacter(fonts[font], ch + 4);
  glRasterPos2f(0, 30);
  glutBitmapCharacter(fonts[font], ch + 5);
  glRasterPos2f(0, -30);
  glutBitmapCharacter(fonts[font], ch + 6);
  glRasterPos2f(-30, 0);
  glutBitmapCharacter(fonts[font], ch + 7);
  glRasterPos2f(30, 0);
  glutBitmapCharacter(fonts[font], ch + 8);
  output(-48, -48, names[font]);
  glutSwapBuffers();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(200, 200);
  glutCreateWindow("Test bitmap fonts");
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-50, 50, -50, 50);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glColor3f(1.0, 1.0, 1.0);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
