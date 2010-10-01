/* 
 * transport.c
 * 
 * FUNCTION:
 * Illustrate principle of shearing vs. parallel transport.
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
Extrusion *arrow = NULL;

/* =========================================================== */
float lastx=0;
float lasty=0;

void draw_arrow (void) {

   /* attach the mouse */
   arrow->sweepTheta = 180.0 + 0.13* lastx;
   arrow->dzdTheta = 0.03 * (lasty+10.0);

   glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, arrow->material->ambient);
   glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, arrow->material->diffuse);
   glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, arrow->material->specular);
   glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 2.0);

#ifdef IBM_GL_32
   rotate (-750, 'x');
   rotate (-100, 'z');

   /* first, draw with the routine that uses a local coordinate 
    * system with torsion */
   translate (-0.5, -0.5, 3.2);

   lathe (arrow->contour->numContourPoints,
           arrow->contour->pts,
           arrow->contour->norms,
           arrow->contour->up,
           arrow->startRadius,    /* donut radius */
           arrow->drdTheta,   /* change in donut radius per revolution */
           arrow->startZ,    /* start z value */
           arrow->dzdTheta,     /* change in Z per revolution */
           NULL,
           NULL,
           arrow->startTheta,     /* start angle */
           arrow->sweepTheta);     /* sweep angle */

   draw_axes ();

   /* next, draw with a routine that uses parallel transport */
   translate (0.0, 0.0, -5.4);
   lmbind (MATERIAL, 88);
   cpack (0x339999);
   spiral (arrow->contour->numContourPoints,
           arrow->contour->pts,
           arrow->contour->norms,
           arrow->contour->up,
           arrow->startRadius,    /* donut radius */
           arrow->drdTheta,   /* change in donut radius per revolution */
           arrow->startZ,    /* start z value */
           arrow->dzdTheta,     /* change in Z per revolution */
           NULL,
           NULL,
           arrow->startTheta,     /* start angle */
           arrow->sweepTheta);     /* sweep angle */

#endif 

#define OPENGL_10
#ifdef OPENGL_10
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotated (-75.0, 1.0, 0.0, 0.0);
   glRotated (-10.0, 0.0, 0.0, 1.0);

   /* first, draw with the routine that uses a local coordinate 
    * system with torsion */
   glTranslated (-0.5, -0.5, 4.2);

   gleLathe (arrow->contour->numContourPoints,
           arrow->contour->pts,
           arrow->contour->norms,
           arrow->contour->up,
           arrow->startRadius,    /* donut radius */
           arrow->drdTheta,   /* change in donut radius per revolution */
           arrow->startZ,    /* start z value */
           arrow->dzdTheta,     /* change in Z per revolution */
           NULL,
           NULL,
           arrow->startTheta,     /* start angle */
           arrow->sweepTheta);     /* sweep angle */

   /* next, draw with a routine that uses parallel transport */
   glTranslated (0.0, 0.0, -8.4);
/*
   lmbind (MATERIAL, 88);
   cpack (0x339999);
*/
   gleSpiral (arrow->contour->numContourPoints,
           arrow->contour->pts,
           arrow->contour->norms,
           arrow->contour->up,
           arrow->startRadius,    /* donut radius */
           arrow->drdTheta,   /* change in donut radius per revolution */
           arrow->startZ,    /* start z value */
           arrow->dzdTheta,     /* change in Z per revolution */
           NULL,
           NULL,
           arrow->startTheta,     /* start angle */
           arrow->sweepTheta);     /* sweep angle */

   glPopMatrix ();
   glutSwapBuffers ();
#endif 
}

/* =========================================================== */

#define SCALE 1.80
#define PT(x,y) { ADD_POINT (arrow->contour, SCALE*x, SCALE*y); }
#define NORM(x,y) { ADD_NORMAL (arrow->contour, x, y); }
#define FACET { MAKE_NORMAL (arrow->contour); }

/* =========================================================== */

void init_arrow (void)
{
   int style;

   arrow = (Extrusion *) malloc (sizeof (Extrusion));
   NEW_EXTRUSION (arrow);

   /* define color of arrow */
   SET_AMB (arrow->material, 0.15, 0.15, 0.15);
   SET_DIFF (arrow->material, 0.15, 0.55, 0.55);
   SET_SPEC (arrow->material, 0.4, 0.4, 0.4);

   /* define lathe/spiral parameters */
   arrow -> startRadius = 7.3;
   arrow -> drdTheta = 0.0;
   arrow -> startZ = 0.0;
   arrow -> dzdTheta = 0.0;
   arrow -> startTheta = 0.0;
   arrow -> sweepTheta = 90.0;

   /* define arrow contour */
   PT (-1.0, -2.0); 
   PT (-1.0, 0.0); FACET; 
   PT (-2.0, 0.0); FACET;
   PT (0.0, 2.0); FACET;
   PT (2.0, 0.0); FACET;
   PT (1.0, 0.0); FACET;
   PT (1.0, -2.0); FACET;

   /* define contour up vector */
   arrow->contour->up[0] = 0.0;
   arrow->contour->up[1] = 0.0;
   arrow->contour->up[2] = 1.0;

   /* set the initial join style */
   style = 0x0;
   style |= TUBE_JN_CAP;
   style |= TUBE_NORM_PATH_EDGE;
   style |= TUBE_NORM_FACET;
   style |= TUBE_CONTOUR_CLOSED;
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

/* ARGSUSED */
void JoinStyle (int msg) 
{
   exit (0);
}

/* set up a light */
GLfloat lightOnePosition[] = {40.0, 40, 100.0, 0.0};
GLfloat lightOneColor[] = {0.54, 0.54, 0.54, 1.0}; 

GLfloat lightTwoPosition[] = {-40.0, 40, 100.0, 0.0};
GLfloat lightTwoColor[] = {0.54, 0.54, 0.54, 1.0}; 

GLfloat lightThreePosition[] = {40.0, 40, -100.0, 0.0};
GLfloat lightThreeColor[] = {0.54, 0.54, 0.54, 1.0}; 

int
main (int argc, char * argv[]) {

   /* initialize glut */
   glutInit (&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutCreateWindow ("transport");
   glutDisplayFunc (draw_arrow);
   glutMotionFunc (MouseMotion);

   /* create popup menu */
   glutCreateMenu (JoinStyle);
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
   glLightfv (GL_LIGHT2, GL_POSITION, lightThreePosition);
   glLightfv (GL_LIGHT2, GL_DIFFUSE, lightThreeColor);
   glLightfv (GL_LIGHT2, GL_AMBIENT, lightThreeColor);
   glEnable (GL_LIGHT2);
   glEnable (GL_LIGHTING);
   glEnable (GL_NORMALIZE);
   /* glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE); */
   /* glEnable (GL_COLOR_MATERIAL); */

   init_arrow ();

   glutMainLoop ();
   return 0;             /* ANSI C requires main to return int. */
}
/* ================== END OF FILE ================== */
