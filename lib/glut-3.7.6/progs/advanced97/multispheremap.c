#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
#define trunc(x) ((double)((int)(x)))
#endif

GLUquadricObj *cone, *base, *qsphere;

static char defaultFile[] = "../data/mandrill.rgb";
GLuint floorList;

GLboolean animate = 1, useSphereMaps = 1;

GLsizei w = 256, h = 256;

#define LEFT 	3
#define RIGHT 	1
#define FRONT	2
#define BACK	0
#define TOP	4
#define BOTTOM 	5

GLuint *faceMap[6];
GLsizei faceW = 128;

GLuint *sphereMap[2];
GLuint sphereW = 256;

GLfloat angle1[6] = {90, 180, 270, 0, 90, -90};
GLfloat axis1[6][3] = {{0,1,0}, {0,1,0}, {0,1,0}, {0,1,0}, {1,0,0}, {1,0,0}};
GLfloat angle2[6] = {0, 0, 0, 0, 180, 180};
GLfloat axis2[6][3] = {{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,1,0}, {0,1,0}};

#define TORUS_BIT	1
#define SPHERE_BIT	2

void reset_textures(void)
{
  unsigned int i;

  /* make sphereMap[0] start out all red... */
  for (i = 0; i < sphereW*sphereW; i++) sphereMap[0][i] = 0xff0000ff;
  /* make sphereMap[1] start out all green... */
  for (i = 0; i < sphereW*sphereW; i++) sphereMap[1][i] = 0x00ff00ff;
}

void realloc_textures(void)
{
  static int first = 1;
  int i;

  if (!first) {
    for (i = 0; i < 6; i++) free(faceMap[i]);
  } else {
    first = 0;
  }

  for (i = 0; i < 6; i++) {
    faceMap[i] = (GLuint *)malloc(faceW*faceW*sizeof(GLuint));
    if (!faceMap[i]) {
      fprintf(stderr, "malloc of %d bytes failed.\n", 
	      faceW*faceW*sizeof(GLuint));
    }
  }

  sphereMap[0] = (GLuint *)malloc(sphereW * sphereW * sizeof(GLuint));
  sphereMap[1] = (GLuint *)malloc(sphereW * sphereW * sizeof(GLuint));
  reset_textures();
}

void eliminate_alpha(GLsizei w, GLsizei h, GLuint *map)
{
  int x, y;

  /* top & bottom rows */
  for (x = 0; x < w; x++) {
    map[x] &= 0xffffff00;
    map[x + (h-1)*w] &= 0xffffff00;
  }

  for (y = 0; y < h; y++) {
    map[y*w] &= 0xffffff00;
    map[y*w + (w-1)] &= 0xffffff00;
  }
}

void init(const char *fname)
{
  GLuint *img;
  GLsizei w, h;
  int comps;

  glEnable(GL_DEPTH_TEST); 
  glEnable(GL_CULL_FACE); 

  cone = gluNewQuadric();
  base = gluNewQuadric();
  qsphere = gluNewQuadric();

  img = read_texture(fname, &w, &h, &comps);
  if (!img) {
    fprintf(stderr, "Could not open %s\n", fname);
    exit(1);
  }
  floorList = glGenLists(1);
  glNewList(floorList, GL_COMPILE);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, 
	       GL_RGBA, GL_UNSIGNED_BYTE, img);
  glEndList();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  free(img);

  glClearColor(.25, .25, .5, 1.0);

  realloc_textures();
}

