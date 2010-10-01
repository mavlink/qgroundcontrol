
/* 
 * candlestick.c
 * 
 * FUNCTION:
 * Draws a skewed candlestick shape using the Lathe primitive
 *
 * HISTORY:
 * -- created by Linas Vepstas October 1991
 * -- C++ and OO playing around Linas Vepstas June 1993
 * -- converted to use GLUT -- December 1995, Linas
 *
 */

/* required include files */
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <GL/tube.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================== */

#define SET_RGB(rgb,r,g,b) {			\
	rgb[0]=r; rgb[1]=g; rgb[2]=b;		\
} 

typedef struct _material {

   /* public data areas */
   float emission[3];	
   float ambient[3];  
   float diffuse[3];  
   float specular[3]; 
   float shininess;

} Material;

#define SET_EMIS(self,r,g,b) { SET_RGB(self->emission, r,g,b); }
#define SET_AMB(self,r,g,b) { SET_RGB(self->ambient, r,g,b); }
#define SET_DIFF(self,r,g,b) { SET_RGB(self->diffuse, r,g,b); }
#define SET_SPEC(self,r,g,b) { SET_RGB(self->specular, r,g,b); }

/* =========================================================== */

#ifdef NOTNOW
class goPolyline {
   public:
      int dimension;
      int numPoints;
      double * pts;

   private:
      int  nfree;

   public:
      goPolyline ();		// by default, construct 3D polyline
      goPolyline (int);            // construct arbitrary dimension polyline
      void Print ();
      void AddPoint (double x, double y);
      void AddNormal (double x, double y);
      void MakeFacetNormal ();
};
#endif /* NOTNOW */

typedef double SVec[2];

typedef struct contour {

   /* public data areas */
   int numContourPoints;
   int numContourNorms;
   SVec * pts;
   SVec * norms;
   double up[3];

} Contour;

#define pfree numContourPoints
#define nfree numContourNorms

#define NEW_CONTOUR(self) {			\
   self -> pts = (SVec *) malloc (100*sizeof (double));	\
   self -> norms = (SVec *) malloc (100*sizeof (double));	\
   self -> pfree = 0;		\
   self -> nfree = 0;		\
}

#define ADD_POINT(self,x,y) { 			\
   self -> pts[self->pfree][0] = x;		\
   self -> pts[self->pfree][1] = y;		\
   self->pfree ++;				\
}

#define ADD_NORMAL(self,x,y) { 			\
   self -> norms[self->nfree][0] = x;		\
   self -> norms[self->nfree][1] = y;		\
   self->nfree ++;				\
}

#define MAKE_NORMAL(self) {			\
   float dx, dy, w;				\
   dx = self -> pts [self->pfree -1][0];	\
   dx -= self -> pts [self->pfree -2][0];	\
   dy = self -> pts [self->pfree -1][1];	\
   dy -= self -> pts [self->pfree -2][1];	\
   w = 1.0 / sqrt (dx*dx+dy*dy);		\
   dx *= w;					\
   dy *= w;					\
   self -> norms[self->nfree][0] = -dy;		\
   self -> norms[self->nfree][1] = dx;		\
   self -> nfree ++;				\
}

/* =========================================================== */
/* class gleExtrustion */

typedef struct _extrusion {
   Material	*material;	/* material description */
   Contour 	*contour;	/* 2D contour */

   double radius;		/* for polycylinder, torus */
   double startRadius;     /* spiral starts in x-y plane */
   double drdTheta;        /* change in radius per revolution */
   double startZ;          /* starting z value */
   double dzdTheta;        /* change in Z per revolution */
   double startXform[2][3]; /* starting contour affine xform */
   double dXdTheta[2][3]; /* tangent change xform per revoln */
   double startTheta;      /* start angle in x-y plane */
   double sweepTheta;     /* degrees to spiral around */

} Extrusion;

#define  NEW_EXTRUSION(self) {		\
   self -> material = (Material *) malloc (sizeof (Material));	\
   self -> contour = (Contour *) malloc (sizeof (Contour));	\
   NEW_CONTOUR (self->contour);					\
}
	

/* =========================================================== */
Extrusion *candle = NULL;

/* =========================================================== */
float lastx=0;
float lasty=0;

