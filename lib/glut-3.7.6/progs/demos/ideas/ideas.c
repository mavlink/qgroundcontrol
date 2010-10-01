
/* Copyright (c) Mark J. Kilgard, 1995. */

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
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#include <sys/timeb.h>
#define gettimeofday(_x, _y)          \
{                                     \
  struct timeb _t;                    \
  ftime(&_t);                         \
  (_x)->tv_sec = _t.time;             \
  (_x)->tv_usec = _t.millitm * 1000;  \
}
#else
#include <sys/time.h>
#endif
/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <stdio.h>
#include <stdlib.h>
#include "objects.h"
#include <GL/glut.h>

#define X 0
#define Y 1
#define Z 2

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEG *M_PI/180.0
#define RAD *180.0/M_PI

float move_speed;		/* Spline distance per second */

int multisample = 0;		/* Antialias polygons? */
int doublebuffer = 1;		/* Doublebuffer? */


#define SPEED_SLOW		0.2	/* Spline distances per second */
#define SPEED_MEDIUM		0.4
#define SPEED_FAST		0.7
#define SPEED_SUPER_FAST	1.0

#define O_NOMS		7
#define O_4MS		8
#define O_8MS		9
#define O_16MS		10

static int RGBA_SB_attributes = GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE;

static int RGBA_DB_attributes = GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE;

float light1_ambient[] = { 0.0,0.0,0.0,1.0 };
float light1_lcolor[] = { 1.0,1.0,1.0,1.0 };
float light1_position[] = { 0.0,1.0,0.0,0.0 };

float light2_ambient[] = { 0.0,0.0,0.0,1.0 };
float light2_lcolor[] = { 0.3,0.3,0.5,1.0 };
float light2_position[] = { -1.0,0.0,0.0,0.0 };

float light3_ambient[] = { 0.2,0.2,0.2,1.0 };
float light3_lcolor[] = { 0.2,0.2,0.2,1.0 };
float light3_position[] = { 0.0,-1.0,0.0,0.0 };

float lmodel_LVW[] = { 0.0 };
float lmodel_ambient[] = { 0.3,0.3,0.3,1.0 };
float lmodel_TWO[] = { GL_TRUE };

float mat_logo_ambient[] = {0.1, 0.1, 0.1, 1.0};
float mat_logo_diffuse[] = {0.5, 0.4, 0.7, 1.0};
float mat_logo_specular[] = {1.0, 1.0, 1.0, 1.0};
float mat_logo_shininess[] = {30.0};

float mat_holder_base_ambient[] = {0.0, 0.0, 0.0, 1.0};
float mat_holder_base_diffuse[] = {0.6, 0.6, 0.6, 1.0};
float mat_holder_base_specular[] = {0.8, 0.8, 0.8, 1.0};
float mat_holder_base_shininess[] = {30.0};

float mat_holder_rings_ambient[] = { 0.0,0.0,0.0,1.0 };
float mat_holder_rings_diffuse[] = { 0.9,0.8,0.0,1.0 };
float mat_holder_rings_specular[] = { 1.0,1.0,1.0,1.0 };
float mat_holder_rings_shininess[] = { 30.0 };

float mat_hemisphere_ambient[] = {0.0, 0.0, 0.0,1.0 };
float mat_hemisphere_diffuse[] = {1.0, 0.2, 0.2,1.0 };
float mat_hemisphere_specular[] = {0.5, 0.5, 0.5,1.0 };
float mat_hemisphere_shininess[] = {20.0};

GLubyte stipple[32*32];

typedef float vector[3];
typedef float vector4[4];
typedef vector parameter[4];

/*
 * Function definitions
 */
