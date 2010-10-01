#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include "texture.h"

#ifndef __sgi
#define trunc(x) ((double)((int)(x)))
#endif

GLsizei sphereTexW, sphereTexH, padSphereTexW, padSphereTexH;
GLint sphereTexComp;


const char defaultSphereMap[] = "../data/spheremap.rgb";
int drawTorus = 1;


int roundup(int n)
{
  int val = 1;
  while (val < n) val <<= 1;
  return val;
}

void create_texture(const char *fname, GLsizei *w,  GLsizei *h, 
		      GLsizei *padW, GLsizei *padH, GLint *comps)
{
  GLuint *img, *padImg = NULL;
  int y;

  img = read_texture(fname, w, h, comps);
  if (!img) {
    fprintf(stderr, "Could not open %s\n", fname);
    exit(1);
  }

  /* if width & height are not powers of two, pad image with black */
  if (*w & (*w - 1)) {
    *padW = roundup(*w);
  } else {
    *padW = *w;
  }
  if (*h & (*h - 1)) {
    *padH = roundup(*h);
  } else {
    *padH = *h;
  }

  if (*padW != *w || *padH != *h) {
printf("rounding %s up...\n", fname);
    padImg = (GLuint *)malloc(*padW * *padH * sizeof(GLuint));
    if (!padImg) {
      fprintf(stderr, "Malloc of %d bytes failed.\n", 
	      *padW * *padH * sizeof(GLuint));
      exit(1);
    }
    memset(padImg, 0, *padW * *padH * sizeof(GLuint));
    for (y = 0; y < *h; y++) {
      memcpy(&padImg[y * *padW], &img[y * *w], *w * sizeof(GLuint));
    }
  }

  /* you should use texture objects here if your system supports them... */
printf("w = %d h = %d\n", *padW, *padH);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, *padW, *padH, 0, 
	       GL_RGBA, GL_UNSIGNED_BYTE, img);

  free(img);
  if (padImg) free(padImg);
}

void init(const char *sphereFile)
{
  static GLfloat lightpos[] = {.5, .75, 1.5, 1};

  glEnable(GL_DEPTH_TEST); 
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    create_texture(sphereFile, &sphereTexW, &sphereTexH, 
		   &padSphereTexW, &padSphereTexH, &sphereTexComp);
}

void reshape(GLsizei w, GLsizei h) 
{
  glViewport(0, 0, w, h);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, 1, .01, 10);
  gluLookAt(0, 0, 0, 
	    0, 0, -1, 
	    0, 1, 0);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void draw_room(void)
{
  /* material for the walls, floor, ceiling */
  static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

  glBegin(GL_QUADS);
  
  /* floor */
  glNormal3f(0, 1, 0);
  glVertex3f(-1, -1, 1);
  glVertex3f(1, -1, 1);
  glVertex3f(1, -1, -1);
  glVertex3f(-1, -1, -1);

  /* ceiling */
  glNormal3f(0, -1, 0);
  glVertex3f(-1, 1, -1);
  glVertex3f(1, 1, -1);
  glVertex3f(1, 1, 1);
  glVertex3f(-1, 1, 1);  

  /* left wall */
  glNormal3f(1, 0, 0);
  glVertex3f(-1, -1, -1);
  glVertex3f(-1, 1, -1); 
  glVertex3f(-1, 1, 1);
  glVertex3f(-1, -1, 1);

  /* right wall */
  glNormal3f(-1, 0, 0);
  glVertex3f(1, -1, 1);
  glVertex3f(1, 1, 1);
  glVertex3f(1, 1, -1);
  glVertex3f(1, -1, -1);

  /* far wall */
  glNormal3f(0, 0, 1);
  glVertex3f(-1, -1, -1);
  glVertex3f(1, -1, -1);
  glVertex3f(1, 1, -1);
  glVertex3f(-1, 1, -1);

  glEnd();

}

void draw_torus(GLdouble angle)
{
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		  GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
		  GL_NEAREST);
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_2D);  
  glEnable(GL_CULL_FACE); 
  
  glPushMatrix();
  glTranslatef(0, 0, -3);
  glRotatef(angle, 1, 1, 0);
  
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  if (drawTorus) {
    glutSolidTorus(.4, .75, 32, 32);
  } else {
    GLUquadricObj *sphere = gluNewQuadric();
    gluSphere(sphere, 1, 32, 32);
  }

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glEnable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);

  glPopMatrix();
}

GLdouble get_secs(void)
{
  return glutGet(GLUT_ELAPSED_TIME) / 1000.0;
}

void draw(void)
{
    GLenum err;
    GLdouble secs, degrees;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* one revolution every 10 seconds... */
    secs = get_secs();
    secs = secs - 10.*trunc(secs / 10.);
    degrees = (secs/10.) * (360.);

#if 0
    draw_room();
#endif
    draw_torus(degrees);

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  static int idle = 1;
  if (key == 27) exit(0);
  if (key == 'o' || key == 'O') {
    drawTorus = (drawTorus == 0);
    draw();
  } else {
    if (idle) {
      glutIdleFunc(0);
    } else {
      glutIdleFunc(draw);
    }
    idle = (idle == 0);
  }
}

main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(256, 256);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutIdleFunc(draw); 
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    init(defaultSphereMap);

    glutMainLoop();
    return 0;
}

