
/* 
 * FUNCTION:
 * very minimal "main()" for GL demos.
 *
 * HISTORY:
 * Linas Vepstas March 1995
 */

/* required include files */
#include <stdlib.h>
#include <GL/glut.h>

extern void DrawStuff (void);
extern void InitStuff (void);

float lastx=0;
float lasty=0;

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
GLfloat lightOneColor[] = {0.99, 0.99, 0.99, 1.0}; 

GLfloat lightTwoPosition[] = {-40.0, 40, 100.0, 0.0};
GLfloat lightTwoColor[] = {0.99, 0.99, 0.99, 1.0}; 

int
main (int argc, char * argv[]) {

   /* initialize glut */
   glutInit (&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutCreateWindow ("basic demo");
   glutDisplayFunc (DrawStuff);
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
   glLightfv (GL_LIGHT0, GL_DIFFUSE, lightOneColor);
   glEnable (GL_LIGHT0);
   glLightfv (GL_LIGHT1, GL_POSITION, lightTwoPosition);
   glLightfv (GL_LIGHT1, GL_DIFFUSE, lightTwoColor);
   glEnable (GL_LIGHT1);
   glEnable (GL_LIGHTING);
   glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable (GL_COLOR_MATERIAL);

   InitStuff ();

   glutMainLoop ();
   return 0;             /* ANSI C requires main to return int. */
}