static void initialize(void);
static void resize_window(int w, int h);
static void build_table(void);
static parameter *calc_spline_params(vector *ctl_pts, int n);
static void calc_spline(vector v, parameter *params, float current_time);
static void normalize(vector v);
static float dot(vector v1, vector v2);
void draw_table(void);
void draw_logo_shadow(void);
void draw_hemisphere(void);
void draw_logo(void);
void draw_under_table(void);
void draw_i(void);
void draw_d(void);
void draw_e(void);
void draw_a(void);
void draw_s(void);
void draw_n(void);
void draw_m(void);
void draw_o(void);
void draw_t(void);

int post_idle = 0;
static void idle(void);
static void do_post_idle(void);
static void display(void);
static void mouse(int b, int s, int x, int y);
static void keyboard(unsigned char c, int x, int y);
static void vis(int);

static void init_materials(void) {
  int x, y;

  /* Stipple pattern */
  for (y = 0; y < 32; y++)
    for (x = 0; x < 4; x++) 
      stipple[y * 4 + x] = (y % 2) ? 0xaa : 0x55;

    glNewList(MAT_LOGO, GL_COMPILE); 
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_logo_ambient); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_logo_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_logo_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_logo_shininess);
    glEndList(); 

    glNewList( MAT_HOLDER_BASE, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_holder_base_ambient); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_holder_base_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_holder_base_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_holder_base_shininess);
    glEndList();

    glNewList(MAT_HOLDER_RINGS, GL_COMPILE); 
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_holder_rings_ambient); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_holder_rings_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_holder_rings_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_holder_rings_shininess);
    glEndList();

    glNewList(MAT_HEMISPHERE, GL_COMPILE); 
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_hemisphere_ambient); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_hemisphere_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_hemisphere_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_hemisphere_shininess);
    glEndList();

}

void init_lights(void) {
  static float ambient[] = { 0.1, 0.1, 0.1, 1.0 };
  static float diffuse[] = { 0.5, 1.0, 1.0, 1.0 };
  static float position[] = { 90.0, 90.0, 150.0, 0.0 };
  
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, position);

  glLightfv (GL_LIGHT1, GL_AMBIENT, light1_ambient);
  glLightfv (GL_LIGHT1, GL_SPECULAR, light1_lcolor);
  glLightfv (GL_LIGHT1, GL_DIFFUSE, light1_lcolor);
  glLightfv (GL_LIGHT1, GL_POSITION, light1_position);
    
  glLightfv (GL_LIGHT2, GL_AMBIENT, light2_ambient);
  glLightfv (GL_LIGHT2, GL_SPECULAR, light2_lcolor);
  glLightfv (GL_LIGHT2, GL_DIFFUSE, light2_lcolor);
  glLightfv (GL_LIGHT2, GL_POSITION, light2_position);

  glLightfv (GL_LIGHT3, GL_AMBIENT, light3_ambient);
  glLightfv (GL_LIGHT3, GL_SPECULAR, light3_lcolor);
  glLightfv (GL_LIGHT3, GL_DIFFUSE, light3_lcolor);
  glLightfv (GL_LIGHT3, GL_POSITION, light3_position);
  
  glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_LVW);
  glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
}

short dev, val;

float current_time=0.0;
float hold_time=0.0;		/* Used when auto-running */

float tmplight[] = {
    GL_POSITION, 0.0, 0.0, 0.0, 0.0, 
};

GLfloat tv[4][4] = {
  {1.0, 0.0, 0.0, 0.0},
  {0.0, 1.0, 0.0, -1.0},
  {0.0, 0.0, 1.0, 0.0},
  {0.0, 0.0, 0.0, 0.0},
};

#define TABLERES 12

float pcr, pcg, pcb, pca;

vector table_points[TABLERES+1][TABLERES+1];
GLubyte tablecolors[TABLERES+1][TABLERES+1];

vector paper_points[4] = {
    {-0.8, 0.0, 0.4},
    {-0.2, 0.0, -1.4},
    {1.0, 0.0, -1.0},
    {0.4, 0.0, 0.8},
};

float dot(vector, vector);

#define TIME 15
#define START_TIME 0.6