void reshape(GLsizei winW, GLsizei winH) 
{
  w = winW/2;

  glViewport(0, 0, w, h);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, 1, .01, 10);
  gluLookAt(-1, 0, 2.577, 0, 0, -5, 0, 1, 0);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void draw_room(void)
{
  /* material for the walls, floor, ceiling */
  static GLfloat wallMat[] = {1.f, 1.f, 1.f, 1.f};

  glPushMatrix();
  glScalef(3, 2, 3);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wallMat);

  /* floor, textured */
  glColor3f(1, 1, 1);
  glEnable(GL_TEXTURE_2D);
  glCallList(floorList);
  glBegin(GL_QUADS);
  glNormal3f(0, 1, 0);
  glTexCoord2f(0, 0);
  glVertex3f(-1, -1, 1);
  glTexCoord2f(1, 0);
  glVertex3f(1, -1, 1);
  glTexCoord2f(1, 1);
  glVertex3f(1, -1, -1);
  glTexCoord2f(0, 1);
  glVertex3f(-1, -1, -1);
  glEnd();
  glDisable(GL_TEXTURE_2D);

  /* ceiling */
  glColor3f(wallMat[0] * 1., wallMat[1] * 1., wallMat[2] * 1.);
  glBegin(GL_QUADS);
  glNormal3f(0, -1, 0);
  glVertex3f(-1, 1, -1);
  glVertex3f(1, 1, -1);
  glVertex3f(1, 1, 1);
  glVertex3f(-1, 1, 1);  

  /* left wall */
  glColor3f(wallMat[0] * .75, wallMat[1] * .75, wallMat[2] * .75);
  glNormal3f(1, 0, 0);
  glVertex3f(-1, -1, -1);
  glVertex3f(-1, 1, -1);
  glVertex3f(-1, 1, 1);
  glVertex3f(-1, -1, 1);

  /* right wall */
  glColor3f(wallMat[0] * .25, wallMat[1] * .25, wallMat[2] * .25);
  glNormal3f(-1, 0, 0);
  glVertex3f(1, -1, 1);
  glVertex3f(1, 1, 1);
  glVertex3f(1, 1, -1);
  glVertex3f(1, -1, -1);

  /* far wall */
  glColor3f(wallMat[0] * .5, wallMat[1] * .5, wallMat[2] * .5);
  glNormal3f(0, 0, 1);
  glVertex3f(-1, -1, -1);
  glVertex3f(1, -1, -1);
  glVertex3f(1, 1, -1);
  glVertex3f(-1, 1, -1);

  /* back wall */
  glColor3f(wallMat[0] * .5, wallMat[1] * .5, wallMat[2] * .5);
  glNormal3f(0, 0, -1);
  glVertex3f(-1, 1, 1);
  glVertex3f(1, 1, 1);
  glVertex3f(1, -1, 1);
  glVertex3f(-1, -1, 1);

  glEnd();

  glPopMatrix();
}

void draw_cone(void)
{
  static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};

  glPushMatrix();
  glTranslatef(0, -1, 0);
  glRotatef(-90, 1, 0, 0);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
  
  /* base is coplanar with floor, so turn off depth testing */
  glDisable(GL_DEPTH_TEST);
  gluDisk(base, 0., .3, 20, 1); 
  glEnable(GL_DEPTH_TEST);

  gluCylinder(cone, .3, 0, 1.25, 20, 1);

  glPopMatrix();
}

void draw_cube(void)
{
  glBegin(GL_QUADS);

  glNormal3f(0, -1, 0);
  glVertex3f(-.25, -.25, -.25);
  glVertex3f(.25, -.25, -.25);
  glVertex3f(.25, -.25, .25);
  glVertex3f(-.25, -.25, .25);

  glNormal3f(0, 1, 0);
  glVertex3f(-.25, .25, .25);  
  glVertex3f(.25, .25, .25);
  glVertex3f(.25, .25, -.25);
  glVertex3f(-.25, .25, -.25);

  glNormal3f(1, 0, 0);
  glVertex3f(.25, -.25, -.25);
  glVertex3f(.25, .25, -.25);
  glVertex3f(.25, .25, .25);
  glVertex3f(.25, -.25, .25);

  glNormal3f(-1, 0, 0);
  glVertex3f(-.25, -.25, .25);
  glVertex3f(-.25, .25, .25);
  glVertex3f(-.25, .25, -.25);
  glVertex3f(-.25, -.25, -.25);

  glNormal3f(0, 0, -1);
  glVertex3f(-.25, .25, -.25);
  glVertex3f(.25, .25, -.25);
  glVertex3f(.25, -.25, -.25);
  glVertex3f(-.25, -.25, -.25);

  glNormal3f(0, 0, 1);
  glVertex3f(-.25, -.25, .25);
  glVertex3f(.25, -.25, .25);
  glVertex3f(.25, .25, .25);
  glVertex3f(-.25, .25, .25);

  glEnd();
}

