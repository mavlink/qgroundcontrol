
/* haloed.c - by Tom McReynolds, SGI */

/* Draw haloed lines. */

#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

enum {CONE = 1};

/* Draw a cone */
void
cone(void)
{
  glPushMatrix();
  glTranslatef(0.f, 0.f, -30.f);
  glCallList(CONE);
  glPopMatrix();
}

/* Draw a torus */
void
torus(void)
{
  glutSolidTorus(10., 20., 16, 16);
}

enum {FILL, WIRE, HALO, OFFSET_HALO, BACKFACE_HALO, TOGGLE};

int rendermode = FILL;

void (*curobj)(void) = cone;

void
menu(int mode)
{
  if(mode == TOGGLE)
    if(curobj == cone)
      curobj = torus;
    else
      curobj = cone;
  else
    rendermode = mode;
  glutPostRedisplay();
}

int winWidth = 512;
int winHeight = 512;

/* used to get current width and height of viewport */
void
reshape(int wid, int ht)
{
  glViewport(0, 0, wid, ht);
  winWidth = wid;
  winHeight = ht;
}

GLfloat viewangle;


void
redraw(void)
{
    /* clear stencil each time */
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glPushMatrix();
    glRotatef(viewangle, 0.f, 1.f, 0.f);

    switch(rendermode) {
    case FILL:
      curobj();
      break;
    case WIRE:
      glDisable(GL_DEPTH_TEST);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glLineWidth(3.f);
      curobj();
      glLineWidth(1.f);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_DEPTH_TEST);
      break;
    case HALO:
      /* draw wide lines into depth buffer */
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
      glLineWidth(9.f);
      curobj();

      /* draw narrow lines into color with depth test on */
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glLineWidth(3.f);
      glDepthFunc(GL_LEQUAL);
      curobj();
      glDepthFunc(GL_LESS);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glLineWidth(1.f);
      break;
    case OFFSET_HALO:
      /* draw wide lines into depth buffer */
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
      glLineWidth(9.f);
      curobj();

      /* draw narrow lines into color with depth test on */
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glLineWidth(3.f);
#if GL_EXT_polygon_offset
      glEnable(GL_POLYGON_OFFSET_EXT);
      glPolygonOffsetEXT(-.5f, -.02f);
#endif
      glDepthFunc(GL_LEQUAL);
      curobj();
      glDepthFunc(GL_LESS);
#if GL_EXT_polygon_offset
      glDisable(GL_POLYGON_OFFSET_EXT);
#endif
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glLineWidth(1.f);
      break;
    case BACKFACE_HALO: /* cheat: only works on single non-intersecting obj */
      /* draw wide lines into depth buffer */
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);
      glLineWidth(3.f);

      curobj();

      /* mask out borders of objects with wide gray lines */
      glCullFace(GL_BACK);
      glLineWidth(9.f);
      glDisable(GL_LIGHTING);
      glColor3f(.7f, .7f, .7f);

      curobj();

      /* draw front face narrow lines without depth test */
      glEnable(GL_LIGHTING);
      glLineWidth(3.f);
      glDisable(GL_DEPTH_TEST);

      curobj();

      /* clean up */
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glLineWidth(1.f);
      break;
    }

    glPopMatrix();
    glutSwapBuffers();

    if(glGetError())
      printf("oops! Bad gl command!\n");
}

/* animate scene by rotating */
enum {ANIM_LEFT, ANIM_RIGHT};
int animDirection = ANIM_LEFT;

void anim(void)
{
  if(animDirection == ANIM_LEFT)
    viewangle -= 1.f;
  else
    viewangle += 1.f;
  glutPostRedisplay();
}

/* ARGSUSED1 */
/* special keys, like array and F keys */
void special(int key, int x, int y)
{
  switch(key) {
  case GLUT_KEY_LEFT:
    glutIdleFunc(anim);
    animDirection = ANIM_LEFT;
    break;
  case GLUT_KEY_RIGHT:
    glutIdleFunc(anim);
    animDirection = ANIM_RIGHT;
    break;
  case GLUT_KEY_UP:
  case GLUT_KEY_DOWN:
    glutIdleFunc(0);
    break;
  }
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  switch(key) {
  case 'a':
    viewangle -= 10.f;
    glutPostRedisplay();
    break;
  case 's':
    viewangle += 10.f;
    glutPostRedisplay();
    break;
  case '\033':
    exit(0);
  }
}


int picked_object;
int xpos = 0, ypos = 0;
int newxpos, newypos;
int startx, starty;

int
main(int argc, char **argv)
{
    static GLfloat lightpos[] = {25.f, 50.f, -50.f, 1.f};
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    GLUquadricObj *cone, *base;

    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_STENCIL|GLUT_DEPTH|GLUT_DOUBLE);
    (void)glutCreateWindow("haloed lines");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);

    glutCreateMenu(menu);
    glutAddMenuEntry("Filled Object", FILL);
    glutAddMenuEntry("Wireframe", WIRE);
    glutAddMenuEntry("Haloed Wireframe", HALO);
    glutAddMenuEntry("Pgon Offset Haloed Wireframe", OFFSET_HALO);
    glutAddMenuEntry("Backface Haloed Wireframe", BACKFACE_HALO);
    glutAddMenuEntry("Toggle Object", TOGGLE);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glClearColor(.7f, .7f, .7f, .7f);

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  

    /* make display list for cone; for efficiency */

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cone_mat);
    gluQuadricOrientation(base, GLU_INSIDE);
    gluDisk(base, 0., 25., 8, 1);
    gluCylinder(cone, 25., 0., 60., 8, 8);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    glMatrixMode(GL_PROJECTION);
    glOrtho(-50., 50., -50., 50., -50., 50.);
    glMatrixMode(GL_MODELVIEW);
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
