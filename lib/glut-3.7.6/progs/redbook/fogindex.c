
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
 *  fogindex.c
 *  This program demonstrates fog in color index mode.  
 *  Three cones are drawn at different z values in a linear 
 *  fog.  32 contiguous colors (from 16 to 47) are loaded 
 *  with a color ramp.
 */
#include <stdlib.h>
#include <GL/glut.h>

/*  Initialize color map and fog.  Set screen clear color 
 *  to end of color ramp.
 */
#define NUM_COLORS 32
#define RAMPSTART 16

void 
myinit(void)
{
  int i;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  for (i = 0; i < NUM_COLORS; i++) {
    GLfloat shade;
    shade = (GLfloat) (NUM_COLORS - i) / (GLfloat) NUM_COLORS;
    glutSetColor(16 + i, shade, shade, shade);
  }
  glEnable(GL_FOG);

  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogi(GL_FOG_INDEX, NUM_COLORS);
  glFogf(GL_FOG_START, 0.0);
  glFogf(GL_FOG_END, 4.0);
  glHint(GL_FOG_HINT, GL_NICEST);
  glClearIndex((GLfloat) (NUM_COLORS + RAMPSTART - 1));
}

/*  display() renders 3 cones at different z positions.
 */
void 
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glTranslatef(-1.0, -1.0, -1.0);
  glRotatef(-90.0, 1.0, 0.0, 0.0);
  glIndexi(RAMPSTART);
  glutSolidCone(1.0, 2.0, 10, 10);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0.0, -1.0, -2.25);
  glRotatef(-90.0, 1.0, 0.0, 0.0);
  glIndexi(RAMPSTART);
  glutSolidCone(1.0, 2.0, 10, 10);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(1.0, -1.0, -3.5);
  glRotatef(-90.0, 1.0, 0.0, 0.0);
  glIndexi(RAMPSTART);
  glutSolidCone(1.0, 2.0, 10, 10);
  glPopMatrix();
  glFlush();
}

void 
myReshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (w <= h)
    glOrtho(-2.0, 2.0, -2.0 * (GLfloat) h / (GLfloat) w,
      2.0 * (GLfloat) h / (GLfloat) w, 0.0, 10.0);
  else
    glOrtho(-2.0 * (GLfloat) w / (GLfloat) h,
      2.0 * (GLfloat) w / (GLfloat) h, -2.0, 2.0, 0.0, 10.0);
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
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX | GLUT_DEPTH);
  glutCreateWindow(argv[0]);
  myinit();
  glutReshapeFunc(myReshape);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