vector light_pos_ctl[] = {

    {0.0, 1.8, 0.0},
    {0.0, 1.8, 0.0},
    {0.0, 1.6, 0.0},

    {0.0, 1.6, 0.0},
    {0.0, 1.6, 0.0},
    {0.0, 1.6, 0.0},
    {0.0, 1.4, 0.0},

    {0.0, 1.3, 0.0},
    {-0.2, 1.5, 2.0},
    {0.8, 1.5, -0.4},
    {-0.8, 1.5, -0.4},

    {0.8, 2.0, 1.0},
    {1.8, 5.0, -1.8},
    {8.0, 10.0, -4.0},
    {8.0, 10.0, -4.0},
    {8.0, 10.0, -4.0},
};

vector logo_pos_ctl[] = {

    {0.0, -0.5, 0.0},

    {0.0, -0.5, 0.0},
    {0.0, -0.5, 0.0},

    {0.0, -0.5, 0.0},
    {0.0, -0.5, 0.0},
    {0.0, -0.5, 0.0},
    {0.0, 0.0, 0.0},

    {0.0, 0.6, 0.0},
    {0.0, 0.75, 0.0},
    {0.0, 0.8, 0.0},
    {0.0, 0.8, 0.0},

    {0.0, 0.5, 0.0},
    {0.0, 0.5, 0.0},
    {0.0, 0.5, 0.0},
    {0.0, 0.5, 0.0},
    {0.0, 0.5, 0.0},
};


vector logo_rot_ctl[] = {

    {0.0, 0.0, -18.4},

    {0.0, 0.0, -18.4},
    {0.0, 0.0, -18.4},

    {0.0, 0.0, -18.4},
    {0.0, 0.0, -18.4},
    {0.0, 0.0, -18.4},
    {0.0, 0.0, -18.4},
    {0.0, 0.0, -18.4},

/*    {90.0, 0.0, -90.0},
    {180.0, 180.0, 90.0}, */
    {240.0, 360.0, 180.0},
    {90.0, 180.0, 90.0},

    {11.9, 0.0, -18.4},
    {11.9, 0.0, -18.4},
    {11.9, 0.0, -18.4},
    {11.9, 0.0, -18.4},
    {11.9, 0.0, -18.4},
};


vector view_from_ctl[] = {

    {-1.0, 1.0, -4.0},

    {-1.0, -3.0, -4.0},	/* 0 */
    {-3.0, 1.0, -3.0},	/* 1 */

    {-1.8, 2.0, 5.4},	/* 2 */
    {-0.4, 2.0, 1.2},	/* 3 */
    {-0.2, 1.5, 0.6},	/* 4 */
    {-0.2, 1.2, 0.6},	/* 5 */

    {-0.8, 1.0, 2.4},	/* 6 */
    {-1.0, 2.0, 3.0},	/* 7 */
    {0.0, 4.0, 3.6},	/* 8 */
    {-0.8, 4.0, 1.2},	/* 9 */

    {-0.2, 3.0, 0.6},	/* 10 */
    {-0.1, 2.0, 0.3},	/* 11 */
    {-0.1, 2.0, 0.3},	/* 12 */
    {-0.1, 2.0, 0.3},	/* 13 */
    {-0.1, 2.0, 0.3},	/* 13 */


};

vector view_to_ctl[] = {

    {-1.0, 1.0, 0.0},

    {-1.0, -3.0, 0.0},
    {-1.0, 1.0, 0.0},

    {0.1, 0.0, -0.3},
    {0.1, 0.0, -0.3},
    {0.1, 0.0, -0.3},
    {0.0, 0.2, 0.0},

    {0.0, 0.6, 0.0},
    {0.0, 0.8, 0.0},
    {0.0, 0.8, 0.0},
    {0.0, 0.8, 0.0},

    {0.0, 0.8, 0.0},
    {0.0, 0.8, 0.0},
    {0.0, 0.8, 0.0},
    {0.0, 0.8, 0.0},
    {0.0, 0.8, 0.0},

};


vector view_from, view_to, logo_pos, logo_rot;
vector4 light_pos;

