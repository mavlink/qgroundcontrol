
/* shadowmap.c - by Tom McReynolds, SGI */

/* Shadows: Shadow volumes. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

/* This program demonstrates shadows on IR using single pass projective
   texture method.  1. Render the scene with light position as the viewpoint
   and save the depth-map into texture image.  2. Use texgen to generate
   texture co-ordinates which are identical to vertex co-ordinates. The
   texture matrix then transforms each pixel coods back to light co-ods. The
   'z' or the depth-value is now available in the 'r' texture co-ordinate.

   3. Render the normal scene enabling texgen and shadow texture comparison. 
   Left mouse button: controls rotation of the scene

   Right mouse button: controls light (and shadow position) */

#define SCENE 10
enum {
  M_NORMAL, M_SHADOW, M_PROJTEX, M_LIGHT
};

GLfloat rotv[] =
{0.0, 0.0, 0.0};        /* rotation vector for scene */
GLfloat rotl[] =
{0.0, 0.0, 0.0};        /* rotation vector for light */
GLfloat lv[] =
{10.0, 10.0, 10.0, 1.0};  /* default light position */

GLfloat perspective_mat[16], modelview_mat[16], temp[16];
int width = 512, height = 512;
int mouse_button, mouse_state;
static int do_light = 0;
static int do_proj = 0; /* Use projective textures instead of shadows */
/* are shadow extensions supported? */
GLboolean shadows_supported = GL_FALSE;
GLboolean ambient_shadows = GL_FALSE;
GLboolean depth_texture = GL_FALSE;

static void generate_shadow_map(void);

static void 
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  width = w;
  height = h;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, (GLfloat) w / (GLfloat) h, 2.0, 30.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 0, 15,
    0, 0, 0,
    0, 1, 0);
}

/* ARGSUSED1 */
static void 
key(unsigned char key, int x, int y)
{
  switch (key) {
  case '\033':
    exit(0);
  }
}

/* ARGSUSED2 */
static void 
mouse(int button, int state, int x, int y)
{
  mouse_button = button;
  mouse_state = state;
}

static void 
motion(int x, int y)
{
  if (mouse_state == GLUT_UP)
    return;

  switch (mouse_button) {
  case GLUT_LEFT_BUTTON:
    rotv[1] = 180.0 * x / 400.0 - 90.0;
    rotv[0] = 180.0 * y / 400.0 - 90.0;
    break;
  case GLUT_MIDDLE_BUTTON:
    rotl[0] = 180.0 * x / 400.0 - 90.0;
    rotl[1] = 180.0 * y / 400.0 - 90.0;
    break;
  }
  glutPostRedisplay();
}

static void 
display(void)
{
  /* Render the scene with the light source as the viewpoint and save the
     depth values in the texture map.  */
  generate_shadow_map();

  /* Now render the normal scene using projective textures to get the depth
     value from the light's point of view into the r-cood of the texture.  */
  glEnable(GL_TEXTURE_2D);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glGetFloatv(GL_PROJECTION_MATRIX, perspective_mat);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glTranslatef(0.5, 0.5, 0.4994);
  glScalef(0.5, 0.5, 0.5);
  glMultMatrixf(perspective_mat);
  glMultMatrixf(modelview_mat);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glRotatef(rotv[0], 1, 0, 0);
  glRotatef(rotv[1], 0, 1, 0);
  glRotatef(rotv[2], 0, 0, 1);
  glCallList(SCENE);

  glPopMatrix();
  glutSwapBuffers();
}

static void 
render_normal_view(void)
{
  glDisable(GL_TEXTURE_2D);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  gluLookAt(0, 0, 15,
    0, 0, 0,
    0, 1, 0);
  glRotatef(rotv[0], 1, 0, 0);
  glRotatef(rotv[1], 0, 1, 0);
  glRotatef(rotv[2], 0, 0, 1);
  glCallList(SCENE);

  glPopMatrix();
  glutSwapBuffers();
}

static void 
render_light_view(void)
{
  glDisable(GL_TEXTURE_2D);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  gluLookAt(lv[0], lv[1], lv[2],
    0, 0, 0,
    0, 1, 0);
  glRotatef(rotl[0], 1, 0, 0);
  glRotatef(rotl[1], 0, 1, 0);
  glRotatef(rotl[2], 0, 0, 1);
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview_mat);

  glCallList(SCENE);
  glPopMatrix();
  if (do_light)
    glutSwapBuffers();
}

