
/* Copyright (c) Mark J. Kilgard, 1994. */

/**
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

GLboolean doubleBuffer;

/* ARGSUSED1 */
static void
Key(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    exit(0);
  }
}

static void
Draw(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  /* red triangle */
  glColor3ub(200, 0, 0);
  glBegin(GL_POLYGON);
  glVertex3i(-4, -4, 0);
  glVertex3i(4, -4, 0);
  glVertex3i(0, 4, 0);
  glEnd();

  glStencilFunc(GL_EQUAL, 1, 1);
  glStencilOp(GL_INCR, GL_KEEP, GL_DECR);

  /* green square */
  glColor3ub(0, 200, 0);
  glBegin(GL_POLYGON);
  glVertex3i(3, 3, 0);
  glVertex3i(-3, 3, 0);
  glVertex3i(-3, -3, 0);
  glVertex3i(3, -3, 0);
  glEnd();

  glStencilFunc(GL_EQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  /* blue square */
  glColor3ub(0, 0, 200);
  glBegin(GL_POLYGON);
  glVertex3i(3, 3, 0);
  glVertex3i(-3, 3, 0);
  glVertex3i(-3, -3, 0);
  glVertex3i(3, -3, 0);
  glEnd();

  if (doubleBuffer) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
}

static void
Args(int argc, char **argv)
{
  GLint i;

  doubleBuffer = GL_TRUE;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-sb") == 0) {
      doubleBuffer = GL_FALSE;
    } else if (strcmp(argv[i], "-db") == 0) {
      doubleBuffer = GL_TRUE;
    }
  }
}

int
main(int argc, char **argv)
{
  GLenum type;

  glutInit(&argc, argv);
  Args(argc, argv);

  type = GLUT_RGB | GLUT_STENCIL;
  type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
  glutInitDisplayMode(type);
  glutCreateWindow("Stencil Test");

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClearStencil(0);
  glStencilMask(1);
  glEnable(GL_STENCIL_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-5.0, 5.0, -5.0, 5.0, -5.0, 5.0);
  glMatrixMode(GL_MODELVIEW);

  glutKeyboardFunc(Key);
  glutDisplayFunc(Draw);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
