#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
#define trunc(x) ((double)((int)(x)))
#endif

#ifdef _WIN32
#define random() ((long)rand() + (rand() << 15) + (rand() << 30))
#endif

GLUquadricObj *cone, *base, *qsphere;

void create_stipple_pattern(GLuint *pat, GLfloat opacity)
{
  int x, y;
  long threshold = (float)0x7fffffff * (1. - opacity);

  for (y = 0; y < 32; y++) {
    pat[y] = 0;
    for (x = 0; x < 32; x++) {
      if (random() > threshold) pat[y] |= (1 << x);
    }
  }
}

void init(void)
{
  static GLfloat lightpos[] = {.5, .75, 1.5, 1};
  GLuint spherePattern[32];

  glEnable(GL_DEPTH_TEST); 
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  cone = gluNewQuadric();
  base = gluNewQuadric();
  qsphere = gluNewQuadric();
  gluQuadricOrientation(base, GLU_INSIDE);

  create_stipple_pattern(spherePattern, .5);
  glPolygonStipple((GLubyte *)spherePattern);
}

void reshape(GLsizei w, GLsizei h) 
{
  glViewport(0, 0, w, h);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, 1, .01, 10);
  gluLookAt(0, 0, 2.577, 0, 0, -5, 0, 1, 0);
  
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
  glVertex3f(-1, -1, 1);
  glVertex3f(-1, 1, 1);
  glVertex3f(-1, 1, -1);

  /* right wall */
  glNormal3f(-1, 0, 0);
  glVertex3f(1, 1, -1);
  glVertex3f(1, 1, 1);
  glVertex3f(1, -1, 1);
  glVertex3f(1, -1, -1);

  /* far wall */
  glNormal3f(0, 0, 1);
  glVertex3f(-1, -1, -1);
  glVertex3f(1, -1, -1);
  glVertex3f(1, 1, -1);
  glVertex3f(-1, 1, -1);

  glEnd();
}

void draw_cone(void)
{
  static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};

  glPushMatrix();
  glTranslatef(0, -1, 0);
  glRotatef(-90, 1, 0, 0);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
  gluCylinder(cone, .3, 0, 1.25, 20, 1);
  gluDisk(base, 0., .3, 20, 1); 

  glPopMatrix();
}

void draw_sphere(GLdouble angle)
{
  static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};

  glPushMatrix();
  glTranslatef(0, -.3, 0);
  glRotatef(angle, 0, 1, 0);
  glTranslatef(0, 0, .6);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
  gluSphere(qsphere, .3, 20, 20);

  glPopMatrix();
}

GLdouble get_secs(void)
{
  return glutGet(GLUT_ELAPSED_TIME) / 1000.0;
}

void draw(void)
{
    GLenum err;
    GLdouble secs;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_room();
    draw_cone();
    secs = get_secs();

    /* draw the transparent object... */
    glEnable(GL_POLYGON_STIPPLE);
    draw_sphere(secs * 360. / 10.);
    glDisable(GL_POLYGON_STIPPLE);

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  if (key == 27) exit(0);
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
    init();

    glutMainLoop();
    return 0;
}