parameter *view_from_spline, *view_to_spline,
	  *light_pos_spline, *logo_pos_spline,
	  *logo_rot_spline;

double a3, a4;

void ideas_usage(void)
{
  fprintf(stderr, "Usage: ideas [-a] [-m] [-d] -s{1-4}\n");
  fprintf(stderr, "Press ESC to quit, 1-4 to control speed, any other key\n");
  fprintf(stderr, "to pause.\n");
}

  int auto_run;		/* If set, then automatically run forever */
  float new_speed;	/* Set new animation speed? */
  int timejerk;		/* Set to indicate time jerked! (menu pulled down) */
  int paused = 0;	/* Paused? */
  int right = 0;	/* Draw right eye? */
  int resetclock;	/* Reset the clock? */
  float timeoffset;	/* Used to compute timing */
  struct timeval start;

int main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);

  auto_run = 0;	/* Don't automatically run forever */
  /* .4 spline distance per second by default */
  move_speed = SPEED_MEDIUM;
  new_speed = SPEED_MEDIUM;
  timeoffset = START_TIME;
  
  for (i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      break;
    }
    
    switch(argv[i][1]) {
    case 'a':	/* Keep running forever */
      auto_run = 1;
      break;
    case 'm':	/* Multisample */
      multisample = 1;
      break;
    case 'd':	/* Single buffer */
      doublebuffer = 0;
      break;
    case 's':
      switch(argv[i][2]) {
      case '1':
	move_speed = new_speed = SPEED_SLOW;
	break;
      case '2':
	move_speed = new_speed = SPEED_MEDIUM;
	break;
      case '3':
	move_speed = new_speed = SPEED_FAST;
	break;
      case '4':
	move_speed = new_speed = SPEED_SUPER_FAST;
	break;
      }
      break;
    default:
      ideas_usage();
      break;
    }
  }
  
  initialize();
  
  current_time = timeoffset;
  resetclock = 1;
  timejerk = 0;
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

static void idle(void) 
{
    if ((current_time) > (TIME*1.0)-3.0) {
      if (auto_run) {
	hold_time += current_time - (TIME - 3.001);
	if (hold_time > 3.0) {	/* 3 second hold */
	  hold_time = 0.0;
	  resetclock = 1;
	}
      } else {
        if(!resetclock) glutIdleFunc(NULL);
      }
      current_time = (TIME*1.0)-3.001;
    } else {
       post_idle = 1;
    }
    glutPostRedisplay();
}

/* ARGSUSED2 */
static void
mouse(int b, int s, int x, int y)
{
   if(b == GLUT_LEFT_BUTTON && s == GLUT_DOWN) {
      resetclock = 1;
      paused = 0;
      glutIdleFunc(idle);
   }
}

/* ARGSUSED1 */
static void
keyboard(unsigned char c, int x, int y)
{
   switch(c) {
   case 27:
      exit(0);
      break;
   case '1':
      new_speed = SPEED_SLOW;
      break;
   case '2':
      new_speed = SPEED_MEDIUM;
      break;
   case '3':
      new_speed = SPEED_FAST;
      break;
   case '4':
      new_speed = SPEED_SUPER_FAST;
      break;
   default:
      if (paused) timejerk = 1;
      paused = ~paused;
      if(paused) {
	 glutIdleFunc(NULL);
      } else {
	 glutIdleFunc(idle);
      }
   }
}

static void
vis(int visible)
{
  if (visible == GLUT_VISIBLE) {
      if(!paused) glutIdleFunc(idle);
      do_post_idle();
  } else {
      if(!paused) glutIdleFunc(NULL);
  }
}

