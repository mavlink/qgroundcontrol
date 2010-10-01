
/* softshadow.c - by Tom McReynolds, SGI */

/* Using the accumulation buffer for soft shadows. */

#include <GL/glut.h>
#include <stdlib.h>

/* Demonstrate the use of accumulation buffer to create soft shadows */

/* Create a single component texture map */
GLfloat *
make_texture(int maxs, int maxt)
{
  int s, t;
  static GLfloat *texture;

  texture = (GLfloat *) malloc(maxs * maxt * sizeof(GLfloat));
  for (t = 0; t < maxt; t++) {
    for (s = 0; s < maxs; s++) {
      texture[s + maxs * t] = ((s >> 4) & 0x1) ^ ((t >> 4) & 0x1);
    }
  }
  return texture;
}

enum {
  SPHERE = 1, CONE, LIGHT, LEFTWALL, FLOOR
};

/* create a matrix that will project the desired shadow */
void
shadowmatrix(GLfloat shadowMat[4][4],
  GLfloat groundplane[4],
  GLfloat lightpos[4])
{
  GLfloat dot;

  /* find dot product between light position vector and ground plane normal */
  dot = groundplane[0] * lightpos[0] +
    groundplane[1] * lightpos[1] +
    groundplane[2] * lightpos[2] +
    groundplane[3] * lightpos[3];

  shadowMat[0][0] = dot - lightpos[0] * groundplane[0];
  shadowMat[1][0] = 0.f - lightpos[0] * groundplane[1];
  shadowMat[2][0] = 0.f - lightpos[0] * groundplane[2];
  shadowMat[3][0] = 0.f - lightpos[0] * groundplane[3];

  shadowMat[0][1] = 0.f - lightpos[1] * groundplane[0];
  shadowMat[1][1] = dot - lightpos[1] * groundplane[1];
  shadowMat[2][1] = 0.f - lightpos[1] * groundplane[2];
  shadowMat[3][1] = 0.f - lightpos[1] * groundplane[3];

  shadowMat[0][2] = 0.f - lightpos[2] * groundplane[0];
  shadowMat[1][2] = 0.f - lightpos[2] * groundplane[1];
  shadowMat[2][2] = dot - lightpos[2] * groundplane[2];
  shadowMat[3][2] = 0.f - lightpos[2] * groundplane[3];

  shadowMat[0][3] = 0.f - lightpos[3] * groundplane[0];
  shadowMat[1][3] = 0.f - lightpos[3] * groundplane[1];
  shadowMat[2][3] = 0.f - lightpos[3] * groundplane[2];
  shadowMat[3][3] = dot - lightpos[3] * groundplane[3];

}

enum {
  X, Y, Z
};
enum {
  A, B, C, D
};

/* find the plane equation given 3 points */
void
findplane(GLfloat plane[4],
  GLfloat v0[3], GLfloat v1[3], GLfloat v2[3])
{
  GLfloat vec0[3], vec1[3];

  /* need 2 vectors to find cross product */
  vec0[X] = v1[X] - v0[X];
  vec0[Y] = v1[Y] - v0[Y];
  vec0[Z] = v1[Z] - v0[Z];

  vec1[X] = v2[X] - v0[X];
  vec1[Y] = v2[Y] - v0[Y];
  vec1[Z] = v2[Z] - v0[Z];

  /* find cross product to get A, B, and C of plane equation */
  plane[A] = vec0[Y] * vec1[Z] - vec0[Z] * vec1[Y];
  plane[B] = -(vec0[X] * vec1[Z] - vec0[Z] * vec1[X]);
  plane[C] = vec0[X] * vec1[Y] - vec0[Y] * vec1[X];

  plane[D] = -(plane[A] * v0[X] + plane[B] * v0[Y] + plane[C] * v0[Z]);
}

void 
sphere(void)
{
  glPushMatrix();
  glTranslatef(60.f, -50.f, -360.f);
  glCallList(SPHERE);
  glPopMatrix();
}

