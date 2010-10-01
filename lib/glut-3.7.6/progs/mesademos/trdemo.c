
/* trdemo.c */

/*
 * Test/demonstration of tile rendering utility library.
 * See tr.h for more info.
 *
 * Brian Paul
 * April 1997
 */


#include <stdio.h>
#include <stdlib.h>
#include "GL/glut.h"
#include "tr.h"


#define TILESIZE 100

#define NUMBALLS 30
static GLfloat BallPos[NUMBALLS][3];
static GLfloat BallSize[NUMBALLS];
static GLfloat BallColor[NUMBALLS][4];

static GLboolean Perspective = GL_TRUE;

static int WindowWidth, WindowHeight;


/* Return random float in [0,1] */
static float Random(void)
{
   int i = rand();
   return (float) (i % 1000) / 1000.0;
}


/* Draw my stuff */
static void DrawScene(void)
{
   int i;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   for (i=0;i<NUMBALLS;i++) {
      int t;
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BallColor[i]);
      glPushMatrix();
      glTranslatef(BallPos[i][0], BallPos[i][1], BallPos[i][2]);
      t = 12 + (int) (BallSize[i] * 12);
      glutSolidSphere(BallSize[i], t, t);
      glPopMatrix();
   }
}


/* Do a demonstration of tiled rendering */
static void Display(void)
{
   GLubyte *image;
   int tile = 0;
   TRcontext *tr;
   int i;

   /* Generate random balls */
   for (i=0;i<NUMBALLS;i++) {
      BallPos[i][0] = -2.0 + 4.0 * Random();
      BallPos[i][1] = -2.0 + 4.0 * Random();
      BallPos[i][2] = -2.0 + 4.0 * Random();
      BallSize[i] = Random();
      BallColor[i][0] = Random();
      BallColor[i][1] = Random();
      BallColor[i][2] = Random();
      BallColor[i][3] = 1.0;
   }

   /* allocate final image buffer */
   image = malloc(WindowWidth * WindowHeight * 4 * sizeof(GLubyte));
   if (!image) {
      printf("Malloc failed!\n");
      return;
   }

   /* Setup tiled rendering.  Each tile is TILESIZE x TILESIZE pixels. */
   tr = trNew();
   trSetup(tr, WindowWidth, WindowHeight, (GLubyte *) image,
	   TILESIZE, TILESIZE);
   if (Perspective)
      trFrustum(tr, -1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   else
      trOrtho(tr, -3.0, 3.0, -3.0, 3.0, -3.0, 3.0);

   /* Draw tiles */
   do {
      trBeginTile(tr);
      DrawScene();
      tile++;
   } while (trEndTile(tr));
   printf("%d tiles drawn\n", tile);

   trDelete(tr);

   /* show final image, might otherwise write it to a file */
   glDrawPixels(WindowWidth, WindowHeight, GL_RGBA, GL_UNSIGNED_BYTE, image);
   glFlush();

   free(image);
}



static void Reshape( int width, int height )
{
   WindowWidth = width;
   WindowHeight = height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   if (Perspective)
      glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   else
      glOrtho( -3.0, 3.0, -3.0, 3.0, -3.0, 3.0);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   if (Perspective)
      glTranslatef( 0.0, 0.0, -15.0 );
}


/* ARGSUSED1 */
static void Key( unsigned char key, int x, int y )
{
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   static GLfloat pos[4] = {0.0, 0.0, 10.0, 0.0};
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_NORMALIZE);
   glEnable(GL_DEPTH_TEST);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition(0, 0);
   glutInitWindowSize( 500, 500 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH );
   glutCreateWindow( "tile rendering" );
   Init();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutMainLoop();
   return 0;             /* ANSI C requires main to return int. */
}

