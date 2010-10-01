/*
 * walkviewer.c [from agviewer.c  (version 1.0)]
 *
 * AGV: a glut viewer. Routines for viewing a 3d scene w/ glut
 *
 * See agv_example.c and agviewer.h comments within for more info.
 *
 * I welcome any feedback or improved versions!
 *
 * Philip Winston - 4/11/95
 * pwinston@hmc.edu
 * http://www.cs.hmc.edu/people/pwinston
 */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "walkviewer.h"

/***************************************************************/
/************************** SETTINGS ***************************/
/***************************************************************/

   /* Initial polar movement settings */
#define INIT_POLAR_AZ  0.0
#define INIT_POLAR_EL 30.0
#define INIT_DIST      3.0
#define INIT_AZ_SPIN   0.5
#define INIT_EL_SPIN   0.0

  /* Initial flying movement settings */
#define INIT_EX        0.0
#define INIT_EY       -2.0
#define INIT_EZ       -2.0
#define INIT_MOVE     0.01
#define MINMOVE      0.001    

  /* Start in this mode */
#define INIT_MODE   POLAR   

  /* Controls:  */

  /* map 0-9 to an EyeMove value when number key is hit in FLYING mode */
#define SPEEDFUNCTION(x) ((x)*(x)*0.001)  

  /* Multiply EyeMove by (1+-MOVEFRACTION) when +/- hit in FLYING mode */
#define MOVEFRACTION 0.25   

  /* What to multiply number of pixels mouse moved by to get rotation amount */
#define EL_SENS   0.5
#define AZ_SENS   0.5

  /* What to multiply number of pixels mouse moved by for movement amounts */
#define DIST_SENS 0.01
#define E_SENS    0.01

  /* Minimum spin to allow in polar (lower forced to zero) */
#define MIN_AZSPIN 0.1
#define MIN_ELSPIN 0.1

  /* Factors used in computing dAz and dEl (which determine AzSpin, ElSpin) */
#define SLOW_DAZ 0.90
#define SLOW_DEL 0.90
#define PREV_DAZ 0.80
#define PREV_DEL 0.80
#define CUR_DAZ  0.20
#define CUR_DEL  0.20

/***************************************************************/
/************************** GLOBALS ****************************/
/***************************************************************/

int     MoveMode = INIT_MODE;  /* FLYING or POLAR mode? */

GLfloat Ex = INIT_EX,             /* flying parameters */
        Ey = INIT_EY,
        Ez = INIT_EZ,
        EyeMove = INIT_MOVE,     

        EyeDist = INIT_DIST,      /* polar params */
        AzSpin  = INIT_AZ_SPIN,
        ElSpin  = INIT_EL_SPIN,

        EyeAz = INIT_POLAR_AZ,    /* used by both */
        EyeEl = INIT_POLAR_EL;

int agvMoving;    /* Currently moving?  */

int downx, downy,   /* for tracking mouse position */
    lastx, lasty,
    downb = -1;     /* and button status */
						
GLfloat downDist, downEl, downAz, /* for saving state of things */
        downEx, downEy, downEz,   /* when button is pressed */
        downEyeMove;                

GLfloat dAz, dEl, lastAz, lastEl;  /* to calculate spinning w/ polar motion */
int     AdjustingAzEl = 0;

int AllowIdle, RedisplayWindow; 
   /* If AllowIdle is 1 it means AGV will install its own idle which
    * will update the viewpoint as needed and send glutPostRedisplay() to the
    * window RedisplayWindow which was set in agvInit().  AllowIdle of 0
    * means AGV won't install an idle funciton, and something like
    * "if (agvMoving) agvMove()" should exist at the end of the running
    * idle function.
    */

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define TORAD(x) ((M_PI/180.0)*(x))
#define TODEG(x) ((180.0/M_PI)*(x))