static void display(void)
{
  float x, y, z, c;

    calc_spline(view_from, view_from_spline, current_time);
    calc_spline(view_to, view_to_spline, current_time);
    calc_spline(light_pos, light_pos_spline, current_time);
    light_pos[3] = 0.0;
    calc_spline(logo_pos, logo_pos_spline, current_time);
    calc_spline(logo_rot, logo_rot_spline, current_time);
    
    tmplight[1] = light_pos[X] - logo_pos[X];
    tmplight[2] = light_pos[Y] - logo_pos[Y];
    tmplight[3] = light_pos[Z] - logo_pos[Z];
    
    glNewList(LIGHT_TMP, GL_COMPILE); 
    glMaterialf(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, * tmplight); 
    glEndList();
    
    tv[0][0] = tv[1][1] = tv[2][2] = light_pos[Y];
    
    glColor3ub(0,  0,  0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    
    /*
     * SHADOW
     */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(view_from[X], view_from[Y], view_from[Z], 
	      view_to[X], view_to[Y], view_to[Z],
	      0.0, 1.0, 0.0);
    
    if (view_from[Y] > 0.0) draw_table();

    glEnable(GL_CULL_FACE); 
    glDisable(GL_DEPTH_TEST); 

    if (logo_pos[Y] < 0.0) {
      
      if (logo_pos[Y]>-0.33) {
	/* We're emerging from the table */
	c = 1.0 - (logo_pos[Y]) / -0.33;
	pca /= 4.0;
	glColor3ub((GLubyte)(128.0*(1.0-c)*0.5 + 255.0*pca*c),
		   (GLubyte)(102.0*(1.0-c)*0.5 + 255.0*pca*c),
		   (GLubyte)(179.0*(1.0-c)*0.5 + 200.0*pca*c));
      } else {
	/* Still under table */
	glColor3ub(128/2,  102/2,  179/2);
      }
      
      glPushMatrix();
      glScalef(0.04,  0.0,  0.04);
      glRotatef(0.1 * (-900), 1.0, 0.0, 0.0);
      glRotatef(0.1 * ((int)(10.0*logo_rot[Z])), 0.0, 0.0, 1.0);
      glRotatef(0.1 * ((int)(10.0*logo_rot[Y])), 0.0, 1.0, 0.0);
      glRotatef(0.1 * ((int)(10.0*logo_rot[X])), 1.0, 0.0, 0.0);
      glRotatef(0.1 * (353), 1.0, 0.0, 0.0);
      glRotatef(0.1 * (450), 0.0, 1.0, 0.0);
      draw_logo_shadow();
      glPopMatrix();
    }
    
    if (logo_pos[Y] > 0.0) {
      glPushMatrix();
      if (logo_pos[Y]<0.33) {
	pca /= 4.0;
	c = 1.0 - (logo_pos[Y])/0.33;
	glColor3ub((GLubyte)(255.0*pca*c),
		   (GLubyte)(255.0*pca*c),
		   (GLubyte)(200.0*pca*c));
      } else {
	glColor3ub(0, 0, 0);
      }
      
      glTranslatef(light_pos[X],  light_pos[Y],  light_pos[Z]);
      glMultMatrixf(&tv[0][0]);
      glTranslatef(-light_pos[X]+logo_pos[X],
		   -light_pos[Y]+logo_pos[Y],
		   -light_pos[Z]+logo_pos[Z]);
      glScalef(0.04,  0.04,  0.04);
      glRotatef (0.1 * (-900), 1.0, 0.0, 0.0);
      glRotatef (0.1 * ((int)(10.0*logo_rot[Z])), 0.0, 0.0, 1.0);
      glRotatef (0.1 * ((int)(10.0*logo_rot[Y])), 0.0, 1.0, 0.0);
      glRotatef (0.1 * ((int)(10.0*logo_rot[X])), 1.0, 0.0, 0.0);
      glRotatef (0.1 * (353), 1.0, 0.0, 0.0);
      glRotatef (0.1 * (450), 0.0, 1.0, 0.0);


      glEnable(GL_POLYGON_STIPPLE);
      glPolygonStipple(stipple);
      draw_logo_shadow();
      glDisable(GL_POLYGON_STIPPLE);
      glPopMatrix();
    }
    /*
     * DONE SHADOW 
     */


    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(.1*(450),  5.0/4.0,  0.5,  20.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(view_from[X],  view_from[Y],  view_from[Z],
	      view_to[X],  view_to[Y],  view_to[Z], 
	      0.0, 1.0, 0.0);
    
    glCallList( MAT_HOLDER_RINGS); 
    
    glPushMatrix();
    glTranslatef(light_pos[X],  light_pos[Y],  light_pos[Z]);
    glScalef(0.1,  0.1,  0.1);
    
    x = light_pos[X] - logo_pos[X];
    y = light_pos[Y] - logo_pos[Y];
    z = light_pos[Z] - logo_pos[Z];
    
    if (x!=0.0) {
      a3 = -atan2(z, x)*10.0 RAD;
    } else a3 = 0.0;
    
    a4 = -atan2(sqrt(x*x + z*z), y)*10.0 RAD;
    
    glRotatef (0.1 * ((int)a3), 0.0, 1.0, 0.0);
    glRotatef (0.1 * ((int)a4), 0.0, 0.0, 1.0);
    glRotatef (0.1 * (-900), 1.0, 0.0, 0.0);
    
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
    glCallList(MAT_HEMISPHERE);
    glEnable(GL_NORMALIZE);
    draw_hemisphere();
    glDisable(GL_NORMALIZE);
    glPopMatrix();

    glDisable(GL_LIGHT2);
    glDisable(GL_LIGHT3); 
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, light_pos);
    
    if (logo_pos[Y] > -0.33) {

      glCallList(MAT_LOGO);
    
      glPushMatrix();
      glTranslatef(logo_pos[X],  logo_pos[Y],  logo_pos[Z]);
      glScalef(0.04,  0.04,  0.04);
      glRotatef (0.1 * (-900), 1.0, 0.0, 0.0);
      glRotatef (0.1 * ((int)(10.0*logo_rot[Z])), 0.0, 0.0, 1.0);
      glRotatef (0.1 * ((int)(10.0*logo_rot[Y])), 0.0, 1.0, 0.0);
      glRotatef (0.1 * ((int)(10.0*logo_rot[X])), 1.0, 0.0, 0.0);
      glRotatef (0.1 * (353), 1.0, 0.0, 0.0);
      glRotatef (0.1 * (450), 0.0, 1.0, 0.0);
      glEnable(GL_LIGHTING);
      draw_logo();
      glPopMatrix();
    }
    
    if (view_from[Y] < 0.0) draw_under_table();
    
    glutSwapBuffers();

    if(post_idle) do_post_idle();
}

static void do_post_idle(void)
{
  struct timeval current;
  float timediff;	

    /* Time jerked -- adjust clock appropriately */
    if (timejerk) {
      timejerk = 0;
      timeoffset = current_time;
      gettimeofday(&start, NULL);
    }
    
    /* Reset our timer */
    if (resetclock) {
      resetclock = 0;
      paused = 0;
      timeoffset = START_TIME;
      gettimeofday(&start, NULL);
    }
    
    /* Compute new time */
    gettimeofday(&current, NULL);
    timediff = (current.tv_sec - start.tv_sec) + 
      ((double) (current.tv_usec - start.tv_usec)) / 1000000.0;
    if (!paused) {
       current_time = timediff * move_speed + timeoffset;
    }
    
    /* Adjust to new speed */
    if (new_speed != move_speed) {
      move_speed = new_speed;
      timeoffset = current_time;
      gettimeofday(&start, NULL);
    }
    post_idle = 0;
}

static void resize_window(int w, int h) 
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective (45.0, 5.0/4.0, 0.5, 20.0); 
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(0, 0, w, h);
}