static void 
generate_shadow_map(void)
{
  int x, y;
  GLfloat log2 = log(2.0);

  x = 1 << ((int) (log((float) width) / log2));
  y = 1 << ((int) (log((float) height) / log2));
  glViewport(0, 0, x, y);
  render_light_view();

  /* Read in frame-buffer into a depth texture map */
#if defined(GL_EXT_subtexture) && defined(GL_EXT_copy_texture)
#ifdef GL_SGIX_depth_texture
  if (do_proj && depth_texture)
#endif
    glCopyTexImage2DEXT(GL_TEXTURE_2D, 0, GL_RGBA,
      0, 0, x, y, 0);
#ifdef GL_SGIX_depth_texture
  else
    glCopyTexImage2DEXT(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16_SGIX,
      0, 0, x, y, 0);
#endif
#endif
  glViewport(0, 0, width, height);
}

static void 
menu(int mode)
{
  switch (mode) {
  case M_NORMAL:
    do_light = 0;
    do_proj = 0;
#ifdef GL_SGIX_shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_SGIX, GL_FALSE);
#endif
    glutDisplayFunc(render_normal_view);
    break;
  case M_SHADOW:
#ifdef GL_SGIX_shadow
    do_light = 0;
    do_proj = 0;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_SGIX, GL_TRUE);
    glutDisplayFunc(display);
#endif
    break;
  case M_PROJTEX:
    do_light = 0;
    do_proj = 1;
#ifdef GL_SGIX_shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_SGIX, GL_FALSE);
#endif
    glutDisplayFunc(display);
    break;
  case M_LIGHT:
    do_light = 1;
    if (do_light)
      glutDisplayFunc(render_light_view);
    else
      glutDisplayFunc(display);
    break;
  }
  glutPostRedisplay();
}

#define XFORM(cmds) \
  glMatrixMode(GL_TEXTURE); \
  cmds; \
  glMatrixMode(GL_MODELVIEW); \
  cmds

static void 
create_scene(void)
{
  GLfloat floor_col[] =
  {0.7, 0.7, 0.7};
  GLfloat floor_norm[] =
  {0.0, 0.0, 1.0};
  GLfloat floor_verts[4][3] =
  {
    {4.0, 4.0, 0.0},
    {-4.0, 4.0, 0.0},
    {-4.0, -4.0, 0.0},
    {4.0, -4.0, 0.0}};
  GLfloat sphere_col[] =
  {0.7, 0.1, 0.2};
  GLfloat box_col[] =
  {0.1, 0.2, 0.7};
  GLfloat box_verts[6][4][3] =
  {
    {
      {1.0, -1.0, -1.0},
      {-1.0, -1.0, -1.0},
      {-1.0, 1.0, -1.0},
      {1.0, 1.0, -1.0}},
    {
      {1.0, -1.0, 1.0},
      {1.0, -1.0, -1.0},
      {1.0, 1.0, -1.0},
      {1.0, 1.0, 1.0}},
    {
      {-1.0, -1.0, 1.0},
      {1.0, -1.0, 1.0},
      {1.0, 1.0, 1.0},
      {-1.0, 1.0, 1.0}},
    {
      {-1.0, -1.0, -1.0},
      {-1.0, -1.0, 1.0},
      {-1.0, 1.0, 1.0},
      {-1.0, 1.0, -1.0}},
    {
      {1.0, 1.0, 1.0},
      {1.0, 1.0, -1.0},
      {-1.0, 1.0, -1.0},
      {-1.0, 1.0, 1.0}},
    {
      {1.0, -1.0, -1.0},
      {1.0, -1.0, 1.0},
      {-1.0, -1.0, 1.0},
      {-1.0, -1.0, -1.0}}};
  GLfloat box_norm[6][3] =
  {
    {0, 0, -1},
    {1, 0, 0},
    {0, 0, 1},
    {-1, 0, 0},
    {0, 1, 0},
    {0, -1, 0}};
  GLUquadricObj *q;
  int i;

  glNewList(SCENE, GL_COMPILE);

  glBegin(GL_QUADS);    /* draw the floor */
  glNormal3fv(floor_norm);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, floor_col);
  for (i = 0; i < 4; i++)
    glVertex3fv(floor_verts[i]);
  glEnd();

  q = gluNewQuadric();
  XFORM(glPushMatrix();
    glTranslatef(1.0, 1.0, 1.01));
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_col);
  gluSphere(q, 1.0, 40, 40);
  XFORM(glPopMatrix());

  XFORM(glPushMatrix();
    glTranslatef(-1.0, -1.0, 1.01));
  for (i = 0; i < 6; i++) {
    glBegin(GL_QUADS);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, box_col);
    glNormal3fv(box_norm[i]);
    glVertex3fv(box_verts[i][0]);
    glVertex3fv(box_verts[i][1]);
    glVertex3fv(box_verts[i][2]);
    glVertex3fv(box_verts[i][3]);
    glEnd();
  }
  XFORM(glPopMatrix());
  glEndList();
}

