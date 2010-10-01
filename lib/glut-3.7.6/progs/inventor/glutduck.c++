
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/*
 * Copyright (c) 1991-94 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the name of Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
 * POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Based on an example from the Inventor Mentor chapter 13, example 5. */

#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoRotationXYZ.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int W = 300, H = 300;
int spinning = 0;
GLubyte *image = NULL;
SoSeparator *root;
SoRotationXYZ *duckRotXYZ;
float angle = 0.0;
int moving  = 0;
int begin;

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  W = w;
  H = h;
  if (image)
    free(image);
  image = (GLubyte *) malloc(W * H * 3);
}

void
renderScene(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  SbViewportRegion myViewport(W, H);
  SoGLRenderAction myRenderAction(myViewport);
  myRenderAction.apply(root);
}

void
redraw(void)
{
  renderScene();
  glutSwapBuffers();
}

int
duckScene(void)
{
   root = new SoSeparator;
   root->ref();

   // Add a camera and light
   SoPerspectiveCamera *myCamera = new SoPerspectiveCamera;
   myCamera->position.setValue(0., -4., 8.0);
   myCamera->heightAngle = M_PI/2.5; 
   myCamera->nearDistance = 1.0;
   myCamera->farDistance = 15.0;
   root->addChild(myCamera);
   root->addChild(new SoDirectionalLight);

   // Rotate scene slightly to get better view
   SoRotationXYZ *globalRotXYZ = new SoRotationXYZ;
   globalRotXYZ->axis = SoRotationXYZ::X;
   globalRotXYZ->angle = M_PI/9;
   root->addChild(globalRotXYZ);

   // Pond group
   SoSeparator *pond = new SoSeparator; 
   root->addChild(pond);
   SoMaterial *cylMaterial = new SoMaterial;
   cylMaterial->diffuseColor.setValue(0., 0.3, 0.8);
   pond->addChild(cylMaterial);
   SoTranslation *cylTranslation = new SoTranslation;
   cylTranslation->translation.setValue(0., -6.725, 0.);
   pond->addChild(cylTranslation);
   SoCylinder *myCylinder = new SoCylinder;
   myCylinder->radius.setValue(4.0);
   myCylinder->height.setValue(0.5);
   pond->addChild(myCylinder);

   // Duck group
   SoSeparator *duck = new SoSeparator;
   root->addChild(duck);

   // Read the duck object from a file and add to the group
   SoInput myInput;
   if (!myInput.openFile("duck.iv"))  {
      if (!myInput.openFile("/usr/share/src/Inventor/examples/data/duck.iv")) {
        return 1;
      }
   }
   SoSeparator *duckObject = SoDB::readAll(&myInput);
   if (duckObject == NULL) return 1;

   // Set up the duck transformations
   duckRotXYZ = new SoRotationXYZ;
   duck->addChild(duckRotXYZ);
   duckRotXYZ->angle = angle;
   duckRotXYZ->axis = SoRotationXYZ::Y;  // rotate about Y axis
   SoTransform *initialTransform = new SoTransform;
   initialTransform->translation.setValue(0., 0., 3.);
   initialTransform->scaleFactor.setValue(6., 6., 6.);
   duck->addChild(initialTransform);
   duck->addChild(duckObject);

   return 0;
}

void
updateModels(void)
{
  duckRotXYZ->angle = angle;
  glutPostRedisplay();
}

void
animate(void)
{
  angle += 0.1;
  updateModels();
}

void
setAnimation(int enable)
{
  if(enable) {
    spinning = 1;
    glutIdleFunc(animate);
  } else {
    spinning = 0;
    glutIdleFunc(NULL);
    glutPostRedisplay();
  }
}

/* ARGSUSED */
void
keyboard(unsigned char ch, int x, int y)
{
  if(ch == ' ') {
    setAnimation(0);
    animate();
  }
}

void
menuSelect(int item)
{
   switch(item) {
   case 1:
     animate();
     break;
   case 2:
     if(!spinning) {
           setAnimation(1);
     } else {
           setAnimation(0);
     }
     break;
#ifdef GL_MULTISAMPLE_SGIS
   case 3:
     if(glIsEnabled(GL_MULTISAMPLE_SGIS)) {
       glDisable(GL_MULTISAMPLE_SGIS);
     } else {
       glEnable(GL_MULTISAMPLE_SGIS);
     }
     glutPostRedisplay();
     break;
#endif
   }
}

void
vis(int visible)
{
  if (visible == GLUT_VISIBLE) {
    if (spinning)
      glutIdleFunc(animate);
  } else {
    if (spinning)
      glutIdleFunc(NULL);
  }
}

/* ARGSUSED */
void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    setAnimation(0);
    moving = 1;
    begin = x;
  }
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    moving = 0;
    glutPostRedisplay();
  }
}

/* ARGSUSED */
void
motion(int x, int y)
{
  if (moving) {
    angle = angle + .01 * (x - begin);
    begin = x;
    updateModels();
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

  SoDB::init();

  if(duckScene()) {
    fprintf(stderr, "couldn't read IV file\n");
    exit(1);
  }
  glutInitWindowSize(W, H);
  glutCreateWindow("GLUT Inventor Duck Pond");
  glutDisplayFunc(redraw);
  glutReshapeFunc(reshape);
  glutCreateMenu(menuSelect);
  glutAddMenuEntry("Step", 1);
  glutAddMenuEntry("Toggle animation", 2);
  if(glutGet(GLUT_WINDOW_NUM_SAMPLES) > 0) {
      glutAddMenuEntry("Toggle multisampling", 3);
  }
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutVisibilityFunc(vis);
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.132, 0.542, 0.132, 1.0);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
