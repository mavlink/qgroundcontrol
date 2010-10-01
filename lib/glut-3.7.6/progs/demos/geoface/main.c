/* aux2glut conversion Copyright (c) Mark J. Kilgard, 1997 */

/* ==========================================================================
                               MAIN_C
=============================================================================

    FUNCTION NAMES

    movelight 		-- moves the light source.
    rotatexface 	-- rotates the face about the X axis.
    rotateyface 	-- rotates the face about the Y axis.
    myinit 		-- local initialization.
    faceinit 		-- glutInit(&argc, argv); initialize the face data.
    display 		-- display functions.
    myReshape 		-- window respahe callback.
    error_exit		-- error function.
    usage		-- usage function.
    GLenum Key 		-- keyboard mappings.
    main 		-- main program.


    C SPECIFICATIONS

    void movelight 	( int x, int y )
    void rotatexface 	( int x, int y )
    void rotateyface 	( int x, int y )
    void myinit 	( void )
    faceinit 		( void )
    void display 	( void )
    void myReshape 	( GLsizei w, GLsizei h )
    void error_exit	( char *error_message )
    void usage		( char *name )
    static GLenum Key 	( int key, GLenum mask )
    void main 		( int argc, char** argv )

    DESCRIPTION
	
	This module is where everything starts.	This module comes as is 
	with no warranties.  

    SIDE EFFECTS
	Unknown.
   
    HISTORY
	Created 16-Dec-94  Keith Waters at DEC's Cambridge Research Lab.
	Modified 22-Nov-96 Sing Bing Kang (sbk@crl.dec.com)
	  Added function print_mesg to print out all the keyboard commands
	  Added the following functionalities:
	    rereading the expression file
	    changing the expression (based on the expression file)
	    quitting the program with 'q' or 'Q' in addition to 'Esc'

============================================================================ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>

#include "memory.h"               /* Local memory allocation macros          */
/*#include "window.h"                Local window header                     */
#include "head.h"                 /* Local head data structure               */

int verbose = 0;

void print_mesg(void);

int DRAW_MODE = 2 ;

HEAD *face ;

static int spinxlight = 0 ;
static int spinylight = 0 ;
static int spinxface = 0 ;
static int spinyface = 0 ;


/* ========================================================================= */
/* motion	                                                             */
/* ========================================================================= */  
/*
** Rotate the face and light about.
*/

int rotate = 0, movelight = 0, origx, origy;

void motion ( int x, int y )
{
  if (rotate) {
    spinyface = ( spinyface + (x - origx) ) % 360 ;
    spinxface = ( spinxface + (y - origy) ) % 360 ;
    origx = x;
    origy = y;
    glutPostRedisplay();
  }
  if (movelight) {
    spinylight = ( spinylight + (x - origx ) ) % 360 ;
    spinxlight = ( spinxlight + (y - origy ) ) % 360 ;
    origx = x;
    origy = y;
    glutPostRedisplay();
  }
}

void
mouse(int button, int state, int x, int y)
{
  switch(button) {
  case GLUT_LEFT_BUTTON:
    if (state == GLUT_DOWN) {
      origx = x;
      origy = y;
      rotate = 1;
    } else {
      rotate = 0;
    }
    break;
  case GLUT_MIDDLE_BUTTON:
    if (state == GLUT_DOWN) {
      origx = x;
      origy = y;
      movelight = 1;
    } else {
      movelight = 0;
    }
    break;
  }
}


/* ========================================================================= */
/* myinit		                                                     */
/* ========================================================================= */  
/*
** Do the lighting thing.
*/

void myinit ( void )
{
  glEnable    ( GL_LIGHTING   ) ;
  glEnable    ( GL_LIGHT0     ) ;
  glDepthFunc ( GL_LEQUAL     ) ;
  glEnable    ( GL_DEPTH_TEST ) ;
}


/* ========================================================================= */
/* faceinit		                                                     */
/* ========================================================================= */  
/*
** Read in the datafiles and glutInit(&argc, argv); initialize the face data structures.
*/

