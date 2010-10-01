
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
 *  nurbs.c
 *  This program shows a NURBS (Non-uniform rational B-splines)
 *  surface, shaped like a heart.
 */
#include <stdlib.h>
#include <GL/glut.h>

#define S_NUMPOINTS 13
#define S_ORDER     3   
#define S_NUMKNOTS  (S_NUMPOINTS + S_ORDER)
#define T_NUMPOINTS 3
#define T_ORDER     3 
#define T_NUMKNOTS  (T_NUMPOINTS + T_ORDER)
#define SQRT2    1.41421356237309504880

/* initialized local data */

GLfloat sknots[S_NUMKNOTS] =
    {-1.0, -1.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0,
      4.0,  5.0,  6.0, 7.0, 8.0, 9.0, 9.0, 9.0};
GLfloat tknots[T_NUMKNOTS] = {1.0, 1.0, 1.0, 2.0, 2.0, 2.0};

GLfloat ctlpoints[S_NUMPOINTS][T_NUMPOINTS][4] = {
{   {4.,2.,2.,1.},{4.,1.6,2.5,1.},{4.,2.,3.0,1.}    },
{   {5.,4.,2.,1.},{5.,4.,2.5,1.},{5.,4.,3.0,1.} },
{   {6.,5.,2.,1.},{6.,5.,2.5,1.},{6.,5.,3.0,1.} },
{   {SQRT2*6.,SQRT2*6.,SQRT2*2.,SQRT2},
    {SQRT2*6.,SQRT2*6.,SQRT2*2.5,SQRT2},
    {SQRT2*6.,SQRT2*6.,SQRT2*3.0,SQRT2}  },
{   {5.2,6.7,2.,1.},{5.2,6.7,2.5,1.},{5.2,6.7,3.0,1.}   },
{   {SQRT2*4.,SQRT2*6.,SQRT2*2.,SQRT2},
    {SQRT2*4.,SQRT2*6.,SQRT2*2.5,SQRT2},
    {SQRT2*4.,SQRT2*6.,SQRT2*3.0,SQRT2}  },
{   {4.,5.2,2.,1.},{4.,4.6,2.5,1.},{4.,5.2,3.0,1.}  },
{   {SQRT2*4.,SQRT2*6.,SQRT2*2.,SQRT2},
    {SQRT2*4.,SQRT2*6.,SQRT2*2.5,SQRT2},
    {SQRT2*4.,SQRT2*6.,SQRT2*3.0,SQRT2}  },
{   {2.8,6.7,2.,1.},{2.8,6.7,2.5,1.},{2.8,6.7,3.0,1.}   },
{   {SQRT2*2.,SQRT2*6.,SQRT2*2.,SQRT2},
    {SQRT2*2.,SQRT2*6.,SQRT2*2.5,SQRT2},
    {SQRT2*2.,SQRT2*6.,SQRT2*3.0,SQRT2}  },
{   {2.,5.,2.,1.},{2.,5.,2.5,1.},{2.,5.,3.0,1.} },
{   {3.,4.,2.,1.},{3.,4.,2.5,1.},{3.,4.,3.0,1.} },
{   {4.,2.,2.,1.},{4.,1.6,2.5,1.},{4.,2.,3.0,1.}    }
};

GLUnurbsObj *theNurb;

/*  Initialize material property, light source, lighting model, 
 *  and depth buffer.
 */
void myinit(void)
{
    GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_diffuse[] = { 1.0, 0.2, 1.0, 1.0 };
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };

    GLfloat light0_position[] = { 1.0, 0.1, 1.0, 0.0 };
    GLfloat light1_position[] = { -1.0, 0.1, 1.0, 0.0 };

    GLfloat lmodel_ambient[] = { 0.3, 0.3, 0.3, 1.0 };

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_AUTO_NORMAL);

    theNurb = gluNewNurbsRenderer();

    gluNurbsProperty(theNurb, GLU_SAMPLING_TOLERANCE, 25.0);
    gluNurbsProperty(theNurb, GLU_DISPLAY_MODE, GLU_FILL);
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef (4., 4.5, 2.5);
    glRotatef (220.0, 1., 0., 0.);
    glRotatef (115.0, 0., 1., 0.);
    glTranslatef (-4., -4.5, -2.5);

    gluBeginSurface(theNurb);
    gluNurbsSurface(theNurb, 
	    S_NUMKNOTS, sknots,
	    T_NUMKNOTS, tknots,
	    4 * T_NUMPOINTS,
	    4,
	    &ctlpoints[0][0][0], 
	    S_ORDER, T_ORDER,
	    GL_MAP2_VERTEX_4);
    gluEndSurface(theNurb);

    glPopMatrix();
    glFlush();
}

void myReshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.5, 0.5, 0.8, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(7.0,4.5,4.0, 4.5,4.5,2.0, 6.0,-3.0,2.0);
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow (argv[0]);
    myinit();
    glutReshapeFunc (myReshape);
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