static void initialize(void)
{
    int attr;

    attr = doublebuffer ? RGBA_DB_attributes : RGBA_SB_attributes;
    glutInitDisplayMode(attr);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Ideas");

    if (multisample) glEnable(GL_POLYGON_SMOOTH); 
    
    init_lights();
    init_materials();

    build_table();

    view_from_spline = calc_spline_params(view_from_ctl, TIME);
    view_to_spline = calc_spline_params(view_to_ctl, TIME);
    light_pos_spline = calc_spline_params(light_pos_ctl, TIME);
    logo_pos_spline = calc_spline_params(logo_pos_ctl, TIME);
    logo_rot_spline = calc_spline_params(logo_rot_ctl, TIME);

    glutReshapeFunc(resize_window);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutVisibilityFunc(vis);

    glMatrixMode(GL_MODELVIEW);
}


static void build_table(void) 
{
    float i, j;

    for (j=0.0; j<=TABLERES*1.0; j+=1.0) {
	for (i=0.0; i<=TABLERES*1.0; i+=1.0) {
	    table_points[(int)j][(int)i][Z] = (i-TABLERES*1.0/2.0)/2.0;
	    table_points[(int)j][(int)i][X] = (j-TABLERES*1.0/2.0)/2.0;
	    table_points[(int)j][(int)i][Y] = 0.0;
	}
    }
}


