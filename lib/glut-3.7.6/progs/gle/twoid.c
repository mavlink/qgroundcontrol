/* 
 * twistoid.c
 *
 * FUNCTION:
 * Show extrusion of open contours. Also, show how torsion is applied.
 *
 * HISTORY:
 * -- linas Vepstas October 1991
 * -- heavily modified to draw corrugated surface, Feb 1993, Linas
 * -- modified to demo twistoid March 1993
 * -- port to glut Linas Vepstas March 1995
 */

/* required include files */
#include <math.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/tube.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int mx = 121;
static int my = 121;

#define OPENGL_10
/* =========================================================== */

#define NUM_TOID1_PTS 5
double toid1_points[NUM_TOID1_PTS][3];
float toid1_colors [NUM_TOID1_PTS][3];
double toid1_twists [NUM_TOID1_PTS];

#define TSCALE (6.0)

#define TPTS(x,y) {				\
   toid1_points[i][0] = TSCALE * (x);		\
   toid1_points[i][1] = TSCALE * (y);		\
   toid1_points[i][2] = TSCALE * (0.0);		\
   i++;						\
}

#define TCOLS(r,g,b) {				\
   toid1_colors[i][0] = (r);			\
   toid1_colors[i][1] = (g);			\
   toid1_colors[i][2] = (b);			\
   i++;						\
}

#define TXZERO() {				\
   toid1_twists[i] = 0.0;			\
   i++;						\
}

void init_toid1_line (void)
{
   int i;

   i=0;
   TPTS (-1.1, 0.0);
   TPTS (-1.0, 0.0);
   TPTS (0.0, 0.0);
   TPTS (1.0, 0.0);
   TPTS (1.1, 0.0);

   i=0;
   TCOLS (0.8, 0.8, 0.5);
   TCOLS (0.8, 0.4, 0.5);
   TCOLS (0.8, 0.8, 0.3);
   TCOLS (0.4, 0.4, 0.5);
   TCOLS (0.8, 0.8, 0.5);

   i=0;
   TXZERO ();
   TXZERO ();
   TXZERO ();
   TXZERO ();
   TXZERO ();
}

/* =========================================================== */

#define SCALE 0.6
#define TWIST(x,y) {						\
   double ax, ay, alen;						\
   twistation[i][0] = SCALE * (x);				\
   twistation[i][1] = SCALE * (y);				\
   if (i!=0) {							\
      ax = twistation[i][0] - twistation[i-1][0];		\
      ay = twistation[i][1] - twistation[i-1][1];		\
      alen = 1.0 / sqrt (ax*ax + ay*ay);			\
      ax *= alen;   ay *= alen;					\
      twist_normal [i-1][0] = - ay;				\
      twist_normal [i-1][1] = ax;				\
   }								\
   i++;								\
}

#define NUM_TWIS_PTS (20)

double twistation [NUM_TWIS_PTS][2];
double twist_normal [NUM_TWIS_PTS][2];

void init_tripples (void)
{
   int i;
   double angle;
   double co, si;

   /* outline of extrusion */
   i=0;
   /* first, draw a semi-curcular "hump" */
   while (i< 11) {
      angle = M_PI * ((double) i) / 10.0;
      co = cos (angle);
      si = sin (angle);
      TWIST ((-7.0 -3.0*co), 1.8*si);
   }

   /* now, a zig-zag corrugation */
   while (i< NUM_TWIS_PTS) {
      TWIST ((-10.0 +(double) i), 0.0);
      TWIST ((-9.5 +(double) i), 1.0);
   }
}

   
/* =========================================================== */

#define V3F(x,y,z) {					\
	float vvv[3]; 					\
	vvv[0] = x; vvv[1] = y; vvv[2] = z; v3f (vvv); 	\
}

#define N3F(x,y,z) {					\
	float nnn[3]; 					\
	nnn[0] = x; nnn[1] = y; nnn[2] = z; n3f (nnn); 	\
}

/* =========================================================== */

void draw_twist (void) {
   int i;

   toid1_twists[2] = (double) (mx-121) / 8.0;

   i=3;
/*
   TPTS (1.0, ((double)my) /400.0);
   TPTS (1.1, 1.1 * ((double)my) / 400.0);
*/
   TPTS (1.0, -((double)(my-121)) /200.0);
   TPTS (1.1, -1.1 * ((double)(my-121)) / 200.0);

#ifdef IBM_GL_32
   rotate (230, 'x');
   rotate (230, 'y');
   scale (1.8, 1.8, 1.8);

   if (mono_color) {
      RGBcolor (178, 178, 204);
      twist_extrusion (NUM_TWIS_PTS, twistation, twist_normal, 
                NULL, NUM_TOID1_PTS, toid1_points, NULL, toid1_twists);
   } else {
      twist_extrusion (NUM_TWIS_PTS, twistation, twist_normal, 
              NULL, NUM_TOID1_PTS, toid1_points, toid1_colors, toid1_twists);
   }
#endif

#ifdef OPENGL_10
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotated (43.0, 1.0, 0.0, 0.0);
   glRotated (43.0, 0.0, 1.0, 0.0);
   glScaled (1.8, 1.8, 1.8);
   gleTwistExtrusion (NUM_TWIS_PTS, twistation, twist_normal, 
              NULL, NUM_TOID1_PTS, toid1_points, NULL, toid1_twists);
   glPopMatrix ();
   glutSwapBuffers ();
#endif

}