void draw_sphere(GLdouble angle)
{
  static GLfloat sphere_mat[] = {.2f, .7f, .2f, 1.f};

  glTexImage2D(GL_TEXTURE_2D, 0, 4, sphereW, sphereW, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, sphereMap[1]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  
  if (useSphereMaps) glEnable(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);

  glPushMatrix();
  glRotatef(45, 0, 0, 1);
  glRotatef(angle, 0, 1, 0);
  glTranslatef(1, 0, 0);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
  glColor3fv(sphere_mat);
#if 1
  {
    GLUquadricObj *sphere = gluNewQuadric();
    gluSphere(sphere, .6, 64, 64);
    gluDeleteQuadric(sphere);
  }
#else
  draw_cube();
#endif

  glPopMatrix();

  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_2D);
}

void draw_torus(GLdouble angle)
{
  angle = 0;

  glTexImage2D(GL_TEXTURE_2D, 0, 4, sphereW, sphereW, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, sphereMap[0]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  if (useSphereMaps) glEnable(GL_TEXTURE_2D);  

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);

  glColor3f(1, .5, .5);

  glPushMatrix();
  glRotatef(angle, 1, 0, 0);
  glRotatef(0, 0, 1, 0);

#if 0
  glutSolidTorus(.2, .25, 32, 32);
#else
  {
    GLUquadricObj *sphere = gluNewQuadric();
    gluSphere(sphere, .2, 64, 64);
  }
#endif

  glPopMatrix();

  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_2D);
}

void draw_scene(GLdouble degrees, GLint bits)
{
  glEnable(GL_CULL_FACE);
  draw_room();

  if (bits & TORUS_BIT) draw_torus(degrees); 
  if (bits & SPHERE_BIT) draw_sphere(degrees);
}

GLdouble get_secs(void)
{
  return glutGet(GLUT_ELAPSED_TIME) / 1000.0;
}

void draw_special_sphere(int tess)
{
  float r = 1.0, r1, r2, z1, z2;
  float theta, phi;
  int nlon = tess, nlat = tess;
  int i, j;

  glBegin(GL_TRIANGLE_FAN);
  theta = M_PI*1.0/nlat;
  r2 = r*sin(theta); z2 = r*cos(theta);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex4f(0.0, 0.0, r*r, r);
  for (j = 0, phi = 0.0; j <= nlon; j++, phi = 2*M_PI*j/nlon) {
    glNormal3f(r2*cos(phi), r2*sin(phi), z2);
    glVertex4f(r2*cos(phi)*z2, r2*sin(phi)*z2, z2*z2, z2); /* top */
  }
  glEnd();

  for (i = 2; i < nlat; i++) {
    theta = M_PI*i/nlat;
    r1 = r*sin(M_PI*(i-1)/nlat); z1 = r*cos(M_PI*(i-1)/nlat);
    r2 = r*sin(theta); z2 = r*cos(theta);

    if (fabs(z1) < 0.01 || fabs(z2) < 0.01)
      break;

    glBegin(GL_QUAD_STRIP);
    for (j = 0, phi = 0; j <= nlat; j++, phi = 2*M_PI*j/nlon) {
      glNormal3f(r1*cos(phi), r1*sin(phi), z1);
      glVertex4f(r1*cos(phi)*z1, r1*sin(phi)*z1, z1*z1, z1);
      glNormal3f(r2*cos(phi), r2*sin(phi), z2);
      glVertex4f(r2*cos(phi)*z2, r2*sin(phi)*z2, z2*z2, z2);
    }
    glEnd();
  }
}

void render_spheremap(void)
{
  GLfloat p[4];
  int i;

  glColor4f(1, 1, 1, 1);

#if 1
  glEnable(GL_TEXTURE_2D);
#endif

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

  p[0] = 2.0; p[1] = p[2] = p[3] = 0.0; /* 2zx */
  glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
  
  glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  p[0] = 0.0; p[1] = 2.0; p[2] = p[3] = 0.0; /* 2zy */
  glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
  
  glTexGenf(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  p[0] = p[1] = 0.0; p[2] = 0.0; p[3] = 2.0; /* 2z */
  glTexGenfv(GL_R, GL_OBJECT_PLANE, p);

  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_GEN_R);
  
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-.98, .98, -.98, .98, 1.0, 100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 0, 6,
	    0, 0, 0,
	    0, 1, 0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 1
  glClearColor(0.25, 0.25, 0.5, 1.0);
  glClearDepth(1.0);
  glClear(/* GL_COLOR_BUFFER_BIT | */GL_DEPTH_BUFFER_BIT);
#endif

  for (i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_2D, 0, 4, faceW, faceW, 0, 
		 GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)faceMap[i]);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(0.5, 0.5, 1.0);
    glTranslatef(1.0, 1.0, 0.0);
    glFrustum(-1.01, 1.01, -1.01, 1.01, 1.0, 100.0);
    if (angle2[i]) {
      glRotatef(angle2[i], axis2[i][0], axis2[i][1], axis2[i][2]);
    }
    glRotatef(angle1[i], axis1[i][0], axis1[i][1], axis1[i][2]);
    
    /* XXX atul does another angle thing here... */
    /* XXX atul does a third angle thing here... */
    
    glTranslatef(0.0, 0.0, -1.00);
    
    glMatrixMode(GL_MODELVIEW);
    glClear(GL_DEPTH_BUFFER_BIT);
    draw_special_sphere(20);
  }

  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_GEN_R);

  glDisable(GL_TEXTURE_2D);
}