void
faceinit ( void )
{
  face = create_face         ( "index.dat", "faceline.dat") ;
  read_muscles               ("muscle.dat", face ) ;
  read_expression_vectors    ("expression-vectors.dat", face ) ;
  data_struct                ( face ) ;	
}

void
read_expressions(void)
{
  read_expression_vectors    ("expression-vectors.dat", face ) ;
}

/* ========================================================================= */
/* display                                                                   */
/* ========================================================================= */  
/*
** Here's were all the display action takes place.
*/

void display ( void )
{
  GLfloat position [] = { 30.0, 70.0, 100.0, 1.0 }  ;
    
  glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

  glPushMatrix ( ) ; 
  
    glTranslatef ( 0.0, 0.0, -30.0 ) ;

    glRotated	( (GLdouble) spinxface, 1.0, 0.0, 0.0 ) ;
    glRotated	( (GLdouble) spinyface, 0.0, 1.0, 0.0 ) ;
  
    glPushMatrix ( ) ; 
      glRotated		( (GLdouble) spinxlight, 1.0, 0.0, 0.0 ) ;
      glRotated		( (GLdouble) spinylight, 0.0, 1.0, 0.0 ) ;
      glLightfv		( GL_LIGHT0, GL_POSITION, position ) ;
  
      glTranslated 	( 0.0, 0.0, 50.0 ) ;
      glDisable		( GL_LIGHTING ) ;
      glColor3f		( 0.0, 1.0, 1.0 ) ;
      glutWireCube 	( 0.1 ) ;
      glEnable		( GL_LIGHTING ) ;
   glPopMatrix ( ) ;

  calculate_polygon_vertex_normal 	( face ) ;

  paint_polygons 			( face, DRAW_MODE, 0 ) ;	

  if ( DRAW_MODE == 0 )
    paint_muscles			( face ) ;
  
  glPopMatrix();

  glutSwapBuffers();
}


/* ========================================================================= */
/* myReshape		                                                     */
/* ========================================================================= */  
/*
** What to do of the window is modified.
*/

void myReshape ( GLsizei w, GLsizei h )
{
  glViewport	( 0,0,w,h ) ;
  glMatrixMode  ( GL_PROJECTION ) ;
  glLoadIdentity( ) ;
  gluPerspective( 40.0, (GLfloat) w/(GLfloat) h, 1.0, 100.0 ) ;
  glMatrixMode  ( GL_MODELVIEW ) ;
}


/* ========================================================================= */
/* error_exit	                                                             */
/* ========================================================================= */  
/*
** Problems!
*/

void error_exit( char *error_message )
{
    fprintf ( stderr, "%s\n", error_message ) ;
    exit( 1 ) ;
}


/* ========================================================================= */
/* usage		                                                     */
/* ========================================================================= */  
/*
** At startup provide usage modes.
*/

void usage( char *name )
{
    fprintf( stderr, "\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
        "usage: ", name, " [options]\n\n",
        "  Options:\n",
        "    -display  displayname  specify an X server connection\n",
        "    -geometry geometry     specify window geometry in pixels\n",
        "    -rgba                  ask for rgba visual\n",
        "    -index                 ask for color index visual\n",
        "    -doublebuffer          ask for double buffered visual\n",
        "    -singlebuffer          ask for single buffered visual\n",
        "    -accum                 ask for accumulation buffer\n",
        "    -alpha                 ask for alpha buffer\n",
        "    -depth                 ask for depth buffer\n",
        "    -stencil               ask for stencil buffer\n",
        "    -aux nauxbuf           specify number of aux buffers\n",
        "    -level planes          specify planes (0=main,>0=overlay,<0=underlay\n",
        "    -transparent           ask for transparent overlay\n",
        "    -opaque                ask for opaque overlay\n"
    );
    
    exit( 1);
}

/* ========================================================================= */
/* Key			                                                     */
/* ========================================================================= */
/*
** Actions on a key press.
*/

static int m = 0, e = 0;

