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
 *  tesswind.c
 *  This program demonstrates the winding rule polygon 
 *  tessellation property.  Four tessellated objects are drawn, 
 *  each with very different contours.  When the w key is pressed, 
 *  the objects are drawn with a different winding rule.
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef GLU_VERSION_1_2

/* Win32 calling conventions. */
#ifndef CALLBACK
#define CALLBACK
#endif

GLdouble currentWinding = GLU_TESS_WINDING_ODD;
int currentShape = 0;
GLUtesselator *tobj;
GLuint list;

/*  Make four display lists, 
 *  each with a different tessellated object. 
 */
void makeNewLists (void) {
   int i;
   static GLdouble rects[12][3] = 
      {50.0, 50.0, 0.0, 300.0, 50.0, 0.0, 
       300.0, 300.0, 0.0, 50.0, 300.0, 0.0,
       100.0, 100.0, 0.0, 250.0, 100.0, 0.0, 
       250.0, 250.0, 0.0, 100.0, 250.0, 0.0,
       150.0, 150.0, 0.0, 200.0, 150.0, 0.0, 
       200.0, 200.0, 0.0, 150.0, 200.0, 0.0};
   static GLdouble spiral[16][3] = 
      {400.0, 250.0, 0.0, 400.0, 50.0, 0.0, 
       50.0, 50.0, 0.0, 50.0, 400.0, 0.0, 
       350.0, 400.0, 0.0, 350.0, 100.0, 0.0, 
       100.0, 100.0, 0.0, 100.0, 350.0, 0.0, 
       300.0, 350.0, 0.0, 300.0, 150.0, 0.0, 
       150.0, 150.0, 0.0, 150.0, 300.0, 0.0, 
       250.0, 300.0, 0.0, 250.0, 200.0, 0.0, 
       200.0, 200.0, 0.0, 200.0, 250.0, 0.0};
   static GLdouble quad1[4][3] = 
      {50.0, 150.0, 0.0, 350.0, 150.0, 0.0, 
      350.0, 200.0, 0.0, 50.0, 200.0, 0.0};
   static GLdouble quad2[4][3] =
      {100.0, 100.0, 0.0, 300.0, 100.0, 0.0, 
       300.0, 350.0, 0.0, 100.0, 350.0, 0.0};
   static GLdouble tri[3][3] =
      {200.0, 50.0, 0.0, 250.0, 300.0, 0.0,
       150.0, 300.0, 0.0};
 
   gluTessProperty(tobj, GLU_TESS_WINDING_RULE, 
                   currentWinding);

   glNewList(list, GL_COMPILE);
      gluTessBeginPolygon(tobj, NULL);
         gluTessBeginContour(tobj);
         for (i = 0; i < 4; i++)
            gluTessVertex(tobj, rects[i], rects[i]);
         gluTessEndContour(tobj);
         gluTessBeginContour(tobj);
         for (i = 4; i < 8; i++)
            gluTessVertex(tobj, rects[i], rects[i]);
         gluTessEndContour(tobj);
         gluTessBeginContour(tobj);
         for (i = 8; i < 12; i++)
            gluTessVertex(tobj, rects[i], rects[i]);
         gluTessEndContour(tobj);
      gluTessEndPolygon(tobj);
   glEndList();

   glNewList(list+1, GL_COMPILE);
      gluTessBeginPolygon(tobj, NULL);
         gluTessBeginContour(tobj);
         for (i = 0; i < 4; i++)
            gluTessVertex(tobj, rects[i], rects[i]);
         gluTessEndContour(tobj);
         gluTessBeginContour(tobj);
         for (i = 7; i >= 4; i--)
            gluTessVertex(tobj, rects[i], rects[i]);
         gluTessEndContour(tobj);
         gluTessBeginContour(tobj);
         for (i = 11; i >= 8; i--)
            gluTessVertex(tobj, rects[i], rects[i]);
         gluTessEndContour(tobj);
      gluTessEndPolygon(tobj);
   glEndList();

   glNewList(list+2, GL_COMPILE);
      gluTessBeginPolygon(tobj, NULL);
         gluTessBeginContour(tobj);
         for (i = 0; i < 16; i++)
            gluTessVertex(tobj, spiral[i], spiral[i]);
         gluTessEndContour(tobj);
      gluTessEndPolygon(tobj);
   glEndList();

   glNewList(list+3, GL_COMPILE);
      gluTessBeginPolygon(tobj, NULL);
         gluTessBeginContour(tobj);
         for (i = 0; i < 4; i++)
            gluTessVertex(tobj, quad1[i], quad1[i]);
         gluTessEndContour(tobj);
         gluTessBeginContour(tobj);
         for (i = 0; i < 4; i++)
            gluTessVertex(tobj, quad2[i], quad2[i]);
         gluTessEndContour(tobj);
         gluTessBeginContour(tobj);
         for (i = 0; i < 3; i++)
            gluTessVertex(tobj, tri[i], tri[i]);
         gluTessEndContour(tobj);
      gluTessEndPolygon(tobj);
   glEndList();
}