void make_projection(int face, GLfloat xpos, GLfloat ypos, GLfloat zpos)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(90, 1, .01, 10);
  if (angle2[face]) {
      glRotatef(angle2[face], axis2[face][0], axis2[face][1], axis2[face][2]);
    }
  glRotatef(angle1[face], axis1[face][0], axis1[face][1], axis1[face][2]);
  gluLookAt(xpos, ypos, zpos,
	    ypos, ypos, zpos - 1,
	    0, 1, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void draw(void)
{
  static int frame = 0;
  GLenum err;
  GLdouble secs;
  static double degrees = 0;
  GLfloat sphereX, sphereY, sphereZ;
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  /* one revolution every 10 seconds... */
  if (animate) {
    secs = get_secs();
    secs = secs - 10.*trunc(secs / 10.);
    degrees = (secs/10.) * (360.);
  }
  
  if (frame == 0) {
    /* switch the viewport and draw the faces of the cube from the 
     * point of view of the square... */
  
    glViewport(w + 0*faceW, 0, faceW, faceW);
    make_projection(LEFT, 0, 0, 0);
    draw_scene(degrees, -1 & ~TORUS_BIT);
    glReadPixels(w + 0*faceW, 0, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[LEFT]);
    eliminate_alpha(faceW, faceW, faceMap[LEFT]);

    glViewport(w + 1*faceW, 0, faceW, faceW);
    make_projection(RIGHT, 0, 0, 0);
    draw_scene(degrees, -1 & ~TORUS_BIT);
    glReadPixels(w + 1*faceW, 0, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[RIGHT]);
    eliminate_alpha(faceW, faceW, faceMap[RIGHT]);

    glViewport(w + 2*faceW, 0, faceW, faceW);
    make_projection(BOTTOM, 0, 0, 0);
    draw_scene(degrees, -1 & ~TORUS_BIT);
    glReadPixels(w + 2*faceW, 0, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[BOTTOM]);
    eliminate_alpha(faceW, faceW, faceMap[BOTTOM]);

    glViewport(w + 0*faceW, faceW, faceW, faceW);
    make_projection(TOP, 0, 0, 0);
    draw_scene(degrees, -1 & ~TORUS_BIT);
    glReadPixels(w + 0*faceW, faceW, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[TOP]);
    eliminate_alpha(faceW, faceW, faceMap[TOP]);

    glViewport(w + 1*faceW, faceW, faceW, faceW);
    make_projection(FRONT, 0, 0, 0);
    draw_scene(degrees, -1 & ~TORUS_BIT);
    glReadPixels(w + 1*faceW, faceW, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[FRONT]);
    eliminate_alpha(faceW, faceW, faceMap[FRONT]);

    glViewport(w + 2*faceW, faceW, faceW, faceW);
    make_projection(BACK, 0, 0, 0);
    draw_scene(degrees, -1 & ~TORUS_BIT);
    glReadPixels(w + 2*faceW, faceW, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[BACK]);
    eliminate_alpha(faceW, faceW, faceMap[BACK]);

    /* create the sphere map for the cube... */
    glViewport(w, 2*faceW, sphereW, sphereW);
    render_spheremap();
    glReadPixels(w, 2*faceW, sphereW, sphereW, GL_RGBA, GL_UNSIGNED_BYTE,
		 sphereMap[0]);
  } else {
    sphereX = 
    sphereY = cos((degrees/360.) * 2.*M_PI);
    sphereZ = -sin((degrees/360.) * 2.*M_PI);

    glViewport(w + 0*faceW, 0, faceW, faceW);
    make_projection(LEFT, sphereX, sphereY, sphereZ);
    draw_scene(degrees, -1 & ~SPHERE_BIT);
    glReadPixels(w + 0*faceW, 0, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[LEFT]);
    eliminate_alpha(faceW, faceW, faceMap[LEFT]);

    glViewport(w + 1*faceW, 0, faceW, faceW);
    make_projection(RIGHT, sphereX, sphereY, sphereZ);
    draw_scene(degrees, -1 & ~SPHERE_BIT);
    glReadPixels(w + 1*faceW, 0, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[RIGHT]);
    eliminate_alpha(faceW, faceW, faceMap[RIGHT]);

    glViewport(w + 2*faceW, 0, faceW, faceW);
    make_projection(BOTTOM, sphereX, sphereY, sphereZ);
    draw_scene(degrees, -1 & ~SPHERE_BIT);
    glReadPixels(w + 2*faceW, 0, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[BOTTOM]);
    eliminate_alpha(faceW, faceW, faceMap[BOTTOM]);

    glViewport(w + 0*faceW, faceW, faceW, faceW);
    make_projection(TOP, sphereX, sphereY, sphereZ);
    draw_scene(degrees, -1 & ~SPHERE_BIT);
    glReadPixels(w + 0*faceW, faceW, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[TOP]);
    eliminate_alpha(faceW, faceW, faceMap[TOP]);

    glViewport(w + 1*faceW, faceW, faceW, faceW);
    make_projection(FRONT, sphereX, sphereY, sphereZ);
    draw_scene(degrees, -1 & ~SPHERE_BIT);
    glReadPixels(w + 1*faceW, faceW, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[FRONT]);
    eliminate_alpha(faceW, faceW, faceMap[FRONT]);

    glViewport(w + 2*faceW, faceW, faceW, faceW);
    make_projection(BACK, sphereX, sphereY, sphereZ);
    draw_scene(degrees, -1 & ~SPHERE_BIT);
    glReadPixels(w + 2*faceW, faceW, faceW, faceW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, faceMap[BACK]);
    eliminate_alpha(faceW, faceW, faceMap[BACK]);

    /* create the sphere map for the cube... */
    glViewport(w + sphereW, 2*faceW, sphereW, sphereW);
    render_spheremap();
    glReadPixels(w+sphereW, 2*faceW, sphereW, sphereW, 
		 GL_RGBA, GL_UNSIGNED_BYTE, sphereMap[1]);
  }
  frame = (frame == 0);

  /* draw both spheremaps */
  glViewport(w, 2*faceW, 2*sphereW, sphereW);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 2*sphereW, 0, sphereW, 0, 1);
  glRasterPos2i(0, 0);
  glDrawPixels(sphereW, sphereW, GL_RGBA, GL_UNSIGNED_BYTE, sphereMap[0]);
  glRasterPos2i(sphereW, 0);
  glDrawPixels(sphereW, sphereW, GL_RGBA, GL_UNSIGNED_BYTE, sphereMap[1]);


    /* draw the scene for the viewer's visual gratification... */
    glViewport(0, 0, w, h); 
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1, .01, 10);
    gluLookAt(0, 0, 0, 
	      0, 0, -1, 
	      0, 1, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -2.577);
    draw_scene(degrees, -1);
    glLoadIdentity();

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  static int idle = 1;
  switch(key) {
  case 'a': case 'A':
    animate = (animate == 0);
    printf("%sanimating\n", animate ? "" : "not ");
    break;
  case 'd': case 'D':
    printf("drawing\n");
    draw();
    break;
  case 'r': case 'R':
    printf("resetting sphere maps...\n");
    reset_textures();
    draw();
    break;
  case 't': case 'T':
    useSphereMaps = (useSphereMaps == 0);
    printf("%susing sphere maps\n", useSphereMaps ? "" : "not ");
    break;
  case 27:
    exit(0);
  default:
    if (idle) {
      glutIdleFunc(0);
    } else {
      glutIdleFunc(draw);
    }
    idle = (idle == 0);
    printf("%sdrawing when idle\n", idle ? "" : "not ");
    break;
  }
}

main(int argc, char *argv[])
{
    glutInitWindowSize(w*2, h);
    glutInitWindowPosition(0, 0);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutIdleFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    init(defaultFile);

    glutMainLoop();
    return 0;
}
