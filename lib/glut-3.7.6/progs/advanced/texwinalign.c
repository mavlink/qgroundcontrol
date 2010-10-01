
/* Copyright (c) Mark J. Kilgard, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This program demonstrates how to use the texture matrix
   and texture coordinate generation (texgen) to generate
   window space texture coordinates for arbitrary 3D geometry.
   The basic technique is to generate texture coordinates
   directly matching the object coordinates and using the
   texture matrix to mimic the viewport, projection, and
   modelview transformations to convert the texture coordinates
   into window coordinates identically to how the actual
   object coordinates are transformed into window space.  It
   is important to have perspective correct texturing if you
   want perspective projections to look right. */

#include <stdlib.h>
#include <GL/glut.h>

GLfloat lightDiffuse[] = {1.0, 0.0, 0.0, 1.0};  /* Red diffuse light. */
GLfloat lightPosition[] = {1.0, 1.0, 1.0, 0.0};  /* Infinite light location. */

GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
  {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
  {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
  {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
GLfloat v[8][3];  /* Will be filled in with X,Y,Z vertexes. */

GLfloat angle = -20.0;
int animating = 1;

#define TEX_WIDTH 16
#define TEX_HEIGHT 16

/* Nice circle texture tiling pattern. */
static char *circles[] = {
  "....xxxx........",
  "..xxxxxxxx......",
  ".xxxxxxxxxx.....",
  ".xxx....xxx.....",
  "xxx......xxx....",
  "xxx......xxx....",
  "xxx......xxx....",
  "xxx......xxx....",
  ".xxx....xxx.....",
  ".xxxxxxxxxx.....",
  "..xxxxxxxx......",
  "....xxxx........",
  "................",
  "................",
  "................",
  "................",
};

/* Nice grid texture tiling pattern. */
static char *grid[] = {
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "..............xx",
  "xxxxxxxxxxxxxxxx",
  "xxxxxxxxxxxxxxxx",
};

static void
makeTexture(char *pattern[])
{
  GLubyte floorTexture[TEX_WIDTH][TEX_HEIGHT][3];
  GLubyte *loc;
  int s, t;

  /* Setup RGB image for the texture. */
  loc = (GLubyte*) floorTexture;
  for (t = 0; t < TEX_HEIGHT; t++) {
    for (s = 0; s < TEX_WIDTH; s++) {
      if (pattern[t][s] == 'x') {
	/* Nice green. */
        loc[0] = 0x6f;
        loc[1] = 0x8f;
        loc[2] = 0x1f;
      } else {
	/* Light gray. */
        loc[0] = 0xaa;
        loc[1] = 0xaa;
        loc[2] = 0xaa;
      }
      loc += 3;
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, TEX_WIDTH, TEX_HEIGHT, 0,
    GL_RGB, GL_UNSIGNED_BYTE, floorTexture);
}

void
drawBox(void)
{
  int i;

  for (i = 0; i < 6; i++) {
    glBegin(GL_QUADS);
    glNormal3fv(&n[i][0]);
    glVertex3fv(&v[faces[i][0]][0]);
    glVertex3fv(&v[faces[i][1]][0]);
    glVertex3fv(&v[faces[i][2]][0]);
    glVertex3fv(&v[faces[i][3]][0]);
    glEnd();
  }
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
    glRotatef(angle, 0.0, 0.0, 1.0);
    drawBox();
  glPopMatrix();
  glutSwapBuffers();
}

int windowWidth;
int windowHeight;
int slideX = 0, slideY = 0;

void
configTextureMatrix(void)
{
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  /* Shift texture in pixel units (slideX,slideY).  You could use this
     to copensate for a viewport origin different from the window
     origin. */
  glTranslatef(slideX/(GLfloat)TEX_WIDTH,
               slideY/(GLfloat)TEX_HEIGHT,
	       0.0);
  /* Scale based on the window size in pixel. */
  glScalef(windowWidth/(GLfloat)TEX_WIDTH,
           windowHeight/(GLfloat)TEX_HEIGHT,
	   1.0);
  /* Mimic the scene's projection matrix setup. */
  gluPerspective( /* field of view in degree */ 40.0,
    /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 10.0);
  /* Mimic the scene's view matrix setup. */
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */
  /* Mimic the scene's model matrix setup. */
  /* Adjust cube position to be aesthetic angle. */
  glTranslatef(0.0, 0.0, -1.0);
  glRotatef(60, 1.0, 0.0, 0.0);
  glRotatef(angle, 0.0, 0.0, 1.0);
  /* Switch back to the modelview matrix. */
  glMatrixMode(GL_MODELVIEW);

}

void
idle(void)
{
  /* Slowly rotate object. */
  angle += 0.5;
  if (angle > 360.0) {
    angle -= 360.0;
  }
  /* Make sure the texture matrix mimics the changing
     modelview matrix. */
  configTextureMatrix();
  glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
  switch(c) {
  case 27: /* Escape */
    exit(0);
    break;
  case 'h':
    slideX--;
    break;
  case 'j':
    slideY--;
    break;
  case 'k':
    slideY++;
    break;
  case 'l':
    slideX++;
    break;
  case 'r':
    angle += 10;
    break;
  case 'a':
    animating = !animating;
    if (animating) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  }
  configTextureMatrix();
  glutPostRedisplay();
}

void
reshape(int width, int height)
{
  windowWidth = width;
  windowHeight = height;
  glViewport(0, 0, width, height);
  configTextureMatrix();
}

void
init(void)
{
  static GLfloat sPlane[4] = { 1.0, 0.0, 0.0, 0.0 };
  static GLfloat tPlane[4] = { 0.0, 1.0, 0.0, 0.0 };
  static GLfloat rPlane[4] = { 0.0, 0.0, 1.0, 0.0 };
  static GLfloat qPlane[4] = { 0.0, 0.0, 0.0, 1.0 };

  /* Setup cube vertex data. */
  v[0][0] = v[1][0] = v[2][0] = v[3][0] = -1;
  v[4][0] = v[5][0] = v[6][0] = v[7][0] = 1;
  v[0][1] = v[1][1] = v[4][1] = v[5][1] = -1;
  v[2][1] = v[3][1] = v[6][1] = v[7][1] = 1;
  v[0][2] = v[3][2] = v[4][2] = v[7][2] = 1;
  v[1][2] = v[2][2] = v[5][2] = v[6][2] = -1;

  /* Use depth buffering for hidden surface elimination. */
  glEnable(GL_DEPTH_TEST);

  /* Enable a single OpenGL light. */
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
  glEnable(GL_LIGHT0);

  /* Setup the view of the cube. */
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
    /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */

  /* Texgen that maps object coordinates directly to texture
     coordinates. */
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGenfv(GL_S, GL_OBJECT_PLANE, sPlane);
  glTexGenfv(GL_T, GL_OBJECT_PLANE, tPlane);
  glTexGenfv(GL_R, GL_OBJECT_PLANE, rPlane);
  glTexGenfv(GL_Q, GL_OBJECT_PLANE, qPlane);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_GEN_R);
  glEnable(GL_TEXTURE_GEN_Q);

  /* Enable texturing.  Perspective correct texturing is
     important to this demo! */
  glEnable(GL_TEXTURE_2D);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  /* Adjust cube position to be aesthetic orientation. */
  glTranslatef(0.0, 0.0, -1.0);
  glRotatef(60, 1.0, 0.0, 0.0);
}

void
menu(int selection)
{
  switch (selection) {
  case 1:
    glEnable(GL_LIGHTING);
    glutPostRedisplay();
    break;
  case 2:
    glDisable(GL_LIGHTING);
    glutPostRedisplay();
    break;
  case 3:
    keyboard('a', 0, 0);
    break;
  case 4:
    makeTexture(circles);
    break;
  case 5:
    makeTexture(grid);
    break;
  case 666:
    exit(0);
  }
}

void
visibility(int state)
{
  if (state == GLUT_VISIBLE) {
    if (animating) {
      glutIdleFunc(idle);
    }
  } else {
    glutIdleFunc(NULL);
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("window space aligned textures");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutVisibilityFunc(visibility);
  init();
  makeTexture(grid);
  glutCreateMenu(menu);
  glutAddMenuEntry("Enable lighting", 1);
  glutAddMenuEntry("Disable lighting", 2);
  glutAddMenuEntry("Animating", 3);
  glutAddMenuEntry("Circles", 4);
  glutAddMenuEntry("Grid", 5);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