/***************************************************************/
/************************ PROTOTYPES ***************************/
/***************************************************************/

  /*
   * these are functions meant for internal use only
   * the other prototypes are in agviewer.h
   */

void PolarLookFrom(GLfloat dist, GLfloat elevation, GLfloat azimuth);
void FlyLookFrom(GLfloat x, GLfloat y, GLfloat z, GLfloat az, GLfloat el);
int  ConstrainEl(void);
void MoveOn(int v);
void SetMove(float newmove);
void normalize(GLfloat v[3]);
void ncrossprod(float v1[3], float v2[3], float cp[3]);


/***************************************************************/
/************************ agvInit ******************************/
/***************************************************************/

void agvInit(int window)
{
  glutMouseFunc(agvHandleButton);
  glutMotionFunc(agvHandleMotion);
  glutKeyboardFunc(agvHandleKeys);
  RedisplayWindow = glutGetWindow();
  agvSetAllowIdle(window);
}

/***************************************************************/
/************************ VIEWPOINT STUFF **********************/
/***************************************************************/

  /*
   * viewing transformation modified from page 90 of red book
   */
void PolarLookFrom(GLfloat dist, GLfloat elevation, GLfloat azimuth)
{
  glTranslatef(0, 0, -dist);
  glRotatef(elevation, 1, 0, 0);
  glRotatef(azimuth, 0, 1, 0);

}

  /*
   * I took the idea of tracking eye position in absolute
   * coords and direction looking in Polar form from denis
   */
void FlyLookFrom(GLfloat x, GLfloat y, GLfloat z, GLfloat az, GLfloat el)
{
  float lookat[3], perp[3], up[3];

  lookat[0] = sin(TORAD(az))*cos(TORAD(el));
  lookat[1] = sin(TORAD(el));
  lookat[2] = -cos(TORAD(az))*cos(TORAD(el));
  normalize(lookat);
  perp[0] = lookat[2];
  perp[1] = 0;
  perp[2] = -lookat[0];
  normalize(perp);
  ncrossprod(lookat, perp, up);
  gluLookAt(x, y, z,
            x+lookat[0], y+lookat[1], z+lookat[2],
            up[0], up[1], up[2]);
}

  /*
   * Call viewing transformation based on movement mode
   */
void agvViewTransform(void)
{ 
  switch (MoveMode) {
    case FLYING:
      FlyLookFrom(Ex, Ey, Ez, EyeAz, EyeEl);
      break;
    case POLAR:
      PolarLookFrom(EyeDist, EyeEl, EyeAz);
      break;
    }
}

  /*
   * keep them vertical; I think this makes a lot of things easier, 
   * but maybe it wouldn't be too hard to adapt things to let you go
   * upside down
   */
int ConstrainEl(void)
{
  if (EyeEl <= -90) {
    EyeEl = -89.99;
    return 1;
  } else if (EyeEl >= 90) {
    EyeEl = 89.99;
    return 1;
  }
  return 0;
}

 /*
  * Idle Function - moves eyeposition
  */
void agvMove(void)
{

  switch (MoveMode)  {
    case FLYING:
      Ex += EyeMove*sin(TORAD(EyeAz))*cos(TORAD(EyeEl));
      Ey += EyeMove*sin(TORAD(EyeEl));
      Ez -= EyeMove*cos(TORAD(EyeAz))*cos(TORAD(EyeEl));
      break;

    case POLAR:
      EyeEl += ElSpin;
      EyeAz += AzSpin;
      if (ConstrainEl()) {  /* weird spin thing to make things look     */
        ElSpin = -ElSpin;      /* look better when you are kept from going */
                               /* upside down while spinning - Isn't great */
        if (fabs(ElSpin) > fabs(AzSpin))
          AzSpin = fabs(ElSpin) * ((AzSpin > 0) ? 1 : -1);
      }
      break;
    }

  if (AdjustingAzEl) {
    dAz *= SLOW_DAZ;
    dEl *= SLOW_DEL;
  }

  if (AllowIdle) {
    glutPostWindowRedisplay(RedisplayWindow);
  }
}


  /*
   * Don't install agvMove as idle unless we will be updating the view
   * and we've been given a RedisplayWindow
   */
