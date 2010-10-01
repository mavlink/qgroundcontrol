
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

#define MAX_SPHERES 50

typedef struct {
  GLfloat x, y, z;
  int detail;
  int material;
} SphereInfo;
/* *INDENT-OFF* */

GLfloat lightPos[4] = {2.0, 4.0, 2.0, 1.0};
GLfloat lightDir[4] = {-2.0, -4.0, -2.0, 1.0};
GLfloat lightAmb[4] = {0.2, 0.2, 0.2, 1.0};
GLfloat lightDiff[4] = {0.8, 0.8, 0.8, 1.0};
GLfloat lightSpec[4] = {0.4, 0.4, 0.4, 1.0};
GLfloat matColor[3][4] = {
  {0.5, 0.0, 0.0, 1.0},
  {0.0, 0.5, 0.0, 1.0},
  {0.0, 0.0, 0.5, 1.0},
};
/* *INDENT-ON* */

GLdouble modelMatrix[16], projMatrix[16];
GLint viewport[4];
int width, height;
int opaque, transparent;
SphereInfo sphereInfo[MAX_SPHERES];
int spheres = 0;
SphereInfo overlaySphere, oldOverlaySphere;

void
drawSphere(SphereInfo * sphere)
{
  glPushMatrix();
  glTranslatef(sphere->x, sphere->y, sphere->z);
  glMaterialfv(GL_FRONT_AND_BACK,
    GL_AMBIENT_AND_DIFFUSE, matColor[sphere->material]);
  glutSolidSphere(1.0, sphere->detail, sphere->detail);
  glPopMatrix();
}

void
display(void)
{
  int i;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  for (i = 0; i < spheres; i++) {
    drawSphere(&sphereInfo[i]);
  }
  glutSwapBuffers();
}

void
overlayDisplay(void)
{
  if (glutLayerGet(GLUT_OVERLAY_DAMAGED)) {
    /* If damaged, clear the overlay. */
    glClear(GL_COLOR_BUFFER_BIT);
  } else {
    /* If not damaged, undraw last overlay sphere. */
    glIndexi(transparent);
    drawSphere(&oldOverlaySphere);
  }
  glIndexi(opaque);
  drawSphere(&overlaySphere);
  /* Single buffered window needs flush. */
  glFlush();
  /* Remember last overaly sphere position for undrawing. */
  oldOverlaySphere = overlaySphere;
}

void
reshape(int w, int h)
{
  width = w;
  height = h;
  /* Reshape both layers. */
  glutUseLayer(GLUT_OVERLAY);
  glViewport(0, 0, w, h);
  glutUseLayer(GLUT_NORMAL);
  glViewport(0, 0, w, h);
  /* Read back viewport for gluUnProject. */
  glGetIntegerv(GL_VIEWPORT, viewport);
}

void
mouse(int button, int state, int x, int y)
{
  GLdouble objx, objy, objz;

  gluUnProject(x, height - y, 0.95,
    modelMatrix, projMatrix, viewport,
    &objx, &objy, &objz);
  overlaySphere.x = objx;
  overlaySphere.y = objy;
  overlaySphere.z = objz;
  overlaySphere.material = button;
  glutUseLayer(GLUT_OVERLAY);
  glutSetColor(opaque,
    2 * matColor[button][0],  /* Red. */
    2 * matColor[button][1],  /* Green. */
    2 * matColor[button][2]);  /* Blue. */
  if (state == GLUT_UP) {
    glutHideOverlay();
    if (spheres < MAX_SPHERES) {
      sphereInfo[spheres] = overlaySphere;
      sphereInfo[spheres].detail = 25;  /* Fine tesselation. */
      spheres++;
    } else {
      printf("oversphere: Out of spheres.\n");
    }
    glutPostRedisplay();
  } else {
    overlaySphere.detail = 10;  /* Coarse tesselation. */
    glutShowOverlay();
    glutPostOverlayRedisplay();
  }
}

void
motion(int x, int y)
{
  GLdouble objx, objy, objz;

  gluUnProject(x, height - y, 0.95,
    modelMatrix, projMatrix, viewport,
    &objx, &objy, &objz);
  overlaySphere.x = objx;
  overlaySphere.y = objy;
  overlaySphere.z = objz;
  glutPostOverlayRedisplay();
}

void
setupMatrices(void)
{
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* degrees field of view */ 50.0,
    /* aspect ratio */ 1.0, /* Z near */ 1.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(
    0.0, 0.0, 5.0,      /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */
}

int
main(int argc, char **argv)
{
  glutInitWindowSize(350, 350);
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("Overlay Sphere Positioning Demo");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);  /* Solid spheres benefit greatly
                              from back face culling. */
  setupMatrices();
  /* Read back matrices for use by gluUnProject. */
  glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

  /* Set up lighting. */
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
  glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);

  glutInitDisplayMode(GLUT_INDEX | GLUT_SINGLE);
  if (glutLayerGet(GLUT_OVERLAY_POSSIBLE) == 0) {
    printf("oversphere: no overlays supported; aborting.\n");
    exit(1);
  }
  glutEstablishOverlay();
  glutHideOverlay();
  glutOverlayDisplayFunc(overlayDisplay);

  /* Find transparent and opaque index. */
  transparent = glutLayerGet(GLUT_TRANSPARENT_INDEX);
  opaque = (transparent + 1)
    % glutGet(GLUT_WINDOW_COLORMAP_SIZE);

  /* Draw overlay sphere as an outline. */
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  /* Make sure overlay clears to transparent. */
  glClearIndex(transparent);
  /* Set up overlay matrices same as normal plane. */
  setupMatrices();

  glutMainLoop();
  return 0;
}