/* =========================================================== */

void init_twist (void)
{
   int js;

   init_toid1_line ();
   init_tripples ();

#ifdef IBM_GL_32
   js = getjoinstyle ();
   js &= ~TUBE_CONTOUR_CLOSED;
   setjoinstyle (js);
#endif

#ifdef OPENGL_10
   js = gleGetJoinStyle ();
   js &= ~TUBE_CONTOUR_CLOSED;
   gleSetJoinStyle (js);
#endif

}

/* get notified of mouse motions */
void MouseMotion (int x, int y)
{
   mx = x;
   my = y;
   glutPostRedisplay ();
}

void JoinStyle (int msg) 
{
   int style;
   /* get the current joint style */
   style = gleGetJoinStyle ();

   /* there are four different join styles, 
    * and two different normal vector styles */
   switch (msg) {
      case 0:
         style &= ~TUBE_JN_MASK;
         style |= TUBE_JN_RAW;
         break;
      case 1:
         style &= ~TUBE_JN_MASK;
         style |= TUBE_JN_ANGLE;
         break;
      case 2:
         style &= ~TUBE_JN_MASK;
         style |= TUBE_JN_CUT;
         break;
      case 3:
         style &= ~TUBE_JN_MASK;
         style |= TUBE_JN_ROUND;
         break;

      case 20:
         style &= ~TUBE_NORM_MASK;
         style |= TUBE_NORM_FACET;
         break;
      case 21:
         style &= ~TUBE_NORM_MASK;
         style |= TUBE_NORM_EDGE;
         break;

      case 99:
         exit (0);

      default:
         break;
   }
   gleSetJoinStyle (style);
   glutPostRedisplay ();
}

/* set up a light */
GLfloat lightOnePosition[] = {40.0, 40, 100.0, 0.0};
GLfloat lightOneColor[] = {0.89, 0.89, 0.89, 1.0}; 

GLfloat lightTwoPosition[] = {-40.0, 40, 100.0, 0.0};
GLfloat lightTwoColor[] = {0.89, 0.89, 0.89, 1.0}; 

GLfloat material[] = {0.93, 0.79, 0.93, 1.0}; 

int
main (int argc, char * argv[]) {

   /* initialize glut */
   glutInit (&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutCreateWindow ("twistoid");
   glutDisplayFunc (draw_twist);
   glutMotionFunc (MouseMotion);

   /* create popup menu */
   glutCreateMenu (JoinStyle);
   glutAddMenuEntry ("Raw Join Style", 0);
   glutAddMenuEntry ("Angle Join Style", 1);
   glutAddMenuEntry ("Cut Join Style", 2);
   glutAddMenuEntry ("Round Join Style", 3);
   glutAddMenuEntry ("------------------", 9999);
   glutAddMenuEntry ("Facet Normal Vectors", 20);
   glutAddMenuEntry ("Edge Normal Vectors", 21);
   glutAddMenuEntry ("------------------", 9999);
   glutAddMenuEntry ("Exit", 99);
   glutAttachMenu (GLUT_MIDDLE_BUTTON);

   /* initialize GL */
   glClearDepth (1.0);
   glEnable (GL_DEPTH_TEST);
   glClearColor (0.2, 0.2, 0.2, 0.0);
   glShadeModel (GL_SMOOTH);

   glMatrixMode (GL_PROJECTION);
   /* roughly, measured in centimeters */
   glFrustum (-9.0, 9.0, -9.0, 9.0, 50.0, 150.0);
   glMatrixMode(GL_MODELVIEW);

   /* initialize lighting */
   glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, material);
   glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, material);
   glLightfv (GL_LIGHT0, GL_POSITION, lightOnePosition);
   glLightfv (GL_LIGHT0, GL_DIFFUSE, lightOneColor);
   glLightfv (GL_LIGHT0, GL_SPECULAR, lightOneColor);
   glEnable (GL_LIGHT0);
   glLightfv (GL_LIGHT1, GL_POSITION, lightTwoPosition);
   glLightfv (GL_LIGHT1, GL_DIFFUSE, lightTwoColor);
   glEnable (GL_LIGHT1);
   glEnable (GL_LIGHTING);
/*
   glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable (GL_COLOR_MATERIAL);
*/

   init_twist ();

   glutMainLoop ();
   return 0;             /* ANSI C requires main to return int. */
}
/* ------------------ end of file -------------------- */
