
/* 
 * main for exhibiting differnt join styles
 *
 * FUNCTION:
 * This demo demonstrates the various join styles,
 * and how they get applied.
 *
 * HISTORY:
 * Linas Vepstas March 1995
 */

/* required include files */
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/tube.h>

extern void InitStuff (void);
extern void DrawStuff (void);

float lastx=0;
float lasty=0;

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
GLfloat lightOneColor[] = {0.99, 0.99, 0.99, 1.0}; 

GLfloat lightTwoPosition[] = {-40.0, 40, 100.0, 0.0};
GLfloat lightTwoColor[] = {0.99, 0.99, 0.99, 1.0}; 

int
main (int argc, char * argv[]) {

   /* initialize glut */
   glutInit (&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutCreateWindow ("join styles");
   glutDisplayFunc (DrawStuff);
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
   glEnable (GL_NORMALIZE);
   glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable (GL_COLOR_MATERIAL);

   InitStuff ();

   glutMainLoop ();
   return 0;             /* ANSI C requires main to return int. */
}
/* ------------------ end of file -------------------- */
