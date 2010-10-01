/**
 * surfgrid.c - simple test of polygon offset
 *
 * GLUT distribution version  $Revision: 1.8 $
 *
 * usage:
 *	surfgrid [-f]
 *
 * options:
 *	-f	run on full screen
 *
 * keys:
 *	p	toggle polygon offset
 *      F       increase polygon offset factor
 *      f       decrease polygon offset factor
 *      B       increase polygon offset bias
 *      b       decrease polygon offset bias
 *	g	toggle grid drawing
 *	s	toggle smooth/flat shading
 *	n	toggle whether to use GL evaluators or GLU nurbs
 *	u	decr number of segments in U direction
 *	U	incr number of segments in U direction
 *	v	decr number of segments in V direction
 *	V	incr number of segments in V direction
 *	escape	quit
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define W 600
#define H 600

float z_axis[] =
{0.0, 0.0, 1.0};

void
norm(float v[3])
{
  float r;

  r = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

  v[0] /= r;
  v[1] /= r;
  v[2] /= r;
}

void
cross(float v1[3], float v2[3], float result[3])
{
  result[0] = v1[1] * v2[2] - v1[2] * v2[1];
  result[1] = v1[2] * v2[0] - v1[0] * v2[2];
  result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float
length(float v[3])
{
  float r = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  return r;
}

static int winwidth = W, winheight = H;
GLUnurbsObj *nobj;
GLuint surflist, gridlist;

int useglunurbs = 0;
int smooth = 1;
GLboolean tracking = GL_FALSE;
int showgrid = 1;
int showsurf = 1;
int fullscreen = 0;
float modelmatrix[16];
float factor = 0.5;
float bias = 0.002;
int usegments = 4;
int vsegments = 4;

int spindx, spindy;
int startx, starty;
int curx, cury;
int prevx, prevy;       /* to get good deltas using glut */

void redraw(void);
void createlists(void);

/* Control points of the torus in Bezier form.  Can be rendered
   using OpenGL evaluators. */
