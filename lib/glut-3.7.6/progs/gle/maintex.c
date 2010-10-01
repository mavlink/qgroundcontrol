
/* 
 * texture map demo scaffolding
 *
 * FUNCTION:
 * Most of this code is required to set up OpenGL and GLUT
 *
 * HISTORY:
 * Written by Linas Vepstas, March 1995
 */

/* required include files */
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/tube.h>
#include "texture.h"

/*  most recent mouse postion */
float lastx = 100.0;
float lasty = 100.0;

extern void InitStuff (void);
extern void DrawStuff (void);

/* get notified of mouse motions */
void MouseMotion (int x, int y)
{
   lastx = x;
   lasty = y;
   glutPostRedisplay ();
}

void TextureStyle (int msg) 
{
   int mode = 0;

   switch (msg) {
      case 301:
         glDisable (GL_TEXTURE_2D);
         break;
      case 302:
         glEnable (GL_TEXTURE_2D);
         break;

      case 501:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_VERTEX_FLAT;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (0.1, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 502:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_NORMAL_FLAT;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (0.1, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 503:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_VERTEX_CYL;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (1.0, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 504:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_NORMAL_CYL;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (1.0, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 505:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_VERTEX_SPH;
         glMatrixMode (GL_TEXTURE); glLoadIdentity (); 
         glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 506:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_NORMAL_SPH;
         glMatrixMode (GL_TEXTURE); glLoadIdentity (); 
         glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;

      case 507:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_VERTEX_MODEL_FLAT;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (0.1, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 508:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_NORMAL_MODEL_FLAT;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (0.1, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 509:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_VERTEX_MODEL_CYL;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (1.0, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 510:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_NORMAL_MODEL_CYL;
         glMatrixMode (GL_TEXTURE); glLoadIdentity ();
         glScalef (1.0, 0.1, 1.0); glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 511:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_VERTEX_MODEL_SPH;
         glMatrixMode (GL_TEXTURE); glLoadIdentity (); 
         glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;
      case 512:
         mode = GLE_TEXTURE_ENABLE | GLE_TEXTURE_NORMAL_MODEL_SPH;
         glMatrixMode (GL_TEXTURE); glLoadIdentity (); 
         glMatrixMode (GL_MODELVIEW);
         gleTextureMode (mode);
         break;

      case 701:
         current_texture = check_texture;
         gluBuild2DMipmaps (GL_TEXTURE_2D, 3, 
                      current_texture -> size,
                      current_texture -> size,
                      GL_RGB, GL_UNSIGNED_BYTE, 
                      (void *) (current_texture->pixmap));
         break;
      case 702:
         current_texture = barberpole_texture;
         gluBuild2DMipmaps (GL_TEXTURE_2D, 3, 
                      current_texture -> size,
                      current_texture -> size,
                      GL_RGB, GL_UNSIGNED_BYTE, 
                      (void *) (current_texture->pixmap));
         break;
      case 703:
         current_texture = wild_tooth_texture;
         gluBuild2DMipmaps (GL_TEXTURE_2D, 3, 
                      current_texture -> size,
                      current_texture -> size,
                      GL_RGB, GL_UNSIGNED_BYTE, 
                      (void *) (current_texture->pixmap));
         break;
      case 704:
         current_texture = planet_texture;
         gluBuild2DMipmaps (GL_TEXTURE_2D, 3, 
                      current_texture -> size,
                      current_texture -> size,
                      GL_RGB, GL_UNSIGNED_BYTE, 
                      (void *) (current_texture->pixmap));
         break;

      case 99:
         exit (0);
      default:
         break;
   }
   glutPostRedisplay();
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
   glutCreateWindow ("texture");
   glutDisplayFunc (DrawStuff);
   glutMotionFunc (MouseMotion);

   /* create popup menu */
   glutCreateMenu (TextureStyle);
   glutAddMenuEntry ("Texture Off", 301);
   glutAddMenuEntry ("Texture On", 302);
   glutAddMenuEntry ("--------------", 9999);
   glutAddMenuEntry ("Vertex Flat", 501);
   glutAddMenuEntry ("Normal Flat", 502);
   glutAddMenuEntry ("Vertex Cylinder", 503);
   glutAddMenuEntry ("Normal Cylinder", 504);
   glutAddMenuEntry ("Vertex Sphere", 505);
   glutAddMenuEntry ("Normal Sphere", 506);
   glutAddMenuEntry ("--------------", 9999);
   glutAddMenuEntry ("Model Vertex Flat", 507);
   glutAddMenuEntry ("Model Normal Flat", 508);
   glutAddMenuEntry ("Model Vertex Cylinder", 509);
   glutAddMenuEntry ("Model Normal Cylinder", 510);
   glutAddMenuEntry ("Model Vertex Sphere", 511);
   glutAddMenuEntry ("Model Normal Sphere", 512);
   glutAddMenuEntry ("--------------", 9999);
   glutAddMenuEntry ("Check Texture", 701);
   glutAddMenuEntry ("Barberpole Texture", 702);
   glutAddMenuEntry ("Wild Tooth Texture", 703);
   glutAddMenuEntry ("Molten Lava Texture", 704);
   glutAddMenuEntry ("--------------", 9999);
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
   glColor3f (0.8, 0.3, 0.6);

   /* initialize and enable texturing */
   setup_textures ();
   gluBuild2DMipmaps (GL_TEXTURE_2D, 3, 
                      current_texture -> size,
                      current_texture -> size,
                      GL_RGB, GL_UNSIGNED_BYTE, 
                      (void *) (current_texture->pixmap));

   glMatrixMode (GL_TEXTURE);
   glLoadIdentity ();
   glScalef (1.0, 0.1, 1.0);
   glMatrixMode (GL_MODELVIEW);

   glEnable (GL_TEXTURE_2D);
/*
   some stuff to play with ....
   glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
   glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
   glEnable (GL_TEXTURE_GEN_S);
   glEnable (GL_TEXTURE_GEN_T);
*/

   gleTextureMode (GLE_TEXTURE_ENABLE | GLE_TEXTURE_VERTEX_CYL);

   InitStuff ();

   glutMainLoop ();
   return 0;             /* ANSI C requires main to return int. */
}

/* -------------------- end of file -------------------- */