void display (void) {
   glClear(GL_COLOR_BUFFER_BIT);
   glColor3f(1.0, 1.0, 1.0);
   glPushMatrix(); 
   glCallList(list);
   glTranslatef(0.0, 500.0, 0.0);
   glCallList(list+1);
   glTranslatef(500.0, -500.0, 0.0);
   glCallList(list+2);
   glTranslatef(0.0, 500.0, 0.0);
   glCallList(list+3);
   glPopMatrix(); 
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

/*  combineCallback is used to create a new vertex when edges
 *  intersect.  coordinate location is trivial to calculate,
 *  but weight[4] may be used to average color, normal, or texture 
 *  coordinate data.
 */
/* ARGSUSED */
void CALLBACK combineCallback(GLdouble coords[3], GLdouble *data[4],
                     GLfloat weight[4], GLdouble **dataOut )
{
   GLdouble *vertex;
   vertex = (GLdouble *) malloc(3 * sizeof(GLdouble));

   vertex[0] = coords[0];
   vertex[1] = coords[1];
   vertex[2] = coords[2];
   *dataOut = vertex;
}

void init(void) 
{
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_FLAT);    

   tobj = gluNewTess();
   gluTessCallback(tobj, GLU_TESS_VERTEX, 
                   (GLvoid (CALLBACK*) ()) &glVertex3dv);
   gluTessCallback(tobj, GLU_TESS_BEGIN, 
                   (GLvoid (CALLBACK*) ()) &beginCallback);
   gluTessCallback(tobj, GLU_TESS_END, 
                   (GLvoid (CALLBACK*) ()) &endCallback);
   gluTessCallback(tobj, GLU_TESS_ERROR, 
                   (GLvoid (CALLBACK*) ()) &errorCallback);
   gluTessCallback(tobj, GLU_TESS_COMBINE, 
                   (GLvoid (CALLBACK*) ()) &combineCallback);

   list = glGenLists(4);
   makeNewLists();
}

void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   if (w <= h)
      gluOrtho2D(0.0, 1000.0, 0.0, 1000.0 * (GLdouble)h/(GLdouble)w);
   else
      gluOrtho2D(0.0, 1000.0 * (GLdouble)w/(GLdouble)h, 0.0, 1000.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 'w':
      case 'W':
         if (currentWinding == GLU_TESS_WINDING_ODD)
            currentWinding = GLU_TESS_WINDING_NONZERO;
         else if (currentWinding == GLU_TESS_WINDING_NONZERO)
            currentWinding = GLU_TESS_WINDING_POSITIVE;
         else if (currentWinding == GLU_TESS_WINDING_POSITIVE)
            currentWinding = GLU_TESS_WINDING_NEGATIVE;
         else if (currentWinding == GLU_TESS_WINDING_NEGATIVE)
            currentWinding = GLU_TESS_WINDING_ABS_GEQ_TWO;
         else if (currentWinding == GLU_TESS_WINDING_ABS_GEQ_TWO)
            currentWinding = GLU_TESS_WINDING_ODD;
         makeNewLists();
         glutPostRedisplay();
         break;
      case 27:
         exit(0);
         break;
      default:
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