void MoveOn(int v)
{
  if (v && ((MoveMode == FLYING && EyeMove != 0) ||
             (MoveMode == POLAR &&
             (AzSpin != 0 || ElSpin != 0 || AdjustingAzEl)))) {
    agvMoving = 1;
    if (AllowIdle)
      glutIdleFunc(agvMove);
  } else {
    agvMoving = 0;
    if (AllowIdle)
      glutIdleFunc(NULL);
  }
}

  /*
   * set new redisplay window.  If <= 0 it means we are not to install
   * an idle function and will rely on whoever does install one to 
   * put statement like "if (agvMoving) agvMove();" at end of it
   */
void agvSetAllowIdle(int allowidle)
{
  if ((AllowIdle = allowidle))
    MoveOn(1);
}


  /*
   * when moving to flying we stay in the same spot, moving to polar we
   * reset since we have to be looking at the origin (though a pivot from
   * current position to look at origin might be cooler)
   */
void agvSwitchMoveMode(int move)
{
  switch (move) {
    case FLYING:
      Ex    = -EyeDist*sin(TORAD(EyeAz))*cos(TORAD(EyeEl));
      Ey    =  EyeDist*sin(TORAD(EyeEl));
      Ez    =  EyeDist*(cos(TORAD(EyeAz))*cos(TORAD(EyeEl)));
      EyeAz =  EyeAz;
      EyeEl = -EyeEl;
      EyeMove = INIT_MOVE;
      break;
    case POLAR:
      EyeDist = INIT_DIST;
      EyeAz   = INIT_POLAR_AZ;
      EyeEl   = INIT_POLAR_EL;
      AzSpin  = INIT_AZ_SPIN;
      ElSpin  = INIT_EL_SPIN;
      break;
    }
  MoveMode = move;
  MoveOn(1);
  glutPostRedisplay();
}

/***************************************************************/
/*******************    MOUSE HANDLING   ***********************/
/***************************************************************/

void agvHandleButton(int button, int state, int x, int y)
{
 if (state == GLUT_DOWN && downb == -1) {  
    lastx = downx = x;
    lasty = downy = y;
    downb = button;    

    switch (button) {
      case GLUT_LEFT_BUTTON:
        lastEl = downEl = EyeEl;
        lastAz = downAz = EyeAz;
        AzSpin = ElSpin = dAz = dEl = 0;
        AdjustingAzEl = 1;
	MoveOn(1);
        break;

      case GLUT_MIDDLE_BUTTON:
        downDist = EyeDist;
	downEx = Ex;
	downEy = Ey;
	downEz = Ez;
	downEyeMove = EyeMove;
	EyeMove = 0;
    }

  } else if (state == GLUT_UP && button == downb) {

    downb = -1;

    switch (button) {
      case GLUT_LEFT_BUTTON:
        if (MoveMode != FLYING) {
	  AzSpin =  -dAz;
	  if (AzSpin < MIN_AZSPIN && AzSpin > -MIN_AZSPIN)
	    AzSpin = 0;	
	  ElSpin = -dEl;
	  if (ElSpin < MIN_ELSPIN && ElSpin > -MIN_ELSPIN)
	    ElSpin = 0; 
	}
        AdjustingAzEl = 0;
        MoveOn(1);
	break;

      case GLUT_MIDDLE_BUTTON:
	EyeMove = downEyeMove;
      }
  }
}

 /*
  * change EyeEl and EyeAz and position when mouse is moved w/ button down
  */