void 
cone(void)
{
  glPushMatrix();
  glTranslatef(-40.f, -40.f, -400.f);
  glCallList(CONE);
  glPopMatrix();

}

enum {
  NONE, SHADOW
};

int rendermode = NONE;

void
menu(int mode)
{
  rendermode = mode;
  glutPostRedisplay();
}

GLfloat leftwallshadow[4][4];
GLfloat floorshadow[4][4];

GLfloat lightpos[] =
{50.f, 50.f, -320.f, 1.f};

/* render while jittering the shadows */
void
render(GLfloat dx, GLfloat dy, GLfloat dz)
{

  /* material properties for objects in scene */
  static GLfloat wall_mat[] =
  {1.f, 1.f, 1.f, 1.f};
  static GLfloat sphere_mat[] =
  {1.f, .5f, 0.f, 1.f};
  static GLfloat cone_mat[] =
  {0.f, .5f, 1.f, 1.f};

  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  /* Note: wall verticies are ordered so they are all front facing this lets
     me do back face culling to speed things up.  */

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

  /* floor */
  /* make the floor textured */
  glEnable(GL_TEXTURE_2D);

  /* Since we want to turn texturing on for floor only, we have to make floor 
     a separate glBegin()/glEnd() sequence. You can't turn texturing on and
     off between begin and end calls */
  glBegin(GL_QUADS);
  glNormal3f(0.f, 1.f, 0.f);
  glTexCoord2i(0, 0);
  glVertex3f(-100.f, -100.f, -320.f);
  glTexCoord2i(1, 0);
  glVertex3f(100.f, -100.f, -320.f);
  glTexCoord2i(1, 1);
  glVertex3f(100.f, -100.f, -520.f);
  glTexCoord2i(0, 1);
  glVertex3f(-100.f, -100.f, -520.f);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  if (rendermode == SHADOW) {

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glColor3f(0.f, 0.f, 0.f);  /* shadow color */

    glPushMatrix();
    glMultMatrixf((GLfloat *) floorshadow);
    glTranslatef(dx, dy, dz);
    cone();
    glPopMatrix();

    glPushMatrix();
    glMultMatrixf((GLfloat *) floorshadow);
    glTranslatef(dx, dy, dz);
    sphere();
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
  }
  /* walls */

  if (rendermode == SHADOW) {
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  }
  glBegin(GL_QUADS);
  /* left wall */
  glNormal3f(1.f, 0.f, 0.f);
  glVertex3f(-100.f, -100.f, -320.f);
  glVertex3f(-100.f, -100.f, -520.f);
  glVertex3f(-100.f, 100.f, -520.f);
  glVertex3f(-100.f, 100.f, -320.f);
  glEnd();

  if (rendermode == SHADOW) {
    glStencilFunc(GL_EQUAL, 1, 1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glColor3f(0.f, 0.f, 0.f);  /* shadow color */
    glDisable(GL_DEPTH_TEST);
    glPushMatrix();
    glMultMatrixf((GLfloat *) leftwallshadow);
    glTranslatef(dx, dy, dz);
    cone();
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
  }
  glBegin(GL_QUADS);
  /* right wall */
  glNormal3f(-1.f, 0.f, 0.f);
  glVertex3f(100.f, -100.f, -320.f);
  glVertex3f(100.f, 100.f, -320.f);
  glVertex3f(100.f, 100.f, -520.f);
  glVertex3f(100.f, -100.f, -520.f);

  /* ceiling */
  glNormal3f(0.f, -1.f, 0.f);
  glVertex3f(-100.f, 100.f, -320.f);
  glVertex3f(-100.f, 100.f, -520.f);
  glVertex3f(100.f, 100.f, -520.f);
  glVertex3f(100.f, 100.f, -320.f);

  /* back wall */
  glNormal3f(0.f, 0.f, 1.f);
  glVertex3f(-100.f, -100.f, -520.f);
  glVertex3f(100.f, -100.f, -520.f);
  glVertex3f(100.f, 100.f, -520.f);
  glVertex3f(-100.f, 100.f, -520.f);
  glEnd();

  glPushMatrix();
  glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
  glDisable(GL_LIGHTING);
  glColor3f(1.f, 1.f, .7f);
  glCallList(LIGHT);
  glEnable(GL_LIGHTING);
  glPopMatrix();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
  cone();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
  sphere();

}

void
redraw(void)
{
  int dx, dy, dz;

  dy = 0;
  /* jitter the light around */
  if (rendermode == SHADOW) {
    glClear(GL_ACCUM_BUFFER_BIT);
    for (dz = -4; dz < 5; dz += 2) {
      for (dx = -4; dx < 5; dx += 2) {
        render((GLfloat) dx, (GLfloat) dy, (GLfloat) dz);
        glAccum(GL_ACCUM, 1.f / 25);
      }
    }
    glAccum(GL_RETURN, 1.f);
  } else
    render(0.f, 0.f, 0.f);

  glutSwapBuffers();
}

/* ARGSUSED1 */
void 
key(unsigned char key, int x, int y)
{
  if (key == '\033')
    exit(0);
}

const int TEXDIM = 256;
/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
  GLfloat *tex;
  GLUquadricObj *sphere, *cone, *base;
  GLfloat plane[4];
  GLfloat v0[3], v1[3], v2[3];

  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE | GLUT_ACCUM);
  (void) glutCreateWindow("soft shadows");
  glutDisplayFunc(redraw);
  glutKeyboardFunc(key);

  glutCreateMenu(menu);
  glutAddMenuEntry("No Shadows", NONE);
  glutAddMenuEntry("Shadows", SHADOW);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* draw a perspective scene */
  glMatrixMode(GL_PROJECTION);
  glFrustum(-100., 100., -100., 100., 320., 640.);
  glMatrixMode(GL_MODELVIEW);

  /* turn on features */
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  /* make shadow matricies */

  /* 3 points on floor */
  v0[X] = -100.f;
  v0[Y] = -100.f;
  v0[Z] = -320.f;
  v1[X] = 100.f;
  v1[Y] = -100.f;
  v1[Z] = -320.f;
  v2[X] = 100.f;
  v2[Y] = -100.f;
  v2[Z] = -520.f;

  findplane(plane, v0, v1, v2);
  shadowmatrix(floorshadow, plane, lightpos);

  /* 3 points on left wall */
  v0[X] = -100.f;
  v0[Y] = -100.f;
  v0[Z] = -320.f;
  v1[X] = -100.f;
  v1[Y] = -100.f;
  v1[Z] = -520.f;
  v2[X] = -100.f;
  v2[Y] = 100.f;
  v2[Z] = -520.f;

  findplane(plane, v0, v1, v2);
  shadowmatrix(leftwallshadow, plane, lightpos);

  /* place light 0 in the right place */
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  /* remove back faces to speed things up */
  glCullFace(GL_BACK);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  /* make display lists for sphere and cone; for efficiency */

  glNewList(SPHERE, GL_COMPILE);
  sphere = gluNewQuadric();
  gluSphere(sphere, 20.f, 20, 20);
  gluDeleteQuadric(sphere);
  glEndList();

  glNewList(LIGHT, GL_COMPILE);
  sphere = gluNewQuadric();
  gluSphere(sphere, 5.f, 20, 20);
  gluDeleteQuadric(sphere);
  glEndList();

  glNewList(CONE, GL_COMPILE);
  cone = gluNewQuadric();
  base = gluNewQuadric();
  glRotatef(-90.f, 1.f, 0.f, 0.f);
  gluDisk(base, 0., 20., 20, 1);
  gluCylinder(cone, 20., 0., 60., 20, 20);
  gluDeleteQuadric(cone);
  gluDeleteQuadric(base);
  glEndList();

  glNewList(FLOOR, GL_COMPILE);
  glEndList();

  glNewList(LEFTWALL, GL_COMPILE);
  glEndList();

  /* load pattern for current 2d texture */
  tex = make_texture(TEXDIM, TEXDIM);
  glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
  free(tex);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