static GLfloat torusbezierpts[] =
{
/* *INDENT-OFF* */
   4.0, 0.0, 0.0, 4.0, 2.0, 0.0, 1.0, 2.0, 3.0, 0.0, 1.0, 2.0,
   3.0, 0.0, 1.0, 2.0, 4.0, 0.0, 1.0, 2.0, 8.0, 0.0, 0.0, 4.0,
   8.0, 0.0, 0.0, 4.0, 4.0, 0.0,-1.0, 2.0, 3.0, 0.0,-1.0, 2.0,
   3.0, 0.0,-1.0, 2.0, 2.0, 0.0,-1.0, 2.0, 4.0, 0.0, 0.0, 4.0,
   2.0,-2.0, 0.0, 2.0, 1.0,-1.0, 0.5, 1.0, 1.5,-1.5, 0.5, 1.0,
   1.5,-1.5, 0.5, 1.0, 2.0,-2.0, 0.5, 1.0, 4.0,-4.0, 0.0, 2.0,
   4.0,-4.0, 0.0, 2.0, 2.0,-2.0,-0.5, 1.0, 1.5,-1.5,-0.5, 1.0,
   1.5,-1.5,-0.5, 1.0, 1.0,-1.0,-0.5, 1.0, 2.0,-2.0, 0.0, 2.0,
   0.0,-2.0, 0.0, 2.0, 0.0,-1.0, 0.5, 1.0, 0.0,-1.5, 0.5, 1.0,
   0.0,-1.5, 0.5, 1.0, 0.0,-2.0, 0.5, 1.0, 0.0,-4.0, 0.0, 2.0,
   0.0,-4.0, 0.0, 2.0, 0.0,-2.0,-0.5, 1.0, 0.0,-1.5,-0.5, 1.0,
   0.0,-1.5,-0.5, 1.0, 0.0,-1.0,-0.5, 1.0, 0.0,-2.0, 0.0, 2.0,
   0.0,-2.0, 0.0, 2.0, 0.0,-1.0, 0.5, 1.0, 0.0,-1.5, 0.5, 1.0,
   0.0,-1.5, 0.5, 1.0, 0.0,-2.0, 0.5, 1.0, 0.0,-4.0, 0.0, 2.0,
   0.0,-4.0, 0.0, 2.0, 0.0,-2.0,-0.5, 1.0, 0.0,-1.5,-0.5, 1.0,
   0.0,-1.5,-0.5, 1.0, 0.0,-1.0,-0.5, 1.0, 0.0,-2.0, 0.0, 2.0,
  -2.0,-2.0, 0.0, 2.0,-1.0,-1.0, 0.5, 1.0,-1.5,-1.5, 0.5, 1.0,
  -1.5,-1.5, 0.5, 1.0,-2.0,-2.0, 0.5, 1.0,-4.0,-4.0, 0.0, 2.0,
  -4.0,-4.0, 0.0, 2.0,-2.0,-2.0,-0.5, 1.0,-1.5,-1.5,-0.5, 1.0,
  -1.5,-1.5,-0.5, 1.0,-1.0,-1.0,-0.5, 1.0,-2.0,-2.0, 0.0, 2.0,
  -4.0, 0.0, 0.0, 4.0,-2.0, 0.0, 1.0, 2.0,-3.0, 0.0, 1.0, 2.0,
  -3.0, 0.0, 1.0, 2.0,-4.0, 0.0, 1.0, 2.0,-8.0, 0.0, 0.0, 4.0,
  -8.0, 0.0, 0.0, 4.0,-4.0, 0.0,-1.0, 2.0,-3.0, 0.0,-1.0, 2.0,
  -3.0, 0.0,-1.0, 2.0,-2.0, 0.0,-1.0, 2.0,-4.0, 0.0, 0.0, 4.0,
  -4.0, 0.0, 0.0, 4.0,-2.0, 0.0, 1.0, 2.0,-3.0, 0.0, 1.0, 2.0,
  -3.0, 0.0, 1.0, 2.0,-4.0, 0.0, 1.0, 2.0,-8.0, 0.0, 0.0, 4.0,
  -8.0, 0.0, 0.0, 4.0,-4.0, 0.0,-1.0, 2.0,-3.0, 0.0,-1.0, 2.0,
  -3.0, 0.0,-1.0, 2.0,-2.0, 0.0,-1.0, 2.0,-4.0, 0.0, 0.0, 4.0,
  -2.0, 2.0, 0.0, 2.0,-1.0, 1.0, 0.5, 1.0,-1.5, 1.5, 0.5, 1.0,
  -1.5, 1.5, 0.5, 1.0,-2.0, 2.0, 0.5, 1.0,-4.0, 4.0, 0.0, 2.0,
  -4.0, 4.0, 0.0, 2.0,-2.0, 2.0,-0.5, 1.0,-1.5, 1.5,-0.5, 1.0,
  -1.5, 1.5,-0.5, 1.0,-1.0, 1.0,-0.5, 1.0,-2.0, 2.0, 0.0, 2.0,
   0.0, 2.0, 0.0, 2.0, 0.0, 1.0, 0.5, 1.0, 0.0, 1.5, 0.5, 1.0,
   0.0, 1.5, 0.5, 1.0, 0.0, 2.0, 0.5, 1.0, 0.0, 4.0, 0.0, 2.0,
   0.0, 4.0, 0.0, 2.0, 0.0, 2.0,-0.5, 1.0, 0.0, 1.5,-0.5, 1.0,
   0.0, 1.5,-0.5, 1.0, 0.0, 1.0,-0.5, 1.0, 0.0, 2.0, 0.0, 2.0,
   0.0, 2.0, 0.0, 2.0, 0.0, 1.0, 0.5, 1.0, 0.0, 1.5, 0.5, 1.0,
   0.0, 1.5, 0.5, 1.0, 0.0, 2.0, 0.5, 1.0, 0.0, 4.0, 0.0, 2.0,
   0.0, 4.0, 0.0, 2.0, 0.0, 2.0,-0.5, 1.0, 0.0, 1.5,-0.5, 1.0,
   0.0, 1.5,-0.5, 1.0, 0.0, 1.0,-0.5, 1.0, 0.0, 2.0, 0.0, 2.0,
   2.0, 2.0, 0.0, 2.0, 1.0, 1.0, 0.5, 1.0, 1.5, 1.5, 0.5, 1.0,
   1.5, 1.5, 0.5, 1.0, 2.0, 2.0, 0.5, 1.0, 4.0, 4.0, 0.0, 2.0,
   4.0, 4.0, 0.0, 2.0, 2.0, 2.0,-0.5, 1.0, 1.5, 1.5,-0.5, 1.0,
   1.5, 1.5,-0.5, 1.0, 1.0, 1.0,-0.5, 1.0, 2.0, 2.0, 0.0, 2.0,
   4.0, 0.0, 0.0, 4.0, 2.0, 0.0, 1.0, 2.0, 3.0, 0.0, 1.0, 2.0,
   3.0, 0.0, 1.0, 2.0, 4.0, 0.0, 1.0, 2.0, 8.0, 0.0, 0.0, 4.0,
   8.0, 0.0, 0.0, 4.0, 4.0, 0.0,-1.0, 2.0, 3.0, 0.0,-1.0, 2.0,
   3.0, 0.0,-1.0, 2.0, 2.0, 0.0,-1.0, 2.0, 4.0, 0.0, 0.0, 4.0,
/* *INDENT-ON* */

};

