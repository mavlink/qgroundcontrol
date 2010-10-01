
/* envphong.c - David G. Yu, SGI */

/**
 ** Demonstrates a use of environment texture mapping for improved highlight
 ** shading.
 **
 ** Press mouse button one to move the object, press mouse button two to
 ** move the light source.  Pressing the <s> key switches between using
 ** regular lighting or texture mapping for the specular highlights. The
 ** <o> key will cycle through objects (sphere, cylinder, torus).  Check
 ** out the event loop code below for other keys which do will things.
 **
 ** TBD
 **  - improve accuracy of the highlight texture map
 **  - reduce or eliminate grazing angle artifacts
 **  - improve user interaction
 **
 ** 1995 -- David G Yu
 **
 ** cc -o envphong envphong.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int win, win2;
int needsLightUpdate = GL_TRUE;
int useLighting = GL_TRUE;
int useSpecularTexture = GL_FALSE;
int useTexture = GL_FALSE;
int useHighRes = GL_FALSE;
int usePattern = GL_FALSE;
int moveLight = GL_FALSE;
int moveObject = GL_FALSE;
int drawObj = 0, maxObj = 2;
GLfloat lightRotX, lightRotY;
GLfloat objectRotX, objectRotY;
int curx, cury, width, height;

void
drawSphere(int numMajor, int numMinor, float radius)
{
  double majorStep = (M_PI / numMajor);
  double minorStep = (2.0 * M_PI / numMinor);
  int i, j;

  for (i = 0; i < numMajor; ++i) {
    double a = i * majorStep;
    double b = a + majorStep;
    double r0 = radius * sin(a);
    double r1 = radius * sin(b);
    GLfloat z0 = radius * cos(a);
    GLfloat z1 = radius * cos(b);

    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j <= numMinor; ++j) {
      double c = j * minorStep;
      GLfloat x = cos(c);
      GLfloat y = sin(c);

      glNormal3f((x * r0) / radius, (y * r0) / radius, z0 / radius);
      glTexCoord2f(j / (GLfloat) numMinor, i / (GLfloat) numMajor);
      glVertex3f(x * r0, y * r0, z0);

      glNormal3f((x * r1) / radius, (y * r1) / radius, z1 / radius);
      glTexCoord2f(j / (GLfloat) numMinor, (i + 1) / (GLfloat) numMajor);
      glVertex3f(x * r1, y * r1, z1);
    }
    glEnd();
  }
}

void
drawCylinder(int numMajor, int numMinor, float height, float radius)
{
  double majorStep = height / numMajor;
  double minorStep = 2.0 * M_PI / numMinor;
  int i, j;

  for (i = 0; i < numMajor; ++i) {
    GLfloat z0 = 0.5 * height - i * majorStep;
    GLfloat z1 = z0 - majorStep;

    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j <= numMinor; ++j) {
      double a = j * minorStep;
      GLfloat x = radius * cos(a);
      GLfloat y = radius * sin(a);

      glNormal3f(x / radius, y / radius, 0.0);
      glTexCoord2f(j / (GLfloat) numMinor, i / (GLfloat) numMajor);
      glVertex3f(x, y, z0);

      glNormal3f(x / radius, y / radius, 0.0);
      glTexCoord2f(j / (GLfloat) numMinor, (i + 1) / (GLfloat) numMajor);
      glVertex3f(x, y, z1);
    }
    glEnd();
  }
}

void
drawTorus(int numMajor, int numMinor, float majorRadius, float minorRadius)
{
  double majorStep = 2.0 * M_PI / numMajor;
  double minorStep = 2.0 * M_PI / numMinor;
  int i, j;

  for (i = 0; i < numMajor; ++i) {
    double a0 = i * majorStep;
    double a1 = a0 + majorStep;
    GLfloat x0 = cos(a0);
    GLfloat y0 = sin(a0);
    GLfloat x1 = cos(a1);
    GLfloat y1 = sin(a1);

    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j <= numMinor; ++j) {
      double b = j * minorStep;
      GLfloat c = cos(b);
      GLfloat r = minorRadius * c + majorRadius;
      GLfloat z = minorRadius * sin(b);

      glNormal3f(x0 * c, y0 * c, z / minorRadius);
      glTexCoord2f(i / (GLfloat) numMajor, j / (GLfloat) numMinor);
      glVertex3f(x0 * r, y0 * r, z);

      glNormal3f(x1 * c, y1 * c, z / minorRadius);
      glTexCoord2f((i + 1) / (GLfloat) numMajor, j / (GLfloat) numMinor);
      glVertex3f(x1 * r, y1 * r, z);
    }
    glEnd();
  }
}

void
setNullTexture(void)
{
  GLubyte texPixel[4] =
  {0xff, 0xff, 0xff, 0xff};
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, 1, 1, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, texPixel);
}

void
setCheckTexture(void)
{
  int texWidth = 64;
  int texHeight = 64;
  GLubyte *texPixels, *p;
  int i, j;

  texPixels = (GLubyte *) malloc(texWidth * texHeight * 4 * sizeof(GLubyte));
  if (texPixels == NULL) {
    return;
  }
  p = texPixels;
  for (i = 0; i < texHeight; ++i) {
    for (j = 0; j < texWidth; ++j) {
      if ((i ^ j) & 8) {
        p[0] = 0xff;
        p[1] = 0xff;
        p[2] = 0xff;
        p[3] = 0xff;
      } else {
        p[0] = 0x08;
        p[1] = 0x08;
        p[2] = 0x08;
        p[3] = 0xff;
      }
      p += 4;
    }
  }

  glTexParameteri(GL_TEXTURE_2D,
    GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,
    GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  gluBuild2DMipmaps(GL_TEXTURE_2D, 4, texWidth, texHeight,
    GL_RGBA, GL_UNSIGNED_BYTE, texPixels);
  free(texPixels);
}

void
setLight(void)
{
  GLfloat light0Pos[4] =
  {0.70, 0.70, 1.25, 0.00};
  GLfloat light0Amb[4] =
  {0.00, 0.00, 0.00, 1.00};
  GLfloat light0Diff[4] =
  {1.00, 1.00, 1.00, 1.00};
  GLfloat light0Spec[4] =
  {1.00, 1.00, 1.00, 1.00};
  GLfloat light0SpotDir[3] =
  {0.00, 0.00, -1.00};
  GLfloat light0SpotExp = 0.00;
  GLfloat light0SpotCutoff = 180.00;
  GLfloat light0Atten0 = 1.00;
  GLfloat light0Atten1 = 0.00;
  GLfloat light0Atten2 = 0.00;

  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light0Amb);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0Diff);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light0Spec);
  glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light0SpotDir);
  glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, light0SpotExp);
  glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, light0SpotCutoff);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, light0Atten0);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, light0Atten1);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, light0Atten2);
}

#define MAT_ALL				0
#define MAT_NO_SPECULAR			1
#define MAT_SPECULAR_ONLY		2
#define MAT_SPECULAR_TEXTURE_ONLY	3
#define MAT_GEN_SPECULAR_TEXTURE	4

void
setMaterial(int mode)
{
  static GLubyte *texPixels;
  int texWidth = 128;
  int texHeight = 128;

  GLfloat matZero[4] =
  {0.00, 0.00, 0.00, 1.00};
  GLfloat matOne[4] =
  {1.00, 1.00, 1.00, 1.00};
  GLfloat matEm[4] =
  {0.00, 0.00, 0.00, 1.00};
  GLfloat matAmb[4] =
  {0.01, 0.01, 0.01, 1.00};
  GLfloat matDiff[4] =
  {0.02, 0.20, 0.16, 1.00};
  GLfloat matSpec[4] =
  {0.50, 0.50, 0.50, 1.00};
  GLfloat matShine = 20.00;

  if ((mode == MAT_ALL) || (mode == MAT_NO_SPECULAR)) {
    glMaterialfv(GL_FRONT, GL_EMISSION, matEm);
    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiff);
  } else {
    glMaterialfv(GL_FRONT, GL_EMISSION, matZero);
    glMaterialfv(GL_FRONT, GL_AMBIENT, matZero);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matZero);
  }

  if ((mode == MAT_ALL) || (mode == MAT_SPECULAR_ONLY)) {
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT, GL_SHININESS, matShine);
  } else {
    glMaterialfv(GL_FRONT, GL_SPECULAR, matZero);
    glMaterialf(GL_FRONT, GL_SHININESS, 1);
  }

  if (mode == MAT_SPECULAR_TEXTURE_ONLY) {
    if (texPixels == NULL) {
      return;
    }
    glMaterialfv(GL_FRONT, GL_SPECULAR, matOne);
    glMaterialf(GL_FRONT, GL_SHININESS, 0);

    glTexParameteri(GL_TEXTURE_2D,
      GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
      GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 4, texWidth, texHeight,
      GL_RGBA, GL_UNSIGNED_BYTE, texPixels);
  }
  if (mode == MAT_GEN_SPECULAR_TEXTURE) {
    if (texPixels == NULL) {
      texPixels = (GLubyte *)
        malloc(texWidth * texHeight * 4 * sizeof(GLubyte));
      if (texPixels == NULL) {
        return;
      }
    }
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setMaterial(MAT_SPECULAR_ONLY);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, 1, 3);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0, 0, -2);

    glPushMatrix();
    glRotatef(lightRotY, 0, 1, 0);
    glRotatef(lightRotX, 1, 0, 0);
    setLight();
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glViewport(0, 0, 128, 128);
    glClearColor(.25, .25, .25, .25);
    glClear(GL_COLOR_BUFFER_BIT);
    drawSphere(128, 128, 1.0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glReadPixels(0, 0, texWidth, texHeight,
      GL_RGBA, GL_UNSIGNED_BYTE, texPixels);

    glPopAttrib();
  }
}

void
initialize(void)
{
  glMatrixMode(GL_PROJECTION);
  glFrustum(-0.50, 0.50, -0.50, 0.50, 1.0, 3.0);
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(0.0, 0.0, -2.0);

  glDepthFunc(GL_LEQUAL);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
}

void
redraw(void)
{
  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBegin(GL_QUADS);
  glColor3f(0.9, 0.0, 1.0);
  glVertex3f(-2.0, -2.0, -1.0);
  glColor3f(0.9, 0.0, 1.0);
  glVertex3f(2.0, -2.0, -1.0);
  glColor3f(0.0, 0.1, 0.1);
  glVertex3f(2.0, 2.0, -1.0);
  glColor3f(0.0, 0.1, 0.1);
  glVertex3f(-2.0, 2.0, -1.0);
  glEnd();

  glEnable(GL_DEPTH_TEST);

  if (useLighting) {
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glRotatef(lightRotY, 0, 1, 0);
    glRotatef(lightRotX, 1, 0, 0);
    setLight();
    glPopMatrix();
  } else {
    glColor3f(0.2, 0.2, 0.2);
  }
  if (useTexture || useSpecularTexture) {
    glEnable(GL_TEXTURE_2D);
  }
  if (useTexture) {
    setCheckTexture();
  } else {
    setNullTexture();
  }
  if (useSpecularTexture) {
    /* pass one */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    setMaterial(MAT_NO_SPECULAR);
  } else {
    setMaterial(MAT_ALL);
  }
  glPushMatrix();
  glRotatef(objectRotY, 0, 1, 0);
  glRotatef(objectRotX, 1, 0, 0);
  if (useHighRes) {
    switch (drawObj) {
    case 0:
      drawSphere(64, 64, 0.8);
      break;
    case 1:
      drawCylinder(32, 64, 1.0, 0.4);
      break;
    case 2:
      drawTorus(64, 64, 0.6, 0.2);
      break;
    default:
      break;
    }
  } else {
    switch (drawObj) {
    case 0:
      drawSphere(16, 32, 0.8);
      break;
    case 1:
      drawCylinder(6, 16, 1.0, 0.4);
      break;
    case 2:
      drawTorus(32, 16, 0.6, 0.2);
      break;
    default:
      break;
    }
  }
  glPopMatrix();

  if (useSpecularTexture) {
    /* pass two */
    if (!useLighting) {
      glColor3f(1.0, 1.0, 1.0);
    }
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    setMaterial(MAT_SPECULAR_TEXTURE_ONLY);
    glPushMatrix();
    glRotatef(objectRotY, 0, 1, 0);
    glRotatef(objectRotX, 1, 0, 0);
    if (useHighRes) {
      switch (drawObj) {
      case 0:
        drawSphere(64, 64, 0.8);
        break;
      case 1:
        drawCylinder(32, 64, 1.0, 0.4);
        break;
      case 2:
        drawTorus(64, 64, 0.6, 0.2);
        break;
      default:
        break;
      }
    } else {
      switch (drawObj) {
      case 0:
        drawSphere(16, 32, 0.8);
        break;
      case 1:
        drawCylinder(6, 16, 1.0, 0.4);
        break;
      case 2:
        drawTorus(32, 16, 0.6, 0.2);
        break;
      default:
        break;
      }
    }
    glPopMatrix();
  }
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
}