/* ARGSUSED1 */
static void Key ( unsigned char key, int x, int y )
{
  char title[512];
  
    switch ( key ) {
      case 27 :
      case 'q' :
      case 'Q' :
	exit (0) ;

      case 'r' :
      case 'R' :
	printf ("Rereading expression file\n");
        read_expressions();
	e = 0; /* reset the expression count variable */
	glutPostRedisplay();
	break;

      case 'a' :
	printf ("increment muscle: %s\n", face->muscle[m]->name ) ;

	/* set the muscle activation */
	face->muscle[m]->mstat += 0.1 ;
	
	activate_muscle ( face, 
			 face->muscle[m]->head, 
			 face->muscle[m]->tail, 
			 face->muscle[m]->fs,
			 face->muscle[m]->fe,
			 face->muscle[m]->zone,
			 0.1 ) ;
	glutPostRedisplay();
	break;

      case 'A' :
	printf ("decrement muscle: %s\n", face->muscle[m]->name ) ;
	face->muscle[m]->mstat -= 0.1 ;

	activate_muscle ( face, 
			 face->muscle[m]->head, 
			 face->muscle[m]->tail, 
			 face->muscle[m]->fs,
			 face->muscle[m]->fe,
			 face->muscle[m]->zone,
			 -0.1 ) ;
	glutPostRedisplay();
	break;

      case 'b' :
	DRAW_MODE++ ;

	if ( DRAW_MODE >= 3 ) DRAW_MODE = 0 ;
	printf ("draw mode: %d\n", DRAW_MODE ) ;
	glutPostRedisplay();
	break;

      case 'c' :
	face_reset ( face ) ;
	glutPostRedisplay();
	break;
	
      case 'n' :
	m++ ;
	if ( m >= face->nmuscles ) m = 0 ;
        sprintf(title, "geoface (%s)", face->muscle[m]->name);
        glutSetWindowTitle(title);
	break;

      case 'e' :
	if (face->expression) {
	face_reset  ( face ) ;
	expressions ( face, e ) ;

	e++ ;
	if ( e >= face->nexpressions ) e = 0 ;
	glutPostRedisplay();
	}
	break;

      case 'h' :
	
	print_mesg();
	
    }
}

/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  char title[512];

  switch(key) {
  case GLUT_KEY_RIGHT:
    m++ ;
    if ( m >= face->nmuscles ) m = 0 ;
    sprintf(title, "geoface (%s)", face->muscle[m]->name);
    glutSetWindowTitle(title);
    break;
  case GLUT_KEY_LEFT:
    m-- ;
    if ( m < 0 ) m = face->nmuscles - 1 ;
    sprintf(title, "geoface (%s)", face->muscle[m]->name);
    glutSetWindowTitle(title);
    break;
  case GLUT_KEY_UP:
	face->muscle[m]->mstat += 0.1 ;

	activate_muscle ( face, 
			 face->muscle[m]->head, 
			 face->muscle[m]->tail, 
			 face->muscle[m]->fs,
			 face->muscle[m]->fe,
			 face->muscle[m]->zone,
			 0.1 ) ;
	glutPostRedisplay();
    break;
  case GLUT_KEY_DOWN:
	face->muscle[m]->mstat -= 0.1 ;

	activate_muscle ( face, 
			 face->muscle[m]->head, 
			 face->muscle[m]->tail, 
			 face->muscle[m]->fs,
			 face->muscle[m]->fe,
			 face->muscle[m]->zone,
			 -0.1 ) ;
	glutPostRedisplay();
    break;
  }
}


/* ========================================================================= *
 * print_mesg
 * Written by: Sing Bing Kang (sbk@crl.dec.com)
 * Date: 11/22/96
 * ========================================================================= */