/* Control points of a torus in NURBS form.  Can be rendered using
   the GLU NURBS routines. */
static GLfloat torusnurbpts[] =
{
/* *INDENT-OFF* */
   4.0, 0.0, 0.0, 4.0, 2.0, 0.0, 1.0, 2.0, 4.0, 0.0, 1.0, 2.0,
   8.0, 0.0, 0.0, 4.0, 4.0, 0.0,-1.0, 2.0, 2.0, 0.0,-1.0, 2.0,
   4.0, 0.0, 0.0, 4.0, 2.0,-2.0, 0.0, 2.0, 1.0,-1.0, 0.5, 1.0,
   2.0,-2.0, 0.5, 1.0, 4.0,-4.0, 0.0, 2.0, 2.0,-2.0,-0.5, 1.0,
   1.0,-1.0,-0.5, 1.0, 2.0,-2.0, 0.0, 2.0,-2.0,-2.0, 0.0, 2.0,
  -1.0,-1.0, 0.5, 1.0,-2.0,-2.0, 0.5, 1.0,-4.0,-4.0, 0.0, 2.0,
  -2.0,-2.0,-0.5, 1.0,-1.0,-1.0,-0.5, 1.0,-2.0,-2.0, 0.0, 2.0,
  -4.0, 0.0, 0.0, 4.0,-2.0, 0.0, 1.0, 2.0,-4.0, 0.0, 1.0, 2.0,
  -8.0, 0.0, 0.0, 4.0,-4.0, 0.0,-1.0, 2.0,-2.0, 0.0,-1.0, 2.0,
  -4.0, 0.0, 0.0, 4.0,-2.0, 2.0, 0.0, 2.0,-1.0, 1.0, 0.5, 1.0,
  -2.0, 2.0, 0.5, 1.0,-4.0, 4.0, 0.0, 2.0,-2.0, 2.0,-0.5, 1.0,
  -1.0, 1.0,-0.5, 1.0,-2.0, 2.0, 0.0, 2.0, 2.0, 2.0, 0.0, 2.0,
   1.0, 1.0, 0.5, 1.0, 2.0, 2.0, 0.5, 1.0, 4.0, 4.0, 0.0, 2.0,
   2.0, 2.0,-0.5, 1.0, 1.0, 1.0,-0.5, 1.0, 2.0, 2.0, 0.0, 2.0,
   4.0, 0.0, 0.0, 4.0, 2.0, 0.0, 1.0, 2.0, 4.0, 0.0, 1.0, 2.0,
   8.0, 0.0, 0.0, 4.0, 4.0, 0.0,-1.0, 2.0, 2.0, 0.0,-1.0, 2.0,
   4.0, 0.0, 0.0, 4.0,
/* *INDENT-ON* */

};

void
move(int x, int y)
{
  prevx = curx;
  prevy = cury;
  curx = x;
  cury = y;
  if (curx != startx || cury != starty) {
    glutPostRedisplay();
    startx = curx;
    starty = cury;
  }
}

void
button(int button, int state, int x, int y)
{
  if (button != GLUT_LEFT_BUTTON)
    return;
  switch (state) {
  case GLUT_DOWN:
    prevx = curx = startx = x;
    prevy = cury = starty = y;
    spindx = 0;
    spindy = 0;
    tracking = GL_TRUE;
    break;
  case GLUT_UP:
    /* 
     * If user released the button while moving the mouse, keep
     * spinning.
     */
    if (x != prevx || y != prevy) {
      spindx = x - prevx;
      spindy = y - prevy;
    }
    tracking = GL_FALSE;
    break;
  }
}

/* Maintain a square window when resizing */
void
reshape(int width, int height)
{
  int size;
  size = (width < height ? width : height);
  glViewport((width - size) / 2, (height - size) / 2, size, size);
  glutReshapeWindow(size, size);
  glutPostRedisplay();
}

void
gridmaterials(void)
{
  static float front_mat_diffuse[] =
  {1.0, 1.0, 0.4, 1.0};
  static float front_mat_ambient[] =
  {0.1, 0.1, 0.1, 1.0};
  static float back_mat_diffuse[] =
  {1.0, 0.0, 0.0, 1.0};
  static float back_mat_ambient[] =
  {0.1, 0.1, 0.1, 1.0};

  glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, front_mat_ambient);
  glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);
  glMaterialfv(GL_BACK, GL_AMBIENT, back_mat_ambient);
}

