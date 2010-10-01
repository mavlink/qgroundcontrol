
/* shadowvol.c - by Tom McReynolds, SGI */

/* Shadows: Shadow maps */

#include <GL/glut.h>
#include <stdlib.h>

/* Demonstrate shadow volumes */

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
  SPHERE = 1, CONE, LIGHT, SHADOWVOL
};

typedef struct {
  GLfloat *verticies;
  GLfloat *normal;
  int n;                /* number of verticies */
} ShadObj;

GLfloat shadVerts[] =
{30.f, 30.f, -350.f,
  60.f, 20.f, -340.f,
  40.f, 40.f, -400.f};

GLfloat shadNormal[] =
{1.f, 1.f, 1.f};
ShadObj shadower;

enum {
  X, Y, Z
};

/* simple way to extend a point to build shadow volume */
void
extend(GLfloat new[3], GLfloat light[3], GLfloat vertex[3], GLfloat t)
{
  GLfloat delta[3];

  delta[X] = vertex[X] - light[X];
  delta[Y] = vertex[Y] - light[Y];
  delta[Z] = vertex[Z] - light[Z];

  new[X] = light[X] + delta[X] * t;
  new[Y] = light[Y] + delta[Y] * t;
  new[Z] = light[Z] + delta[Z] * t;
}

/* Create a shadow volume in a display list */
/* XXX light should have 4 compoents */
void
makeShadowVolume(ShadObj * shadower, GLfloat light[3],
  GLfloat t, GLint dlist)
{
  int i;
  GLfloat newv[3];

  glNewList(dlist, GL_COMPILE);
  glDisable(GL_LIGHTING);
  glBegin(GL_QUADS);
  /* for debugging */
  glColor3f(.2f, .8f, .4f);
  for (i = 0; i < shadower->n; i++) {
    glVertex3fv(&shadower->verticies[i * 3]);
    extend(newv, light, &shadower->verticies[i * 3], t);
    glVertex3fv(newv);
    extend(newv, light, &shadower->verticies[((i + 1) % shadower->n) * 3],
      t);
    glVertex3fv(newv);
    glVertex3fv(&shadower->verticies[((i + 1) % shadower->n) * 3]);
  }
  glEnd();
  glEnable(GL_LIGHTING);
  glEndList();
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
  NONE, NOLIGHT, VOLUME, SHADOW
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
{50.f, 50.f, -340.f, 1.f};

/* render while jittering the shadows */
void
render(ShadObj * obj)
{
  static GLfloat shad_mat[] =
  {1.f, .1f, .1f, 1.f};
  GLfloat *v;           /* vertex pointer */
  int i;

  /* material properties for objects in scene */
  static GLfloat wall_mat[] =
  {1.f, 1.f, 1.f, 1.f};

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

  /* walls */

  glBegin(GL_QUADS);
  /* left wall */
  glNormal3f(1.f, 0.f, 0.f);
  glVertex3f(-100.f, -100.f, -320.f);
  glVertex3f(-100.f, -100.f, -520.f);
  glVertex3f(-100.f, 100.f, -520.f);
  glVertex3f(-100.f, 100.f, -320.f);

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

  cone();

  sphere();

  glCallList(LIGHT);

  /* draw shadowing object */
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, shad_mat);
  glBegin(GL_POLYGON);
  glNormal3fv(obj->normal);
  for (v = obj->verticies, i = 0; i < obj->n; i++) {
    glVertex3fv(v);
    v += 3;
  }
  glEnd();

}

void
redraw(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  switch (rendermode) {
  case NONE:
    render(&shadower);
    break;
  case NOLIGHT:
    glDisable(GL_LIGHT0);
    render(&shadower);
    glEnable(GL_LIGHT0);
    break;
  case VOLUME:
    render(&shadower);
    glCallList(SHADOWVOL);
    break;
  case SHADOW:
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(&shadower);  /* render scene in depth buffer */

    glEnable(GL_STENCIL_TEST);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_ALWAYS, 0, 0);

    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glCullFace(GL_BACK);  /* increment using front face of shadow volume */
    glCallList(SHADOWVOL);

    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    glCullFace(GL_FRONT);  /* increment using front face of shadow volume */
    glCallList(SHADOWVOL);

    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LEQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glStencilFunc(GL_EQUAL, 1, 1);  /* draw shadowed part */
    glDisable(GL_LIGHT0);
    render(&shadower);

    glStencilFunc(GL_EQUAL, 0, 1);  /* draw lit part */
    glEnable(GL_LIGHT0);
    render(&shadower);

    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
    break;
  }

  glutSwapBuffers();    /* high end machines may need this */
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
  static GLfloat sphere_mat[] =
  {1.f, .5f, 0.f, 1.f};
  static GLfloat cone_mat[] =
  {0.f, .5f, 1.f, 1.f};

  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
  (void) glutCreateWindow("shadow volumes");
  glutDisplayFunc(redraw);
  glutKeyboardFunc(key);

  glutCreateMenu(menu);
  glutAddMenuEntry("No Shadows", NONE);
  glutAddMenuEntry("No Light", NOLIGHT);
  glutAddMenuEntry("Show Volume", VOLUME);
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
  glEnable(GL_CULL_FACE);

  /* place light 0 in the right place */
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  /* remove back faces to speed things up */
  glCullFace(GL_BACK);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  /* make display lists for sphere and cone; for efficiency */

  glNewList(SPHERE, GL_COMPILE);
  sphere = gluNewQuadric();
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
  gluSphere(sphere, 20.f, 20, 20);
  gluDeleteQuadric(sphere);
  glEndList();

  glNewList(CONE, GL_COMPILE);
  cone = gluNewQuadric();
  base = gluNewQuadric();
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
  glRotatef(-90.f, 1.f, 0.f, 0.f);
  gluDisk(base, 0., 20., 20, 1);
  gluCylinder(cone, 20., 0., 60., 20, 20);
  gluDeleteQuadric(cone);
  gluDeleteQuadric(base);
  glEndList();

  glNewList(LIGHT, GL_COMPILE);
  sphere = gluNewQuadric();
  glPushMatrix();
  glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
  glDisable(GL_LIGHTING);
  glColor3f(.9f, .9f, .6f);
  gluSphere(sphere, 5.f, 20, 20);
  glEnable(GL_LIGHTING);
  glPopMatrix();
  gluDeleteQuadric(sphere);
  glEndList();

  /* load pattern for current 2d texture */
  tex = make_texture(TEXDIM, TEXDIM);
  glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
  free(tex);

  shadower.verticies = shadVerts;
  shadower.normal = shadNormal;
  shadower.n = sizeof(shadVerts) / (3 * sizeof(GLfloat));

  makeShadowVolume(&shadower, lightpos, 10.f, SHADOWVOL);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
