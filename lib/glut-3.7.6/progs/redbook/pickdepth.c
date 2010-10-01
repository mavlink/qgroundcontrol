
/* Copyright (c) Mark J. Kilgard, 1994. */

/*
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
/*
 *  pickdepth.c
 *  Picking is demonstrated in this program.  In 
 *  rendering mode, three overlapping rectangles are 
 *  drawn.  When the left mouse button is pressed, 
 *  selection mode is entered with the picking matrix.  
 *  Rectangles which are drawn under the cursor position
 *  are "picked."  Pay special attention to the depth 
 *  value range, which is returned.
 */
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

void 
myinit(void)
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);
  glDepthRange(0.0, 1.0);  /* The default z mapping */
}

/*  The three rectangles are drawn.  In selection mode, 
 *  each rectangle is given the same name.  Note that 
 *  each rectangle is drawn with a different z value.
 */
void 
drawRects(GLenum mode)
{
  if (mode == GL_SELECT)
    glLoadName(1);
  glBegin(GL_QUADS);
  glColor3f(1.0, 1.0, 0.0);
  glVertex3i(2, 0, 0);
  glVertex3i(2, 6, 0);
  glVertex3i(6, 6, 0);
  glVertex3i(6, 0, 0);
  glEnd();
  if (mode == GL_SELECT)
    glLoadName(2);
  glBegin(GL_QUADS);
  glColor3f(0.0, 1.0, 1.0);
  glVertex3i(3, 2, -1);
  glVertex3i(3, 8, -1);
  glVertex3i(8, 8, -1);
  glVertex3i(8, 2, -1);
  glEnd();
  if (mode == GL_SELECT)
    glLoadName(3);
  glBegin(GL_QUADS);
  glColor3f(1.0, 0.0, 1.0);
  glVertex3i(0, 2, -2);
  glVertex3i(0, 7, -2);
  glVertex3i(5, 7, -2);
  glVertex3i(5, 2, -2);
  glEnd();
}

/*  processHits() prints out the contents of the 
 *  selection array.
 */
void 
processHits(GLint hits, GLuint buffer[])
{
  int i;
  unsigned int j;
  GLuint names, *ptr;

  printf("hits = %d\n", hits);
  ptr = (GLuint *) buffer;
  for (i = 0; i < hits; i++) {  /* for each hit  */
    names = *ptr;
    printf(" number of names for hit = %d\n", names);
    ptr++;
    printf("  z1 is %g;", (float) *ptr/0xffffffff);
    ptr++;
    printf(" z2 is %g\n", (float) *ptr/0xffffffff);
    ptr++;
    printf("   the name is ");
    for (j = 0; j < names; j++) {  /* for each name */
      printf("%d ", *ptr);
      ptr++;
    }
    printf("\n");
  }
}

/*  pickRects() sets up selection mode, name stack, 
 *  and projection matrix for picking.  Then the objects 
 *  are drawn.
 */
#define BUFSIZE 512

void 
pickRects(int button, int state, int x, int y)
{
  GLuint selectBuf[BUFSIZE];
  GLint hits;
  GLint viewport[4];

  if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
    return;

  glGetIntegerv(GL_VIEWPORT, viewport);

  glSelectBuffer(BUFSIZE, selectBuf);
  (void) glRenderMode(GL_SELECT);

  glInitNames();
  glPushName((GLuint) ~0);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
/*  create 5x5 pixel picking region near cursor location */
  gluPickMatrix((GLdouble) x, (GLdouble) (viewport[3] - y),
    5.0, 5.0, viewport);
  glOrtho(0.0, 8.0, 0.0, 8.0, -0.5, 2.5);
  drawRects(GL_SELECT);
  glPopMatrix();
  glFlush();

  hits = glRenderMode(GL_RENDER);
  processHits(hits, selectBuf);
}

void 
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawRects(GL_RENDER);
  glutSwapBuffers();
}

void 
myReshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, 8.0, 0.0, 8.0, -0.5, 2.5);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, depth buffer, and handle input events.
 */
int
main(int argc, char **argv)
{
  glutInitWindowSize(200, 200);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInit(&argc, argv);
  glutCreateWindow(argv[0]);
  myinit();
  glutMouseFunc(pickRects);
  glutReshapeFunc(myReshape);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