void draw_table(void)
{
    float c;
    int i, j;
    int k, l;
    float ov[3], lv[3];

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    ov[X] = light_pos[X]-logo_pos[X];
    ov[Y] = light_pos[Y]-logo_pos[Y];
    ov[Z] = light_pos[Z]-logo_pos[Z];

    normalize(ov);

    for (j=0; j<=TABLERES; j++) {
      for (i=0; i<=TABLERES; i++) {
	lv[X] = light_pos[X] - table_points[j][i][X];
	lv[Y] = light_pos[Y] - table_points[j][i][Y];
	lv[Z] = light_pos[Z] - table_points[j][i][Z];
	normalize(lv);
	if ((c = dot(lv, ov))<0.0) c = 0.0;
	c = c * c * c * lv[Y] * 255.0;
	/* fade */
	if ((current_time>TIME-5.0) && (current_time<TIME-3.0)) 
	  c *= 1.0 - (current_time-(TIME-5.0)) * 0.5;
	
	tablecolors[j][i] = (int)c;
      }
    }
    
    
    for (l=0; l<TABLERES; l++) {
      
      glBegin(GL_TRIANGLE_STRIP);
      for (k=0; k<=TABLERES; k++) {
	glColor3ub(tablecolors[l][k],
		   tablecolors[l][k],
		   tablecolors[l][k]);
	glVertex3fv(table_points[l][k]);

	glColor3ub(tablecolors[l+1][k],
		   tablecolors[l+1][k], 
		   tablecolors[l+1][k]);
	glVertex3fv(table_points[l+1][k]);
	
      }
	glEnd();
    }

    if (logo_pos[Y]>-0.33 && logo_pos[Y]<0.33) {
	glEnable(GL_DEPTH_TEST);
    }

    pca = 0.0;
    glBegin(GL_POLYGON);
    for (i=0; i<4; i++) {
      lv[X] = light_pos[X] - paper_points[i][X];
      lv[Y] = light_pos[Y] - paper_points[i][Y];
      lv[Z] = light_pos[Z] - paper_points[i][Z];
      normalize(lv);
      if ((c = dot(lv, ov))<0.0) c = 0.0;
      c = c * c * c * lv[Y];
      /* fade */
      if ((current_time>TIME-5.0) && (current_time<TIME-3.0)) 
	c *= 1.0 - (current_time-(TIME-5.0)) * 0.5;
      
      pcr = c * 255; pcg = c * 255; pcb = c * 200;
      pca += c;
      glColor3ub((GLubyte)pcr,  (GLubyte)pcg,  (GLubyte)pcb);
      glVertex3fv(paper_points[i]);
    }
    glEnd();

    glPushMatrix();
    glRotatef (0.1 * (-184), 0.0, 1.0, 0.0);
    glTranslatef(-0.3, 0.0, -0.8);
    glRotatef (0.1 * (-900), 1.0, 0.0, 0.0);
    glScalef(0.015, 0.015, 0.015);


    if (current_time>TIME*1.0-5.0) {
	c = (current_time-(TIME*1.0-5.0))/2.0;
	glColor3ub((GLubyte)(c*255.0),  (GLubyte)(c*255.0),  (GLubyte)(c*255.0));
    } else glColor3ub(0,  0,  0);

    glDisable(GL_DEPTH_TEST);

    draw_i();
    glTranslatef(3.0,  0.0,  0.0);

    draw_d();
    glTranslatef(6.0,  0.0,  0.0);

    draw_e();
    glTranslatef(5.0,  0.0,  0.0);

    draw_a();
    glTranslatef(6.0,  0.0,  0.0);

    draw_s();
    glTranslatef(10.0,  0.0,  0.0);

    draw_i();
    glTranslatef(3.0,  0.0,  0.0);

    draw_n();
    glTranslatef(-31.0,  -13.0,  0.0);

    draw_m();
    glTranslatef(10.0,  0.0,  0.0);

    draw_o();
    glTranslatef(5.0,  0.0,  0.0);

    draw_t();
    glTranslatef(4.0,  0.0,  0.0);

    draw_i();
    glTranslatef(3.5,  0.0,  0.0);

    draw_o();
    glTranslatef(5.0,  0.0,  0.0);

    draw_n();

    glPopMatrix();

}