/*
** Prints out help message
*/
void
print_mesg(void)
{
fprintf(stderr,"\n");
fprintf(stderr,"a:       draw mode (to `pull' the current facial muscle)\n");
fprintf(stderr,"A:       draw mode (to `contract' current facial muscle)\n");
fprintf(stderr,"c:       face reset\n");
fprintf(stderr,"n:       next muscle (to select another facial muscle to manipulate)\n");
fprintf(stderr,"e:       next expression\n");
fprintf(stderr,"b:       to change draw mode: wireframe->polygonal patches->smooth surface\n");
fprintf(stderr,"r,R:     reread the expression file (../face-data/expression-vectors.dat)\n         (Note: this resets the expression sequence to the beginning)\n");
fprintf(stderr,"q,Q,Esc: quit\n");
fprintf(stderr,"h:       outputs this message\n");
fprintf(stderr,"\n");
}

void
muscle_select(int value)
{
  char title[512];

  /* Select muscle. */
  m = value;
  sprintf(title, "geoface (%s)", face->muscle[m]->name);
  glutSetWindowTitle(title);
}

void
main_menu_select(int value)
{
  char title[512];

  switch(value) {
  case 1:
    face_reset ( face ) ;
    glutPostRedisplay();
    break;
  case 2:
    print_mesg();
    break;
  case 3:
    face->muscle[m]->mstat += 0.25 ;
    activate_muscle ( face, 
      face->muscle[m]->head, 
      face->muscle[m]->tail, 
      face->muscle[m]->fs,
      face->muscle[m]->fe,
      face->muscle[m]->zone,
      +0.25 ) ;
    glutPostRedisplay();
    break;
  case 4:
    face->muscle[m]->mstat -= 0.25 ;
    activate_muscle ( face, 
      face->muscle[m]->head, 
      face->muscle[m]->tail, 
      face->muscle[m]->fs,
      face->muscle[m]->fe,
      face->muscle[m]->zone,
      -0.25 ) ;
    glutPostRedisplay();
    break;
  case 5:
    m++ ;
    if ( m >= face->nmuscles ) m = 0 ;
    sprintf(title, "geoface (%s)", face->muscle[m]->name);
    glutSetWindowTitle(title);
    break;
  case 666:
    exit(0);
    break;
  }
}

void
draw_mode_select(int value)
{
  DRAW_MODE = value;
  glutPostRedisplay();
}

void
make_menus(void)
{
  int i, j, muscle_menu, draw_mode_menu;
  char *entry;

  muscle_menu = glutCreateMenu(muscle_select);
  for (i=0; i<face->nmuscles; i++) {
    entry = face->muscle[i]->name;
    for(j=(int) strlen(entry)-1; j>=0; j--) {
      if (entry[j] == '_') entry[j] = ' ';
    }
    glutAddMenuEntry(entry, i);
  }
  draw_mode_menu = glutCreateMenu(draw_mode_select);
  glutAddMenuEntry("Wireframe", 0);
  glutAddMenuEntry("Polygonal patches", 1);
  glutAddMenuEntry("Smooth surface", 2);
  glutCreateMenu(main_menu_select);
  glutAddMenuEntry("Pull muscle up", 3);
  glutAddMenuEntry("Pull muscle down", 4);
  glutAddMenuEntry("Next muscle", 5);
  glutAddSubMenu("Select muscle", muscle_menu);
  glutAddSubMenu("Draw mode", draw_mode_menu);
  glutAddMenuEntry("Face reset", 1);
  glutAddMenuEntry("Print help", 2);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/* ========================================================================= */
/* main			                                                    */
/* ========================================================================= */
/*
** All the initialization and action takes place here.
*/

int main ( int argc, char** argv )
{
  int i;

  glutInitWindowSize	( 400, 600 ) ;
  glutInit(&argc, argv);
  for(i=1; i<argc; i++) {
    if(!strcmp(argv[i], "-v")) {
      verbose = 1;
    }
  }
  glutInitDisplayMode	(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
  glutCreateWindow		( "geoface" ) ;
  myinit		( ) ;
  faceinit		( ) ;
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutKeyboardFunc		( Key ) ;	
  glutSpecialFunc(special);
  glutReshapeFunc 	( myReshape ) ;
  glutDisplayFunc(display);
  make_menus();
  glutMainLoop() ;
  return 0;             /* ANSI C requires main to return int. */
}

  

