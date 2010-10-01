
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o simpletxf simpletxf.c texfont.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <GL/glut.h>

#include "TexFont.h"

int doubleBuffer = 1;
char *filename = "default.txf";
TexFont *txf;
GLfloat angle = 20;

void
idle(void)
{
  angle += 4;
  glutPostRedisplay();
}

void 
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);
}

void
display(void)
{
  char *str;

  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  glRotatef(angle, 0, 0, 1);
  glTranslatef(-2.0, 0.0, 0.0);
  glScalef(1 / 60.0, 1 / 60.0, 1 / 60.0);

  glPushMatrix();
  glColor3f(0.0, 0.0, 1.0);
  str = "OpenGL is";
  txfRenderString(txf, str, (int) strlen(str));
  glPopMatrix();

  glPushMatrix();
  glColor3f(1.0, 0.0, 0.0);
  glTranslatef(0.0, -60.0, 0.0);
  str = "the best.";
  txfRenderString(txf, str, (int) strlen(str));
  glPopMatrix();

  glPopMatrix();

  /* Swap the buffers if necessary. */
  if (doubleBuffer) {
    glutSwapBuffers();
  }
}

int minifyMenu;

void
minifySelect(int value)
{
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
  glutPostRedisplay();
}

int alphaMenu;

void
alphaSelect(int value)
{
  switch (value) {
  case GL_ALPHA_TEST:
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.5);
    break;
  case GL_BLEND:
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case GL_NONE:
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    break;
  }
}

void
mainSelect(int value)
{
  if (value == 666) {
    exit(0);
  }
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-sb")) {
      doubleBuffer = 0;
    } else {
      filename = argv[i];
    }
  }
  if (filename == NULL) {
    fprintf(stderr, "usage: show [GLUT-options] [-sb] txf-file\n");
    exit(1);
  }
  txf = txfLoadFont(filename);
  if (txf == NULL) {
    fprintf(stderr, "Problem loading %s\n", filename);
    exit(1);
  }

  if (doubleBuffer) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  } else {
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH);
  }
  glutInitWindowSize(300, 300);
  glutCreateWindow("texfont");
  glutDisplayFunc(display);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, 1.0, 0.1, 20.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,
    0.0, 0.0, 0.0,
    0.0, 1.0, 0.0);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);
  alphaSelect(GL_ALPHA_TEST);
  minifySelect(GL_NEAREST);

  txfEstablishTexture(txf, 0, GL_TRUE);

  glutVisibilityFunc(visible);

  minifyMenu = glutCreateMenu(minifySelect);
  glutAddMenuEntry("Nearest", GL_NEAREST);
  glutAddMenuEntry("Linear", GL_LINEAR);
  glutAddMenuEntry("Nearest mipmap nearest", GL_NEAREST_MIPMAP_NEAREST);
  glutAddMenuEntry("Linear mipmap nearest", GL_LINEAR_MIPMAP_NEAREST);
  glutAddMenuEntry("Nearest mipmap linear", GL_NEAREST_MIPMAP_LINEAR);
  glutAddMenuEntry("Linear mipmap linear", GL_LINEAR_MIPMAP_LINEAR);

  alphaMenu = glutCreateMenu(alphaSelect);
  glutAddMenuEntry("Alpha testing", GL_ALPHA_TEST);
  glutAddMenuEntry("Alpha blending", GL_BLEND);
  glutAddMenuEntry("Nothing", GL_NONE);

  glutCreateMenu(mainSelect);
  glutAddSubMenu("Filtering", minifyMenu);
  glutAddSubMenu("Alpha", alphaMenu);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
