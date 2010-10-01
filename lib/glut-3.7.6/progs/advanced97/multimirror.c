#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

GLUquadricObj *cone, *base, *qsphere;

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
#define trunc(x) ((double)((int)(x)))
#endif

int draw_passes = 8;

int headsUp = 0;

typedef struct {
  GLfloat verts[4][3];
  GLfloat scale[3];
  GLfloat trans[3];
} Mirror;

Mirror mirrors[] = {
  /* mirror on the left wall */
  {{{-1., -.75, -.75}, {-1., .75, -.75}, {-1., .75, .75}, {-1, -.75, .75}},
     {-1, 1, 1}, {2, 0, 0}},

  /* mirror on right wall */
  {{{1., -.75, .75}, {1., .75, .75}, {1., .75, -.75}, {1., -.75, -.75}},
     {-1, 1, 1}, {-2, 0, 0}},
};
int nMirrors = 2;

void init(void)
{
  static GLfloat lightpos[] = {.5, .75, 1.5, 1};

  glEnable(GL_DEPTH_TEST); 
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  glEnable(GL_CULL_FACE);

  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  cone = gluNewQuadric();
  qsphere = gluNewQuadric();
}

void make_viewpoint(void)
{
  if (headsUp) {
    float width = (1 + 2*(draw_passes/nMirrors)) * 1.25;
    float height = (width / tan((30./360.) * (2.*M_PI))) + 1;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1, height - 3, height + 3);
    gluLookAt(0, height, 0,
	      0, 0, 0, 
	      0, 0, 1);
  } else {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1, .01, 4 + 2*(draw_passes / nMirrors));
    gluLookAt(-2, 0, .75, 
	      0, 0, 0, 
	      0, 1, 0);
  }
    
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void reshape(GLsizei w, GLsizei h) 
{
  glViewport(0, 0, w, h);
  make_viewpoint();
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

  /* back wall */
  glNormal3f(0, 0, -1);
  glVertex3f(-1, 1, 1);
  glVertex3f(1, 1, 1);
  glVertex3f(1, -1, 1);
  glVertex3f(-1, -1, 1);
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

  glPopMatrix();
}

void draw_sphere(GLdouble secs)
{
  static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
  GLfloat angle;

  /* one revolution every 10 seconds... */
  secs = secs - 10.*trunc(secs / 10.);
  angle = (secs/10.) * (360.);

  glPushMatrix();
  glTranslatef(0, -.3, 0);
  glRotatef(angle, 0, 1, 0);
  glTranslatef(.6, 0, 0);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
  gluSphere(qsphere, .3, 20, 20);

  glPopMatrix();
}

GLdouble get_secs(void)
{
  return glutGet(GLUT_ELAPSED_TIME) / 1000.0;
}

void draw_mirror(Mirror *m)
{
  glBegin(GL_QUADS);
  glVertex3fv(m->verts[0]);
  glVertex3fv(m->verts[1]);
  glVertex3fv(m->verts[2]);
  glVertex3fv(m->verts[3]);
  glEnd();
}

/* A note on matrix management:  it would be easier to use push and
 * pop to save and restore the matrices, but the projection matrix stack
 * is very shallow, so we just undo what we did.  In the extreme this
 * could lead to mathematic error. */

GLenum reflect_through_mirror(Mirror *m, GLenum cullFace)
{
  GLenum newCullFace = ((cullFace == GL_FRONT) ? GL_BACK : GL_FRONT);

  glMatrixMode(GL_PROJECTION);
  glScalef(m->scale[0], m->scale[1], m->scale[2]); 
  glTranslatef(m->trans[0], m->trans[1], m->trans[2]); 
  glMatrixMode(GL_MODELVIEW);

  /* must flip the cull face since reflection reverses the orientation
   * of the polygons */
  glCullFace(newCullFace);

  return newCullFace;
}

void undo_reflect_through_mirror(Mirror *m, GLenum cullFace)
{
  glMatrixMode(GL_PROJECTION);
  glTranslatef(-m->trans[0], -m->trans[1], -m->trans[2]);
  glScalef(1./m->scale[0], 1./m->scale[1], 1./m->scale[2]);
  glMatrixMode(GL_MODELVIEW);

  glCullFace(cullFace);
}

