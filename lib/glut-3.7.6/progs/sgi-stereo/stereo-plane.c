
/* This is an OpenGL/GLUT implementation of the "plane" stereo  
   demonstration program found in Appendix 1 of "The CrystalEyes  Handbook"
   by Lenny Lipton, 1991, StereoGraphics Corp.   */

/* Ported to OpenGL/GLUT by Mike Blackwell, mkb@cs.cmu.edu, Oct. 1995.  */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "fullscreen_stereo.h"

#define SPEED		100      /* How often to update rotation - milliseconds */
#define STEP		1.0       /* How much to rotate - degrees */

double position = 0;    /* Rotation of plane */

/* Clean up and quit */
void
all_done(void)
{
  stop_fullscreen_stereo();
  exit(0);
}

/* ARGSUSED */
void
timer(int value)
{
  position -= STEP;
  if (position < 0.0)
    position = 360.0 - STEP;
  glutPostRedisplay();
  glutTimerFunc(SPEED, timer, 0);
}

/* ARGSUSED1 */
void
keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case '\033':
    all_done();
    break;
  default:
    putchar('\007');
    fflush(stdout);
    break;
  }
}

/* This routine performs the perspective projection for one eye's subfield.
   The projection is in the direction of the negative z axis.

   xmin, ymax, ymin, ymax = the coordinate range, in the plane of zero
   parallax setting, that will be displayed on the screen. The ratio between
   (xmax-xmin) and (ymax-ymin) should equal the aspect ration of the display.

   znear, zfar = the z-coordinate values of the clipping planes.

   zzps = the z-coordinate of the plane of zero parallax setting.

   dist = the distance from the center of projection to the plane of zero
   parallax.

   eye = half the eye separation; positive for the right eye subfield,
   negative for the left eye subfield. */

void
stereoproj(float xmin, float xmax, float ymin, float ymax,
  float znear, float zfar, float zzps, float dist, float eye)
{
  float xmid, ymid, clip_near, clip_far, top, bottom, left, right, dx, dy, n_over_d;

  dx = xmax - xmin;
  dy = ymax - ymin;

  xmid = (xmax + xmin) / 2.0;
  ymid = (ymax + ymin) / 2.0;

  clip_near = dist + zzps - znear;
  clip_far = dist + zzps - zfar;

  n_over_d = clip_near / dist;

  top = n_over_d * dy / 2.0;
  bottom = -top;
  right = n_over_d * (dx / 2.0 - eye);
  left = n_over_d * (-dx / 2.0 - eye);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(left, right, bottom, top, clip_near, clip_far);

  glTranslatef(-xmid - eye, -ymid, -zzps - dist);
}

void
draw_airplane(void)
{
  static float airplane[9][3] =
  {
    {0.0, 0.5, -4.5},
    {3.0, 0.5, -4.5},
    {3.0, 0.5, -3.5},
    {0.0, 0.5, 0.0},
    {0.0, 0.5, 3.25},
    {0.0, -0.5, 5.5},
    {-3.0, 0.5, -3.5},
    {-3.0, 0.5, -4.5},
    {0.0, -0.5, -4.5}
  };

  glColor3ub(0xb0, 0x30, 0xff);  /* Purple color */

  glBegin(GL_LINE_LOOP);
  glVertex3fv(airplane[6]);
  glVertex3fv(airplane[7]);
  glVertex3fv(airplane[1]);
  glVertex3fv(airplane[2]);
  glVertex3fv(airplane[4]);
  glEnd();

  glBegin(GL_LINE_LOOP);
  glVertex3fv(airplane[0]);
  glVertex3fv(airplane[4]);
  glVertex3fv(airplane[5]);
  glVertex3fv(airplane[8]);
  glEnd();

  glBegin(GL_LINE_STRIP);
  glVertex3fv(airplane[6]);
  glVertex3fv(airplane[3]);
  glVertex3fv(airplane[2]);
  glEnd();
}

/* This routine puts a stereo image of a paper airplane onto the screen */
void
redraw(void)
{
  /* Draw left subfield */
  stereo_left_buffer();

  glClearColor(0.07, 0.07, 0.07, 0.00);
  glClear(GL_COLOR_BUFFER_BIT);

  /* Z-coordinate of plane of zero parallax is 0.0. In that plane, the coord
     range drawn to the screen will be

     (-6.0 to 6.0, -4.8 to 4.8).

     Z-coordinate clipping planes are -6.0 and 6.0. The eyes are set at world
     coord distance 14.5 from the plane of zero parallax, and the eye
     separation is 0.62 in world coords. These two values were calculated
     using equations 11 to 15, and 17 to 19 in chapter 5. */

  stereoproj(-6.0, 6.0, -4.8, 4.8, 6.0, -6.0, 0.0, 14.5, -0.31);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef((float) position, 0.0, 1.0, 0.0);
  glRotatef(-10.0, 1.0, 0.0, 0.0);
  draw_airplane();
  glFlush();

  /* Draw right subfield */
  stereo_right_buffer();

  glClearColor(0.07, 0.07, 0.07, 0.00);
  glClear(GL_COLOR_BUFFER_BIT);

  /* Same as above stereoproj() call, except that eye arg is positive */
  stereoproj(-6.0, 6.0, -4.8, 4.8, 6.0, -6.0, 0.0, 14.5, 0.31);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef((float) position, 0.0, 1.0, 0.0);
  glRotatef(-10.0, 1.0, 0.0, 0.0);
  draw_airplane();
  glFlush();

  glutSwapBuffers();    /* Update screen */
}

int
main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutCreateWindow("GLUT-based SGI hack stereo demo");
  glutFullScreen();
  glutSetCursor(GLUT_CURSOR_NONE);
  start_fullscreen_stereo();

  glutDisplayFunc(redraw);
  glutKeyboardFunc(keyboard);
  glutTimerFunc(SPEED, timer, 0);

  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
  glLineWidth(1.5);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
