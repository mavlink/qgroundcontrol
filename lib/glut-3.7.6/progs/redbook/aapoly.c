/*
 * Copyright (c) 1993-1997, Silicon Graphics, Inc.
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
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

/*
 *  aapoly.c
 *  This program draws filled polygons with antialiased
 *  edges.  The special GL_SRC_ALPHA_SATURATE blending 
 *  function is used.
 *  Pressing the 't' key turns the antialiasing on and off.
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

GLboolean polySmooth = GL_TRUE;

static void init(void)
{
   glCullFace (GL_BACK);
   glEnable (GL_CULL_FACE);
   glBlendFunc (GL_SRC_ALPHA_SATURATE, GL_ONE);
   glClearColor (0.0, 0.0, 0.0, 0.0);
}

#define NFACE 6
#define NVERT 8
void drawCube(GLdouble x0, GLdouble x1, GLdouble y0, GLdouble y1,
        GLdouble z0, GLdouble z1)
{
   static GLfloat v[8][3];
   static GLfloat c[8][4] = {
      {0.0, 0.0, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0},
      {0.0, 1.0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0},
      {0.0, 0.0, 1.0, 1.0}, {1.0, 0.0, 1.0, 1.0},
      {0.0, 1.0, 1.0, 1.0}, {1.0, 1.0, 1.0, 1.0}
   };

/*  indices of front, top, left, bottom, right, back faces  */
   static GLubyte indices[NFACE][4] = {
      {4, 5, 6, 7}, {2, 3, 7, 6}, {0, 4, 7, 3},
      {0, 1, 5, 4}, {1, 5, 6, 2}, {0, 3, 2, 1}
   };

   v[0][0] = v[3][0] = v[4][0] = v[7][0] = x0;
   v[1][0] = v[2][0] = v[5][0] = v[6][0] = x1;
   v[0][1] = v[1][1] = v[4][1] = v[5][1] = y0;
   v[2][1] = v[3][1] = v[6][1] = v[7][1] = y1;
   v[0][2] = v[1][2] = v[2][2] = v[3][2] = z0;
   v[4][2] = v[5][2] = v[6][2] = v[7][2] = z1;

#ifdef GL_VERSION_1_1
   glEnableClientState (GL_VERTEX_ARRAY);
   glEnableClientState (GL_COLOR_ARRAY);
   glVertexPointer (3, GL_FLOAT, 0, v);
   glColorPointer (4, GL_FLOAT, 0, c);
   glDrawElements (GL_QUADS, NFACE*4, GL_UNSIGNED_BYTE, indices);
   glDisableClientState (GL_VERTEX_ARRAY);
   glDisableClientState (GL_COLOR_ARRAY);
#else
   printf ("If this is GL Version 1.0, ");
   printf ("vertex arrays are not supported.\n");
   exit(1);
#endif
}

/*  Note:  polygons must be drawn from front to back
 *  for proper blending.
 */
void display(void)
{
   if (polySmooth) {
      glClear (GL_COLOR_BUFFER_BIT);
      glEnable (GL_BLEND);
      glEnable (GL_POLYGON_SMOOTH);
      glDisable (GL_DEPTH_TEST);
   }
   else { 
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glDisable (GL_BLEND);
      glDisable (GL_POLYGON_SMOOTH);
      glEnable (GL_DEPTH_TEST);
   }

   glPushMatrix ();
      glTranslatef (0.0, 0.0, -8.0);    
      glRotatef (30.0, 1.0, 0.0, 0.0);
      glRotatef (60.0, 0.0, 1.0, 0.0); 
      drawCube(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5);
   glPopMatrix ();

   glFlush ();
}

void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(30.0, (GLfloat) w/(GLfloat) h, 1.0, 20.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 't':
      case 'T':
         polySmooth = !polySmooth;
         glutPostRedisplay();
         break;
      case 27:
         exit(0);  /*  Escape key  */
         break;
      default:
         break;
   }
}

/*  Main Loop
 */
int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB 
                        | GLUT_ALPHA | GLUT_DEPTH);
   glutInitWindowSize(200, 200);
   glutCreateWindow(argv[0]);
   init ();
   glutReshapeFunc (reshape);
   glutKeyboardFunc (keyboard);
   glutDisplayFunc (display);
   glutMainLoop();
   return 0;
}

