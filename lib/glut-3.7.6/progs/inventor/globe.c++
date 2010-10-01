
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Inventor/nodes/SoTexture2Transform.h>

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
SoRotationXYZ *globeSpin;
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

void
globeScene(void)
{
   root = new SoSeparator;
   root->ref();

   // Add a camera and light
   SoPerspectiveCamera *myCamera = new SoPerspectiveCamera;
   myCamera->position.setValue(0., 0., 2.2);
   myCamera->heightAngle = M_PI/2.5; 
   myCamera->nearDistance = 0.5;
   myCamera->farDistance = 10.0;
   root->addChild(myCamera);
   root->addChild(new SoDirectionalLight);

   SoRotationXYZ *globalRotXYZ = new SoRotationXYZ;
   globalRotXYZ->axis = SoRotationXYZ::X;
   globalRotXYZ->angle = M_PI/9;
   root->addChild(globalRotXYZ);

   // Set up the globe transformations
   globeSpin = new SoRotationXYZ;
   root->addChild(globeSpin);
   globeSpin->angle = angle;
   globeSpin->axis = SoRotationXYZ::Y;  // rotate about Y axis

   // Add the globe, a sphere with a texture map.
   // Put it within a separator.
   SoSeparator *sphereSep = new SoSeparator;
   SoTexture2  *myTexture2 = new SoTexture2;
   SoComplexity *sphereComplexity = new SoComplexity;
   sphereComplexity->value = 0.55;
   root->addChild(sphereSep);
   sphereSep->addChild(myTexture2);
   sphereSep->addChild(sphereComplexity);
   sphereSep->addChild(new SoSphere);
   myTexture2->filename = "globe.rgb";
}

void
updateModels(void)
{
  globeSpin->angle = angle;
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
  globeScene();

  glutInitWindowSize(W, H);
  glutCreateWindow("As the world turns");
  glutDisplayFunc(redraw);
  glutReshapeFunc(reshape);
  glutCreateMenu(menuSelect);
  glutAddMenuEntry("Step", 1);
  glutAddMenuEntry("Toggle spinning", 2);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutVisibilityFunc(vis);

  /* Enable depth testing for Open Inventor. */
  glEnable(GL_DEPTH_TEST);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