void agvHandleMotion(int x, int y)
{
  int deltax = x - downx, deltay = y - downy;

  switch (downb) {
    case GLUT_LEFT_BUTTON:
      EyeEl  = downEl + EL_SENS * ((MoveMode == FLYING) ? -deltay : deltay);
      ConstrainEl();
      EyeAz  = downAz + AZ_SENS * deltax;
      dAz    = PREV_DAZ*dAz + CUR_DAZ*(lastAz - EyeAz);
      dEl    = PREV_DEL*dEl + CUR_DEL*(lastEl - EyeEl);
      lastAz = EyeAz;
      lastEl = EyeEl;
      break;
    case GLUT_MIDDLE_BUTTON:
        EyeDist = downDist + DIST_SENS*deltay;
        Ex = downEx - E_SENS*deltay*sin(TORAD(EyeAz))*cos(TORAD(EyeEl));
        Ey = downEy - E_SENS*deltay*sin(TORAD(EyeEl));
        Ez = downEz + E_SENS*deltay*cos(TORAD(EyeAz))*cos(TORAD(EyeEl));
      break;
  }
  glutPostRedisplay();
}

/***************************************************************/
/********************* KEYBOARD HANDLING ***********************/
/***************************************************************/

  /*
   * set EyeMove (current speed) for FLYING mode
   */
void SetMove(float newmove)
{
  if (newmove > MINMOVE) {
    EyeMove = newmove;
    MoveOn(1);
  } else {
    EyeMove = 0;
    MoveOn(0);
  }
}

  /*
   * 0->9 set speed, +/- adjust current speed  -- in FLYING mode
   */
/* ARGSUSED1 */
void agvHandleKeys(unsigned char key, int x, int y)
{
  if (MoveMode != FLYING)
    return;

  if (key >= '0' && key <= '9')
    SetMove(SPEEDFUNCTION((key-'0')));
  else
    switch(key) {
      case '+':  
        if (EyeMove == 0)
          SetMove(MINMOVE);
         else
	  SetMove(EyeMove *= (1 + MOVEFRACTION));
        break;
      case '-':
	SetMove(EyeMove *= (1 - MOVEFRACTION));
        break;
    }
}

/***************************************************************/
/*********************** VECTOR STUFF **************************/
/***************************************************************/

  /* normalizes v */
void normalize(GLfloat v[3])
{
  GLfloat d = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

  if (d == 0)
    fprintf(stderr, "Zero length vector in normalize\n");
  else
    v[0] /= d; v[1] /= d; v[2] /= d;
}

  /* calculates a normalized crossproduct to v1, v2 */
void ncrossprod(float v1[3], float v2[3], float cp[3])
{
  cp[0] = v1[1]*v2[2] - v1[2]*v2[1];
  cp[1] = v1[2]*v2[0] - v1[0]*v2[2];
  cp[2] = v1[0]*v2[1] - v1[1]*v2[0];
  normalize(cp);
}

/***************************************************************/
/**************************** AXES *****************************/
/***************************************************************/


  /* draw axes -- was helpful to debug/design things */
void agvMakeAxesList(int displaylistnum)
{
  int i,j;
  GLfloat axes_ambuse[] =   { 0.5, 0.0, 0.0, 1.0 };
  glNewList(displaylistnum, GL_COMPILE);
  glPushAttrib(GL_LIGHTING_BIT);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, axes_ambuse);
    glBegin(GL_LINES);
      glVertex3f(15, 0, 0); glVertex3f(-15, 0, 0);
      glVertex3f(0, 15, 0); glVertex3f(0, -15, 0);
      glVertex3f(0, 0, 15); glVertex3f(0, 0, -15);
    glEnd();
    for (i = 0; i < 3; i++) {
      glPushMatrix();
        glTranslatef(-10*(i==0), -10*(i==1), -10*(i==2));
        for (j = 0; j < 21; j++) {
          glutSolidCube(0.1);
          glTranslatef(i==0, i==1, i==2);
	}
      glPopMatrix();
    }
  glPopAttrib();
  glEndList();  
}