void draw_candle (void) {

   /* attach the mouse */
   candle->dzdTheta = - 0.015 * (lasty -150.0);

/* rotational delta sine & cosines from mouse */
/* disable twist -- confusing to the viewer, hard to explain */
/*
   mouse -> AttachMouseYd (mouse, 0.0004, -0.1, &candle->dXdTheta[0][1]);
*/
   glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, candle->material->ambient);
   glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, candle->material->diffuse);
   glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, candle->material->specular);
   glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 2.0);

#ifdef IBM_GL_32
   rotate (-750, 'x');
   rotate (-1050, 'z');
   translate (-0.5, -0.5, -0.5);

   lathe (candle->contour->numContourPoints,
           candle->contour->pts,
           candle->contour->norms,
           candle->contour->up,
           candle->startRadius,    /* donut radius */
           candle->drdTheta,   /* change in donut radius per revolution */
           candle->startZ,    /* start z value */
           candle->dzdTheta,     /* change in Z per revolution */
           candle->startXform, 
           candle->dXdTheta,
           candle->startTheta,     /* start angle */
           candle->sweepTheta);     /* sweep angle */
#endif


#define OPENGL_10
#ifdef OPENGL_10
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotated (-75.0, 1.0, 0.0, 0.0);
   glRotated (-105.0, 0.0, 0.0, 1.0);

   glEnable (GL_LIGHTING);
   gleLathe (candle->contour->numContourPoints,
           candle->contour->pts,
           candle->contour->norms,
           candle->contour->up,
           candle->startRadius,    /* donut radius */
           candle->drdTheta,   /* change in donut radius per revolution */
           candle->startZ,    /* start z value */
           candle->dzdTheta,     /* change in Z per revolution */
           candle->startXform, 
           candle->dXdTheta,
           candle->startTheta,     /* start angle */
           candle->sweepTheta);     /* sweep angle */

   glDisable (GL_LIGHTING);

   glPopMatrix ();
   glutSwapBuffers ();
#endif 
}

/* =========================================================== */

#define SCALE 1.0
#define PT(x,y) { ADD_POINT (candle->contour, SCALE*x, SCALE*y); }
#define NORM(x,y) { ADD_NORMAL (candle->contour, x, y); }
#define FACET { MAKE_NORMAL (candle->contour); }

/* =========================================================== */

void init_candle (void)
{
   int j;
   double theta, dtheta;
   int style;

   candle = (Extrusion *) malloc (sizeof (Extrusion));
   NEW_EXTRUSION (candle);

   /* define candle color */
   SET_AMB (candle->material, 0.25, 0.25, 0.25);
   SET_DIFF (candle->material, 0.8, 0.6, 0.175);
   SET_SPEC (candle->material, 0.45, 0.45, 0.45);

   /* define lathe/spiral parameters */
   candle -> startRadius = 1.5;
   candle -> drdTheta = 0.0;
   candle -> startZ = 0.0;
   candle -> dzdTheta = 0.0;
   candle -> startTheta = 0.0;
   candle -> sweepTheta = 360.0;

   /* initialize contour up vector */
   candle->contour->up[0] = 1.0;
   candle->contour->up[1] = 0.0;
   candle->contour->up[2] = 0.0;

   /* define candlestick contour */
   PT (-8.0, 0.0); 
   PT (-10.0, 0.0); FACET; 
   PT (-10.0, 2.0); FACET;
   PT (-9.6, 2.0); FACET;
   PT (-8.0, 0.0); FACET;
   PT (-5.8, 0.0); FACET;
   PT (-5.2, 0.6); FACET;
   PT (-4.6, 0.0); FACET;
   PT (-1.5, 0.0); FACET;

   dtheta = M_PI /14.0;
   theta = 0.0;
   for (j=0; j<14; j++) {
      PT ((-1.5*cos(theta)) , (1.5*sin(theta)));
      NORM ((-cos(theta)) , sin(theta));
      theta += dtheta;
   }
   PT (1.5, 0.0); FACET; 
   PT (4.6, 0.0); FACET;
   PT (5.2, 0.6); FACET;
   PT (5.8, 0.0); FACET;
   PT (7.0, 0.0); FACET;
   PT (7.5, 0.2); FACET;
   PT (8.0, 0.8); FACET;
   PT (8.3, 0.9); FACET;
   PT (8.15, 1.8); FACET;
   PT (8.8, 2.8); FACET;
   PT (9.2, 3.8); FACET;
   PT (9.5, 3.8); FACET;
   PT (9.56, 3.75); FACET;
   PT (9.62, 3.75); FACET;
   PT (9.7, 3.8); FACET; 
   PT (10.0, 3.8); FACET;
   PT (10.0, 0.0); FACET;
   PT (7.0, 0.0); FACET;

   /* initialize the transofrms */
   candle->startXform[0][0] = 1.0;
   candle->startXform[0][1] = 0.0;
   candle->startXform[0][2] = 0.0;
   candle->startXform[1][0] = 0.0;
   candle->startXform[1][1] = 1.0;
   candle->startXform[1][2] = 0.0;

   candle->dXdTheta[0][0] = 0.0;
   candle->dXdTheta[0][1] = 0.0;
   candle->dXdTheta[0][2] = 0.0;
   candle->dXdTheta[1][0] = 0.0;
   candle->dXdTheta[1][1] = 0.0;
   candle->dXdTheta[1][2] = 0.0;

   /* set the initial join style */
   style = gleGetJoinStyle ();
   style &= ~TUBE_NORM_MASK;
   style |= TUBE_NORM_PATH_EDGE;
   style |= TUBE_NORM_FACET;
   gleSetJoinStyle (style);

}