void
surfacematerials(void)
{
  static float front_mat_diffuse[] =
  {0.2, 0.7, 0.4, 1.0};
  static float front_mat_ambient[] =
  {0.1, 0.1, 0.1, 1.0};
  static float back_mat_diffuse[] =
  {1.0, 1.0, 0.2, 1.0};
  static float back_mat_ambient[] =
  {0.1, 0.1, 0.1, 1.0};

  glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, front_mat_ambient);
  glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);
  glMaterialfv(GL_BACK, GL_AMBIENT, back_mat_ambient);
}

void
init(void)
{
  static float ambient[] =
  {0.0, 0.0, 0.0, 1.0};
  static float diffuse[] =
  {1.0, 1.0, 1.0, 1.0};
  static float position[] =
  {90.0, 90.0, -150.0, 0.0};
  static float lmodel_ambient[] =
  {1.0, 1.0, 1.0, 1.0};

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, 1.0, 2.0, 200.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);

  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, position);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_AUTO_NORMAL);
  glFrontFace(GL_CCW);

  glEnable(GL_MAP2_VERTEX_4);
  glClearColor(0.25, 0.25, 0.5, 0.0);

#if GL_EXT_polygon_offset
  glPolygonOffsetEXT(factor, bias);
  glEnable(GL_POLYGON_OFFSET_EXT);
#endif

  nobj = gluNewNurbsRenderer();
#ifdef GLU_VERSION_1_1  /* New GLU 1.1 interface. */
  gluNurbsProperty(nobj, GLU_SAMPLING_METHOD, GLU_DOMAIN_DISTANCE);
#endif

  surflist = glGenLists(1);
  gridlist = glGenLists(1);
  createlists();
}

void
drawmesh(void)
{
  int i, j;
  float *p;

  int up2p = 4;
  int uorder = 3, vorder = 3;
  int nu = 4, nv = 4;
  int vp2p = up2p * uorder * nu;

  for (j = 0; j < nv; j++) {
    for (i = 0; i < nu; i++) {
      p = torusbezierpts + (j * vp2p * vorder) + (i * up2p * uorder);
#if GL_EXT_polygon_offset
      glPolygonOffsetEXT(factor, bias);
#endif
      glMap2f(GL_MAP2_VERTEX_4, 0.0, 1.0, up2p, 3, 0.0, 1.0, vp2p, 3,
        (void *) p);
      if (showsurf) {
        surfacematerials();
        glEvalMesh2(GL_FILL, 0, usegments, 0, vsegments);
      }
      if (showgrid) {
        gridmaterials();
        glEvalMesh2(GL_LINE, 0, usegments, 0, vsegments);
      }
    }
  }
}

void
redraw(void)
{
  int dx, dy;
  float v[3], rot[3];
  float len, ang;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3f(1, 0, 0);

  if (tracking) {
    dx = curx - startx;
    dy = cury - starty;
  } else {
    dx = spindx;
    dy = spindy;
  }
  if (dx || dy) {
    dy = -dy;
    v[0] = dx;
    v[1] = dy;
    v[2] = 0;

    len = length(v);
    ang = -len / 600 * 360;
    norm(v);
    cross(v, z_axis, rot);

    /* This is certainly not recommended for programs that care
       about performance or numerical stability: we concatenate
       the rotation onto the current modelview matrix and read
       the matrix back, thus saving ourselves from writing our
       own matrix manipulation routines.  */
    glLoadIdentity();
    glRotatef(ang, rot[0], rot[1], rot[2]);
    glMultMatrixf(modelmatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
  }
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -10.0);
  glMultMatrixf(modelmatrix);

  if (useglunurbs) {
    if (showsurf)
      glCallList(surflist);
    if (showgrid)
      glCallList(gridlist);
  } else {
    glMapGrid2f(usegments, 0.0, 1.0, vsegments, 0.0, 1.0);
    drawmesh();
  }

  glutSwapBuffers();
}

static void
usage(void)
{
  printf("usage: surfgrid [-f]\n");
  exit(-1);
}

/* what to do when a menu item is selected. This function also handles
   keystroke events.  */