void draw_under_table(void) 
{
    int k, l;

    glDisable(GL_DEPTH_TEST);


    glColor3ub(0,  0,  0);

    for (l=0; l<TABLERES; l++) {

	glBegin(GL_TRIANGLE_STRIP);
	for (k=0; k<=TABLERES; k++) {

	    glVertex3fv(table_points[l][k]);
	    glVertex3fv(table_points[l+1][k]);

	}
	glEnd();
    }

    glEnable(GL_DEPTH_TEST); 

}

static void calc_spline(vector v, parameter *params, float current_time)
{

    float t;
    int ti, i;

    t = current_time - (float)((int)current_time);

    ti = current_time;
    /* XXX Hack so that time will not overflow the params array.
       The size of the spline params array should not be built into
       this routine this way. */
    if (ti >= (TIME - 3)) {
      ti = TIME - 4;
    }

    for (i=0; i<3; i++) {
	v[i] = params[ti][3][i] +
	       params[ti][2][i] * t +
	       params[ti][1][i] * t * t +
	       params[ti][0][i] * t * t * t;
    }

}

static parameter *calc_spline_params(vector *ctl_pts, int n)
{

    int i, j;
    parameter *params;

    if (n<4) {
	fprintf(stderr,
	    "calc_spline_params: not enough control points\n");
	return (NULL);
    }

    params = (parameter *)malloc(sizeof(parameter) * (n-3));

    for (i=0; i<n-3; i++) {

	for (j=0; j<3; j++) {

	    params[i][3][j] = ctl_pts[i+1][j];
	    params[i][2][j] = ctl_pts[i+2][j] - ctl_pts[i][j];
	    params[i][1][j] =  2.0 * ctl_pts[i][j] +
			      -2.0 * ctl_pts[i+1][j] +
			       1.0 * ctl_pts[i+2][j] +
			      -1.0 * ctl_pts[i+3][j];
	    params[i][0][j] = -1.0 * ctl_pts[i][j] +
			       1.0 * ctl_pts[i+1][j] +
			      -1.0 * ctl_pts[i+2][j] +
			       1.0 * ctl_pts[i+3][j];

	}
    }

    return (params);
}


static void normalize(vector v)
{
    float r;

    r = sqrt(v[X]*v[X] + v[Y]*v[Y] + v[Z]*v[Z]);

    v[X] /= r;
    v[Y] /= r;
    v[Z] /= r;
}


static float dot(vector v1, vector v2)
{
    return v1[X]*v2[X]+v1[Y]*v2[Y]+v1[Z]*v2[Z];
}