/* =========================================================== */

/* get notified of mouse motions */
void MouseMotion (int x, int y)
{
   lastx = x;
   lasty = y;
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

      case 20:
         style &= ~TUBE_NORM_MASK;
         style |= TUBE_NORM_FACET;
         break;
      case 21:
         style &= ~TUBE_NORM_MASK;
         style |= TUBE_NORM_EDGE;
         break;
      case 22:
         style &= ~TUBE_NORM_MASK;
         style |= TUBE_NORM_PATH_EDGE;
         style |= TUBE_NORM_FACET;
         break;
      case 23:
         style &= ~TUBE_NORM_MASK;
         style |= TUBE_NORM_PATH_EDGE;
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
GLfloat lightOneColor[] = {0.54, 0.54, 0.54, 1.0}; 

GLfloat lightTwoPosition[] = {-40.0, 40, 100.0, 0.0};
GLfloat lightTwoColor[] = {0.54, 0.54, 0.54, 1.0}; 

int
main (int argc, char * argv[]) {

   /* initialize glut */
   glutInit (&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutCreateWindow ("candlestick");
   glutDisplayFunc (draw_candle);
   glutMotionFunc (MouseMotion);

   /* create popup menu */
   glutCreateMenu (JoinStyle);
   glutAddMenuEntry ("Facet Normal Vectors", 20);
   glutAddMenuEntry ("Edge Normal Vectors", 21);
   glutAddMenuEntry ("Facet Sweep Normal Vectors", 22);
   glutAddMenuEntry ("Edge Sweep Normal Vectors", 23);
   glutAddMenuEntry ("------------------", 9999);
   glutAddMenuEntry ("Exit", 99);
   glutAttachMenu (GLUT_MIDDLE_BUTTON);

   /* initialize GL */
   glClearDepth (1.0);
   glEnable (GL_DEPTH_TEST);
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glShadeModel (GL_SMOOTH);

   glMatrixMode (GL_PROJECTION);
   /* roughly, measured in centimeters */
   glFrustum (-9.0, 9.0, -9.0, 9.0, 50.0, 150.0);
   glMatrixMode(GL_MODELVIEW);

   /* initialize lighting */
   glLightfv (GL_LIGHT0, GL_POSITION, lightOnePosition);
   glLightfv (GL_LIGHT0, GL_AMBIENT, lightOneColor);
   glLightfv (GL_LIGHT0, GL_DIFFUSE, lightOneColor);
   glLightfv (GL_LIGHT0, GL_SPECULAR, lightOneColor);
   glEnable (GL_LIGHT0);
   glLightfv (GL_LIGHT1, GL_POSITION, lightTwoPosition);
   glLightfv (GL_LIGHT1, GL_DIFFUSE, lightTwoColor);
   glLightfv (GL_LIGHT1, GL_AMBIENT, lightTwoColor);
   glEnable (GL_LIGHT1);
   glEnable (GL_LIGHTING);
   glEnable (GL_NORMALIZE);
   /* glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE); */
   /* glEnable (GL_COLOR_MATERIAL); */

   init_candle ();

   glutMainLoop ();
   return 0;             /* ANSI C requires main to return int. */
}

/* ===================== END OF FILE ================== */