static void 
init(void)
{
  GLfloat ambient[] =
  {0.1, 0.1, 0.1, 1.0};
  GLfloat diffuse[] =
  {0.8, 0.7, 0.8, 1.0};
  GLfloat specular[] =
  {0.5, 0.6, 0.8, 1.0};
  GLfloat p[4];

  create_scene();
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClearDepth(1.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_SMOOTH);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, lv);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  if (shadows_supported) {
#ifdef GL_SGIX_shadow_ambient
    if (ambient_shadows)
      glTexParameterf(GL_TEXTURE_2D, GL_SHADOW_AMBIENT_SGIX, 0.6);
#endif
#ifdef GL_SGIX_shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_SGIX, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_OPERATOR_SGIX,
      GL_TEXTURE_LEQUAL_R_SGIX);
#endif
  }
  /* Enable texgen to get texture-coods (x, y, z, w) at every point.These
     texture co-ordinates are then transformed by the texture matrix.  */
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

  p[0] = 1.0;
  p[1] = p[2] = p[3] = 0.0;
  glTexGenfv(GL_S, GL_OBJECT_PLANE, p);

  p[0] = 0.0;
  p[1] = 1.0;
  p[2] = p[3] = 0.0;
  glTexGenfv(GL_T, GL_OBJECT_PLANE, p);

  p[0] = p[1] = 0.0;
  p[2] = 1.0, p[3] = 0.0;
  glTexGenfv(GL_R, GL_OBJECT_PLANE, p);

  p[0] = p[1] = p[2] = 0.0;
  p[3] = 1.0;
  glTexGenfv(GL_Q, GL_OBJECT_PLANE, p);

  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_GEN_R);
  glEnable(GL_TEXTURE_GEN_Q);
}

int
main(int argc, char *argv[])
{
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(width, height);
  glutCreateWindow("Shadow Map");

  if (glutExtensionSupported("GL_SGIX_shadow") &&
    glutExtensionSupported("GL_EXT_subtexture") &&
    glutExtensionSupported("GL_EXT_copy_texture")) {
    shadows_supported = GL_TRUE;
    ambient_shadows = glutExtensionSupported("GL_SGIX_shadow_ambient");
    depth_texture = glutExtensionSupported("GL_SGIX_depth_texture");
  } else {
    fprintf(stderr, "shadowmap: uses several OpenGL extensions to operate fully:\n");
    fprintf(stderr, "  GL_SGIX_shadow\n");
    fprintf(stderr, "  GL_SGIX_shadow_ambient\n");
    fprintf(stderr, "  GL_SGIS_depth_texture\n");
    fprintf(stderr, "  GL_EXT_subtexture\n");
    fprintf(stderr, "  GL_EXT_copy_texture\n");
  }

  init();
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMotionFunc(motion);
  glutMouseFunc(mouse);
  glutKeyboardFunc(key);
  glutCreateMenu(menu);
  glutAddMenuEntry("Normal view", M_NORMAL);
  glutAddMenuEntry("Light view", M_LIGHT);
  glutAddMenuEntry("Projective textures", M_PROJTEX);
  if (shadows_supported)
    glutAddMenuEntry("Shadows", M_SHADOW);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