void
checkErrors(void)
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "Error: %s\n", (char *) gluErrorString(error));
  }
}

void
usage(char *name)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "usage: %s [ options ]\n", name);
  fprintf(stderr, "\n");
  fprintf(stderr, "  Options:\n");
  fprintf(stderr, "    none\n");
  fprintf(stderr, "\n");
}

void 
help(void)
{
  printf("'h'      - help\n");
  printf("'l'      - toggle lighting\n");
  printf("'o'      - switch objects\n");
  printf("'r'      - toggle resolution\n");
  printf("'s'      - toggle two pass lighting\n");
  printf("'t'      - toggle texturing\n");
  printf("left mouse     - move object\n");
  printf("middle mouse   - move light\n");
}

void
motion(int x, int y)
{
  if (moveLight || moveObject) {
    GLfloat dx = (y - cury) * 360.0 / height;
    GLfloat dy = (x - curx) * 360.0 / width;
    if (moveLight) {
      lightRotX += dx;
      if (lightRotX > 360)
        lightRotX -= 360;
      if (lightRotX < 0)
        lightRotX += 360;
      lightRotY += dy;
      if (lightRotY > 360)
        lightRotY -= 360;
      if (lightRotY < 0)
        lightRotY += 360;
      needsLightUpdate = GL_TRUE;
    } else if (moveObject) {
      objectRotX += dx;
      if (objectRotX > 360)
        objectRotX -= 360;
      if (objectRotX < 0)
        objectRotX += 360;
      objectRotY += dy;
      if (objectRotY > 360)
        objectRotY -= 360;
      if (objectRotY < 0)
        objectRotY += 360;
    }
    curx = x;
    cury = y;
  }
  glutPostRedisplay();
}

