
/* Copyright (c) Mark J. Kilgard, 1997. */

/* A version of the OpenGL Programming Guide's depthcue.c that
   uses the "addfog" routines to add depth cueing as a post-rendering
   pass.  This is not a rather poor application for "addfog" but it
   does demonstrate how it use "addfog".  See "addfog.c" for details.  */

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
 *  depthcue.c
 *  This program draws a wireframe model, which uses 
 *  intensity (brightness) to give clues to distance.
 *  Fog is used to achieve this effect.
 */
#include <stdlib.h>
#include <GL/glut.h>
#include "addfog.h"

/*  Initialize linear fog for depth cueing.
 */
void myinit(void)
{
    GLfloat fogColor[4] = {0.0, 0.0, 0.0, 1.0};

    glFogi (GL_FOG_MODE, GL_LINEAR);
    glHint (GL_FOG_HINT, GL_NICEST);  /*  per pixel   */
    glFogf (GL_FOG_START, 3.0);
    glFogf (GL_FOG_END, 5.0);
    glFogfv (GL_FOG_COLOR, fogColor);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);

    afEyeNearFar(3.0, 5.0);
    afFogStartEnd(3.0, 5.0);
    afFogMode(GL_LINEAR);
    afFogColor(0.0, 0.0, 0.0);
}

int width, height;
int twopass = 1;

/*  display() draws an icosahedron.
 */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!twopass) {
      glEnable(GL_FOG);
    } else {
      glDisable(GL_FOG);
    }

    glColor3f (1.0, 1.0, 1.0);
    glutWireIcosahedron();

    if (twopass) {
      afDoFinalFogPass(0, 0, width, height);
    }

    glutSwapBuffers();
}

void myReshape(int w, int h)
{
  width = w;
  height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective (45.0, (GLfloat) w/(GLfloat) h, 3.0, 5.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
    glTranslatef (0.0, 0.0, -4.0);  /*  move object into view   */
}

/* ARGSUSED1 */
void
keyboard(unsigned char c, int x, int y)
{
  switch(c) {
  case 'T':
  case 't':
    twopass = 1 - twopass;
  case ' ':
    glutPostRedisplay();
    break;
  }
}

void
menu(int value)
{
  switch(value) {
  case 1:
    twopass = 0;
    break;
  case 2:
    twopass = 1;
    break;
  }
  glutPostRedisplay();
}

/*  Main Loop
 */
int main(int argc, char** argv)
{
    glutInitWindowSize(380, 380);
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("depthcuing by post-rendering pass");
    myinit();
    glutReshapeFunc(myReshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutCreateMenu(menu);
    glutAddMenuEntry("GL_LINEAR depthcueing", 1);
    glutAddMenuEntry("\"add fog\" post-render depthcueing", 2);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}