void draw_scene(GLdouble secs, int passes, GLenum cullFace, 
		GLuint stencilVal, GLuint mirror)
{
  GLenum newCullFace;
  int passesPerMirror, passesPerMirrorRem;
  unsigned int curMirror, drawMirrors;
  int i;

  /* one pass to draw the real scene */
  passes--;

  /* only draw in my designated locations */
  glStencilFunc(GL_EQUAL, stencilVal, 0xffffffff);

  /* draw things which may obscure the mirrors first */
  draw_sphere(secs);
  draw_cone();

  /* now draw the appropriate number of mirror reflections.  for
   * best results, we perform a depth-first traversal by allocating
   * a number of passes for each of the mirrors. */
  if (mirror != 0xffffffff) {
    passesPerMirror = passes / (nMirrors - 1);
    passesPerMirrorRem = passes % (nMirrors - 1);
    if (passes > nMirrors - 1) drawMirrors = nMirrors - 1;
    else drawMirrors = passes;
  } else {
    /* mirror == -1 means that this is the initial scene (there was no 
     * mirror) */
    passesPerMirror = passes / nMirrors;
    passesPerMirrorRem = passes % nMirrors;
    if (passes > nMirrors) drawMirrors = nMirrors;
    else drawMirrors = passes;
  }
  for (i = 0; drawMirrors > 0; i++) {
    curMirror = i % nMirrors;
    if (curMirror == mirror) continue;
    drawMirrors--;

    /* draw mirror into stencil buffer but not color or depth buffers */
    glColorMask(0, 0, 0, 0);
    glDepthMask(0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); 
    draw_mirror(&mirrors[curMirror]);
    glColorMask(1, 1, 1, 1);
    glDepthMask(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    /* draw reflected scene */
    newCullFace = reflect_through_mirror(&mirrors[curMirror], cullFace);
    if (passesPerMirrorRem) {
      draw_scene(secs, passesPerMirror + 1, newCullFace, stencilVal + 1, 
		 curMirror);      
      passesPerMirrorRem--;
    } else {
      draw_scene(secs, passesPerMirror, newCullFace, stencilVal + 1, 
		 curMirror);
    }
    undo_reflect_through_mirror(&mirrors[curMirror], cullFace);

    /* back to our stencil value */
    glStencilFunc(GL_EQUAL, stencilVal, 0xffffffff);    
  }

  draw_room(); 
}

void draw(void)
{
  GLenum err;
  GLfloat secs = get_secs();
  
  glDisable(GL_STENCIL_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  
  if (!headsUp) glEnable(GL_STENCIL_TEST);
  draw_scene(secs, draw_passes, GL_BACK, 0, (unsigned)-1);
  glDisable(GL_STENCIL_TEST);

  if (headsUp) {
    /* draw a red floor on the original scene */
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glColor3f(1, 0, 0);
    glVertex3f(-1, -.95, 1);
    glVertex3f(1, -.95, 1);
    glVertex3f(1, -.95, -1);
    glVertex3f(-1, -.95, -1);    
    glEnd();
    glEnable(GL_LIGHTING);
  }

  err = glGetError();
  if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
  
  glutSwapBuffers(); 
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  switch(key) {
  case '.':  case '>':  case '+':  case '=':
    draw_passes++;
    printf("Passes = %d\n", draw_passes);
    make_viewpoint();
    break;
  case ',':  case '<':  case '-':  case '_':
    draw_passes--;
    if (draw_passes < 1) draw_passes = 1;
    printf("Passes = %d\n", draw_passes);
    make_viewpoint();
    break;
  case 'h':  case 'H':
    /* heads up mode */
    headsUp = (headsUp == 0);
    make_viewpoint();
    break;
  case 27:
    exit(0);
  }
}

#define MIN_COLOR_BITS 4
#define MIN_DEPTH_BITS 8

main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(256, 256);
    glutInitWindowPosition(0, 0);
    if (argc > 1) {
      glutInitDisplayString("samples stencil>=3 rgb depth");
    } else { 
      glutInitDisplayString("samples stencil>=3 rgb double depth");
    }
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutIdleFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    init();

    glutMainLoop();
    return 0;
}