void
mouse(int button, int state, int x, int y)
{
  if (state == GLUT_DOWN) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      moveObject = GL_TRUE;
      motion(curx = x, cury = y);
      break;
    case GLUT_MIDDLE_BUTTON:
      moveLight = GL_TRUE;
      motion(curx = x, cury = y);
      break;
    }
  } else if (state == GLUT_UP) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      moveObject = GL_FALSE;
      break;
    case GLUT_MIDDLE_BUTTON:
      moveLight = GL_FALSE;
      break;
    }
  }
}

void
display(void)
{
  if (useSpecularTexture && needsLightUpdate) {
    setMaterial(MAT_GEN_SPECULAR_TEXTURE);
    needsLightUpdate = GL_FALSE;
  }
  redraw();
  glFlush();
  glutSwapBuffers();
  checkErrors();
}

void
reshape(int w, int h)
{
  glViewport(0, 0, width = w, height = h);
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 'h':
    help();
    break;
  case ' ':
    break;
  case 'l':
    useLighting = !useLighting;
    break;
  case 's':
    useSpecularTexture = !useSpecularTexture;
    needsLightUpdate = GL_TRUE;
    break;
  case 't':
    useTexture = !useTexture;
    break;
  case 'r':
    useHighRes = !useHighRes;
    break;
  case 'o':
    ++drawObj;
    if (drawObj > maxObj)
      drawObj = 0;
    break;
  case '\033':
    exit(0);
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
domenu(int value)
{
  key((unsigned char) value, 0, 0);
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  glutInitWindowSize(width = 300, height = 300);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);

  for (i = 1; i < argc; ++i) {
    usage(argv[0]);
    exit(1);
  }
  win = glutCreateWindow("envphong");
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutCreateMenu(domenu);
  glutAddMenuEntry("Toggle lighting", 'l');
  glutAddMenuEntry("Toggle checker texture", 't');
  glutAddMenuEntry("Toggle two-pass textured specular", 's');
  glutAddMenuEntry("Toggle object resolution", 'r');
  glutAddMenuEntry("Switch object", 'o');
  glutAddMenuEntry("Print help", 'h');
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  initialize();
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
