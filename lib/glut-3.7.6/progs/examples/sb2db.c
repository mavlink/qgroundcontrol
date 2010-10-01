
/* Copyright (c) Mark J. Kilgard, 1994, 1996. */

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

/* sb2db.c - This program demonstrates switching between single buffered
   and double buffered windows when using GLUT.  Use the pop-up menu to
   change the buffering style used.  On machine that split the screen's
   color resolution in half when double buffering, you should notice better
   coloration (less or no dithering) in single buffer mode (but flicker on
   redraws, particularly when rotation is toggled on).  */

/* This program is based on the GLUT scene.c program. */

#include <stdlib.h>
#include <GL/glut.h>

int sbwin, dbwin;
int angle;

/*  Initialize material property and light source.  */
void
myinit(void)
{
  GLfloat light_ambient[] =
  {0.3, 0.3, 0.3, 1.0};
  GLfloat light_diffuse[] =
  {6.0, 6.0, 6.0, 1.0};
  GLfloat light_specular[] =
  {1.0, 1.0, 1.0, 1.0};
  /* light_position is NOT default value */
  GLfloat light_position[] =
  {-1.0, 1.0, 1.0, 0.0};

  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glEnable(GL_LIGHT0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

void
display(void)
{
  static GLfloat red[] =
  {0.8, 0.0, 0.0, 1.0};
  static GLfloat yellow[] =
  {0.8, 0.8, 0.0, 1.0};
  static GLfloat green[] =
  {0.0, 0.8, 0.0, 1.0};

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  glRotatef(angle, 1.0, 0.0, 0.0);
  glScalef(1.3, 1.3, 1.3);
  glRotatef(20.0, 1.0, 0.0, 0.0);

  glPushMatrix();
  glTranslatef(-0.75, 0.5, 0.0);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, red);
  glutSolidTorus(0.275, 0.85, 10, 15);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-0.75, -0.5, 0.0);
  glRotatef(270.0, 1.0, 0.0, 0.0);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, yellow);
  glutSolidCone(1.0, 1.0, 40, 40);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0.75, 0.0, -1.0);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, green);
  glutSolidIcosahedron();
  glPopMatrix();

  glPopMatrix();

  if (glutGetWindow() == sbwin) {
    glFlush();
  } else {
    glutSwapBuffers();
  }
}

/* Used by both windows, this routine setups the OpenGL context's
   projection matrix correctly.  Note that we call this routine for
   both contexts to keep them in sync after reshapes. */
void
reshapeOpenGLState(int w, int h)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (w <= h)
    glOrtho(-2.5, 2.5, -2.5 * (GLfloat) h / (GLfloat) w,
      2.5 * (GLfloat) h / (GLfloat) w, -10.0, 10.0);
  else
    glOrtho(-2.5 * (GLfloat) w / (GLfloat) h,
      2.5 * (GLfloat) w / (GLfloat) h, -2.5, 2.5, -10.0, 10.0);
  glMatrixMode(GL_MODELVIEW);
}

/* When the single buffered (ie, the top window) gets resized, we
   need to resize the child double buffered window as well.  Hence
   the glutReshapeWindow on the child.  NOTE:  You want a separate
   resize callback for the double buffered window to set the viewport
   since the window's size won't really be changed until the double buffered
   gets its dbReshape callback.  Otherwise, you could trick OpenGL intop
   clipping based on the old window size. */
void
sbReshape(int w, int h)
{
  glutSetWindow(sbwin);
  glViewport(0, 0, w, h);
  reshapeOpenGLState(w, h);
  glutSetWindow(dbwin);
  glutReshapeWindow(w, h);
}

void
dbReshape(int w, int h)
{
  glViewport(0, 0, w, h);
  reshapeOpenGLState(w, h);
}

void
rotation(void)
{
  angle += 2;
  angle = angle % 360;
  glutPostRedisplay();
}

int animation = 0;  /* Are we doing an animated rotation currently? */

void
main_menu(int value)
{
  switch (value) {
  case 1:
    /* Smart toggle rotation ensures we switch to double buffered when
       animating and single buffered when not animating. */
    animation = 1 - animation;  /* Toggle. */
    if (animation) {
      glutIdleFunc(rotation);
      glutSetWindow(sbwin);
      glutSetWindowTitle("sb2db - double buffer mode");
      glutSetWindow(dbwin);
      glutShowWindow();   /* Show the double buffered window. */
    } else {
      glutIdleFunc(NULL);
      glutSetWindow(sbwin);
      glutSetWindowTitle("sb2db - single buffer mode");
      glutSetWindow(dbwin);
      glutHideWindow();   /* Hide the double buffered window. */
    }
    break;
  case 2:
    glutSetWindow(dbwin);
    glutHideWindow();   /* Hide the double buffered window. */
    glutSetWindow(sbwin);
    glutSetWindowTitle("sb2db - single buffer mode");
    break;
  case 3:
    glutSetWindow(sbwin);
    glutSetWindowTitle("sb2db - double buffer mode");
    glutSetWindow(dbwin);
    glutShowWindow();   /* Show the double buffered window. */
    break;
  case 4:
    animation = 1 - animation;  /* Toggle. */
    if (animation)
      glutIdleFunc(rotation);
    else
      glutIdleFunc(NULL);
    break;
  case 666:
    exit(0);
    break;
  }
}

/* You have to track the visibility of both the single buffered
   and double buffered windows together. */
void
visibility(int state)
{
  static int sbvis = GLUT_NOT_VISIBLE, dbvis = GLUT_NOT_VISIBLE;
  int eithervis;

  if (glutGetWindow() == sbwin) {
    sbvis = state;
  } else {
    dbvis = state;
  }
  eithervis = (sbvis == GLUT_VISIBLE) || (dbvis == GLUT_VISIBLE);
  if (eithervis) {
    /* Resume rotating idle callback if we become visible and
       animation is enabled. */
    if (animation) {
      glutIdleFunc(rotation);
    }
  } else {
    /* Disable animation when both windows are not visible. */
    glutIdleFunc(NULL);
  }
}

int
main(int argc, char **argv)
{
  glutInitWindowSize(500, 500);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_SINGLE);

  /* The top window is single buffered. */
  sbwin = glutCreateWindow(argv[0]);
  glutReshapeFunc(sbReshape);
  glutDisplayFunc(display);
  glutVisibilityFunc(visibility);
  myinit();

  /* The child window is double buffered.  We show this window
     when displaying double buffered and hide it to show the
     single buffered window. */
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  dbwin = glutCreateSubWindow(glutGetWindow(),
    0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
  glutDisplayFunc(display);
  glutReshapeFunc(dbReshape);
  glutVisibilityFunc(visibility);

  /* Call myinit for both the single buffered window and the
     double buffered window.  We must mirror the same OpenGL
     state in both window's OpenGL contexts.  If you make this
     program more complicated, remember to keep the window's
     context state in sync. */
  myinit();

  /* Initially hide the double buffered window to start in
     single buffered mode. */
  glutHideWindow();

  glutCreateMenu(main_menu);
  glutAddMenuEntry("Smart rotation toggle", 1);
  glutAddMenuEntry("Single buffer", 2);
  glutAddMenuEntry("Double buffer", 3);
  glutAddMenuEntry("Toggle rotation", 4);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutSetWindow(sbwin);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
