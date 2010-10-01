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
 *  tess.c
 *  This program demonstrates polygon tessellation.
 *  Two tesselated objects are drawn.  The first is a
 *  rectangle with a triangular hole.  The second is a
 *  smooth shaded, self-intersecting star.
 *
 *  Note the exterior rectangle is drawn with its vertices
 *  in counter-clockwise order, but its interior clockwise.
 *  Note the combineCallback is needed for the self-intersecting
 *  star.  Also note that removing the TessProperty for the 
 *  star will make the interior unshaded (WINDING_ODD).
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef GLU_VERSION_1_2

/* Win32 calling conventions. */
#ifndef CALLBACK
#define CALLBACK
#endif

GLuint startList;

void display (void) {
   glClear(GL_COLOR_BUFFER_BIT);
   glColor3f(1.0, 1.0, 1.0);
   glCallList(startList);
   glCallList(startList + 1);
   glFlush();
}

void CALLBACK beginCallback(GLenum which)
{
   glBegin(which);
}

void CALLBACK errorCallback(GLenum errorCode)
{
   const GLubyte *estring;

   estring = gluErrorString(errorCode);
   fprintf(stderr, "Tessellation Error: %s\n", estring);
   exit(0);
}

void CALLBACK endCallback(void)
{
   glEnd();
}

void CALLBACK vertexCallback(GLvoid *vertex)
{
   const GLdouble *pointer;

   pointer = (GLdouble *) vertex;
   glColor3dv(pointer+3);
   glVertex3dv(vertex);
}

/*  combineCallback is used to create a new vertex when edges
 *  intersect.  coordinate location is trivial to calculate,
 *  but weight[4] may be used to average color, normal, or texture
 *  coordinate data.  In this program, color is weighted.
 */
void CALLBACK combineCallback(GLdouble coords[3], 
                     GLdouble *vertex_data[4],
                     GLfloat weight[4], GLdouble **dataOut )
{
   GLdouble *vertex;
   int i;

   vertex = (GLdouble *) malloc(6 * sizeof(GLdouble));

   vertex[0] = coords[0];
   vertex[1] = coords[1];
   vertex[2] = coords[2];
   for (i = 3; i < 6; i++)
      vertex[i] = weight[0] * vertex_data[0][i] 
                  + weight[1] * vertex_data[1][i]
                  + weight[2] * vertex_data[2][i] 
                  + weight[3] * vertex_data[3][i];
   *dataOut = vertex;
}

void init (void) 
{
   GLUtesselator *tobj;
   GLdouble rect[4][3] = {50.0, 50.0, 0.0,
                          200.0, 50.0, 0.0,
                          200.0, 200.0, 0.0,
                          50.0, 200.0, 0.0};
   GLdouble tri[3][3] = {75.0, 75.0, 0.0,
                         125.0, 175.0, 0.0,
                         175.0, 75.0, 0.0};
   GLdouble star[5][6] = {250.0, 50.0, 0.0, 1.0, 0.0, 1.0,
                          325.0, 200.0, 0.0, 1.0, 1.0, 0.0,
                          400.0, 50.0, 0.0, 0.0, 1.0, 1.0,
                          250.0, 150.0, 0.0, 1.0, 0.0, 0.0,
                          400.0, 150.0, 0.0, 0.0, 1.0, 0.0};

   glClearColor(0.0, 0.0, 0.0, 0.0);

   startList = glGenLists(2);

   tobj = gluNewTess();
   gluTessCallback(tobj, GLU_TESS_VERTEX, 
                   (GLvoid (CALLBACK*) ()) &glVertex3dv);
   gluTessCallback(tobj, GLU_TESS_BEGIN, 
                   (GLvoid (CALLBACK*) ()) &beginCallback);
   gluTessCallback(tobj, GLU_TESS_END, 
                   (GLvoid (CALLBACK*) ()) &endCallback);
   gluTessCallback(tobj, GLU_TESS_ERROR, 
                   (GLvoid (CALLBACK*) ()) &errorCallback);

   /*  rectangle with triangular hole inside  */
   glNewList(startList, GL_COMPILE);
   glShadeModel(GL_FLAT);    
   gluTessBeginPolygon(tobj, NULL);
      gluTessBeginContour(tobj);
         gluTessVertex(tobj, rect[0], rect[0]);
         gluTessVertex(tobj, rect[1], rect[1]);
         gluTessVertex(tobj, rect[2], rect[2]);
         gluTessVertex(tobj, rect[3], rect[3]);
      gluTessEndContour(tobj);
      gluTessBeginContour(tobj);
         gluTessVertex(tobj, tri[0], tri[0]);
         gluTessVertex(tobj, tri[1], tri[1]);
         gluTessVertex(tobj, tri[2], tri[2]);
      gluTessEndContour(tobj);
   gluTessEndPolygon(tobj);
   glEndList();

   gluTessCallback(tobj, GLU_TESS_VERTEX, 
                   (GLvoid (CALLBACK*) ()) &vertexCallback);
   gluTessCallback(tobj, GLU_TESS_BEGIN, 
                   (GLvoid (CALLBACK*) ()) &beginCallback);
   gluTessCallback(tobj, GLU_TESS_END, 
                   (GLvoid (CALLBACK*) ()) &endCallback);
   gluTessCallback(tobj, GLU_TESS_ERROR, 
                   (GLvoid (CALLBACK*) ()) &errorCallback);
   gluTessCallback(tobj, GLU_TESS_COMBINE, 
                   (GLvoid (CALLBACK*) ()) &combineCallback);

   /*  smooth shaded, self-intersecting star  */
   glNewList(startList + 1, GL_COMPILE);
   glShadeModel(GL_SMOOTH);    
   gluTessProperty(tobj, GLU_TESS_WINDING_RULE,
                   GLU_TESS_WINDING_POSITIVE);
   gluTessBeginPolygon(tobj, NULL);
      gluTessBeginContour(tobj);
         gluTessVertex(tobj, star[0], star[0]);
         gluTessVertex(tobj, star[1], star[1]);
         gluTessVertex(tobj, star[2], star[2]);
         gluTessVertex(tobj, star[3], star[3]);
         gluTessVertex(tobj, star[4], star[4]);
      gluTessEndContour(tobj);
   gluTessEndPolygon(tobj);
   glEndList();
   gluDeleteTess(tobj);
}

void reshape (int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0.0, (GLdouble) w, 0.0, (GLdouble) h);
}

/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 27:
         exit(0);
         break;
   }
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
   glutInitWindowSize(500, 500);
   glutCreateWindow(argv[0]);
   init();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMainLoop();
   return 0;  
}

#else
int main(int argc, char** argv)
{
    fprintf (stderr, "This program demonstrates the new tesselator API in GLU 1.2.\n");
    fprintf (stderr, "Your GLU library does not support this new interface, sorry.\n");
    return 0;
}
#endif