void
menu(int item)
{
  switch (item) {
  case 'p':
#if GL_EXT_polygon_offset
    if (glIsEnabled(GL_POLYGON_OFFSET_EXT)) {
      glDisable(GL_POLYGON_OFFSET_EXT);
      printf("disabling polygon offset\n");
    } else {
      glEnable(GL_POLYGON_OFFSET_EXT);
      printf("enabling polygon offset\n");
    }
#endif
    break;
  case 'F':
    factor += 0.1;
    printf("factor: %8.4f\n", factor);
    break;
  case 'f':
    factor -= 0.1;
    printf("factor: %8.4f\n", factor);
    break;
  case 'B':
    bias += 0.0001;
    printf("bias:  %8.4f\n", bias);
    break;
  case 'b':
    bias -= 0.0001;
    printf("bias:  %8.4f\n", bias);
    break;
  case 'g':
    showgrid = !showgrid;
    break;
  case 'n':
    useglunurbs = !useglunurbs;
    break;
  case 's':
    smooth = !smooth;
    if (smooth) {
      glShadeModel(GL_SMOOTH);
    } else {
      glShadeModel(GL_FLAT);
    }
    break;
  case 't':
    showsurf = !showsurf;
    break;
  case 'u':
    usegments = (usegments < 2 ? 1 : usegments - 1);
    createlists();
    break;
  case 'U':
    usegments++;
    createlists();
    break;
  case 'v':
    vsegments = (vsegments < 2 ? 1 : vsegments - 1);
    createlists();
    break;
  case 'V':
    vsegments++;
    createlists();
    break;
  case '\033':         /* ESC key: quit */
    exit(0);
    break;
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  menu((int) key);
}

void
animate(void)
{
  if (!tracking && (spindx != 0 || spindy != 0))
    glutPostRedisplay();
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);  /* initialize glut, processing
                             arguments */

  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'f':
        fullscreen = 1;
        break;
      default:
        usage();
        break;
      }
    } else {
      usage();
    }
  }

  glutInitWindowSize(winwidth, winheight);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("surfgrid");

  /* create a menu for the right mouse button */
  glutCreateMenu(menu);
#if GL_EXT_polygon_offset
  glutAddMenuEntry("p: toggle polygon offset", 'p');
#endif
  glutAddMenuEntry("F: increase factor", 'F');
  glutAddMenuEntry("f: decrease factor", 'f');
  glutAddMenuEntry("B: increase bias", 'B');
  glutAddMenuEntry("b: decrease bias", 'b');
  glutAddMenuEntry("g: toggle grid", 'g');
  glutAddMenuEntry("s: toggle smooth shading", 's');
  glutAddMenuEntry("t: toggle surface", 't');
  glutAddMenuEntry("n: toggle GL evalutators/GLU nurbs", 'n');
  glutAddMenuEntry("u: decrement u segments", 'u');
  glutAddMenuEntry("U: increment u segments", 'U');
  glutAddMenuEntry("v: decrement v segments", 'v');
  glutAddMenuEntry("V: increment v segments", 'V');
  glutAddMenuEntry("<esc>: exit program", '\033');
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* set callbacks */
  glutKeyboardFunc(key);
  glutDisplayFunc(redraw);
  glutReshapeFunc(reshape);
  glutMouseFunc(button);
  glutMotionFunc(move);
  glutIdleFunc(animate);

#if GL_EXT_polygon_offset
  if (!glutExtensionSupported("GL_EXT_polygon_offset")) {
    printf("Warning: "
      "GL_EXT_polygon_offset not supported on this machine... "
      "trying anyway\n");
  }
#else
  printf("Warning: not compiled with GL_EXT_polygon_offset support.\n");
#endif

  init();
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

float circleknots[] =
{0.0, 0.0, 0.0, 0.25, 0.50, 0.50, 0.75, 1.0, 1.0, 1.0};

void
createlists(void)
{
#ifdef GLU_VERSION_1_1  /* New GLU 1.1 interface. */
  gluNurbsProperty(nobj, GLU_U_STEP, (usegments - 1) * 4);
  gluNurbsProperty(nobj, GLU_V_STEP, (vsegments - 1) * 4);

  gluNurbsProperty(nobj, GLU_DISPLAY_MODE, GLU_FILL);
#endif
  glNewList(surflist, GL_COMPILE);
  surfacematerials();
  gluBeginSurface(nobj);
  gluNurbsSurface(nobj, 10, circleknots, 10, circleknots,
    4, 28, torusnurbpts, 3, 3, GL_MAP2_VERTEX_4);
  gluEndSurface(nobj);
  glEndList();

  gluNurbsProperty(nobj, GLU_DISPLAY_MODE, GLU_OUTLINE_POLYGON);
  glNewList(gridlist, GL_COMPILE);
  gridmaterials();
  gluBeginSurface(nobj);
  gluNurbsSurface(nobj, 10, circleknots, 10, circleknots,
    4, 28, torusnurbpts, 3, 3, GL_MAP2_VERTEX_4);
  gluEndSurface(nobj);
  glEndList();
}
