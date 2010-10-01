
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* This program demonstrates an OpenGL histogram extension based algorithm
   for occlusion culling.  Occlusion culling tries to speed rendering by
   quickly determine what objects that are in the view frustrum are not
   actually visible because they are occluded by other objects in the scene.
   If object occlusion can be quickly determined, you can save time by simply 
   not rendering occluded objects.  */

/* cc -o occlude occlude.c -lglut -lGL -lGLU -lXmu -lXext -lX11 -lm */

/* XXX Note that IMPACTs running IRIX 6.2 and earlier do not implement the
   histogram extension over alpha correctly.  The algorithm works correctly
   on RealityEngine and InfiniteReality platforms. */

/* Hacked starting with the OpenGL programming Guide's scene.c */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <GL/glut.h>

#define TORUS		1
#define TETRAHEDRON	2
#define ICOSAHEDRON	3

int W = 250, H = 250;
int showBoxes = 0;
int single = 0;
int showHistogram = 0;
int showRate = 0;
int occlusionDectection = 1;
int nameOccludedTeapots = 1;
int noOcclude = 0;
int frames = 0;
int renderCount = 0, occludedCount = 0;

void
output(GLfloat x, GLfloat y, char *format,...)
{
  va_list args;
  char buffer[200], *p;

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  glPushMatrix();
  glTranslatef(x, y, 0);
  for (p = buffer; *p; p++)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  glPopMatrix();
}

/* Initialize material property and light source. */
void
myinit(void)
{
  GLfloat light_ambient[] =
  {0.2, 0.2, 0.2, 1.0};
  GLfloat light_diffuse[] =
  {1.0, 1.0, 1.0, 1.0};
  GLfloat light_specular[] =
  {1.0, 1.0, 1.0, 1.0};
  GLfloat light_position[] =
  {1.0, 1.0, 1.0, 0.0};

  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glEnable(GL_LIGHT0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  glNewList(TORUS, GL_COMPILE);
  glutSolidTorus(0.275, 0.85, 10, 15);
  glEndList();
  glNewList(TETRAHEDRON, GL_COMPILE);
  glutSolidTetrahedron();
  glEndList();
  glNewList(ICOSAHEDRON, GL_COMPILE);
  glutSolidIcosahedron();
  glEndList();

  /* Make sure we clear with alpha set to 1. */
  glClearColor(0, 0, 0, 1);

#ifdef GL_EXT_histogram
  /* Do a histogram on alpha with 8 bins; throw away the image data used when
     computing the histogram. */
  glHistogramEXT(GL_HISTOGRAM_EXT, 8, GL_ALPHA, GL_TRUE);
#endif
}

void
draw(void)
{
  glPushMatrix();
  glScalef(1.3, 1.3, 1.3);
  glRotatef(23.0, 1.0, 0.0, 0.0);

  glPushMatrix();
  glTranslatef(-0.75, -0.5, 0.0);
  glRotatef(270.0, 1.0, 0.0, 0.0);
  glCallList(TETRAHEDRON);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-0.75, 0.5, 0.0);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  glCallList(TORUS);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0.75, 0.0, -1.0);
  glCallList(ICOSAHEDRON);
  glPopMatrix();

  glPopMatrix();
}

void
myortho(void)
{
  if (W <= H)
    glOrtho(-2.5, 2.5, -2.5 * (GLfloat) H / (GLfloat) W,
      2.5 * (GLfloat) H / (GLfloat) W, -10.0, 10.0);
  else
    glOrtho(-2.5 * (GLfloat) W / (GLfloat) H,
      2.5 * (GLfloat) W / (GLfloat) H, -2.5, 2.5, -10.0, 10.0);
}

GLint turn = 90;
int dir = -3;

void
idle(void)
{
  /* Make the angle alternate back and forth. */
  turn += dir;
  if (turn > 50)
    dir = -3;
  if (turn <= 0)
    dir = 3;
  glutPostRedisplay();
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);
}

void
boundingBox(void)
{
  glScalef(1.9 * .6, 1.3 * .6, 1.2 * .6);
  glutSolidCube(0.9);
}

void
teapot(void)
{
  glutSolidTeapot(0.3);
}

typedef void (*RenderFunc) (void);

void
render1(RenderFunc func)
{
  glPushMatrix();
  glTranslatef(-0.75, 0.5, 0.0);
  glRotatef(turn, 0.0, 1.0, 0.0);
  glTranslatef(3.0, 0.0, 0.0);
  glColor4f(0, 1, 0, .25);
  func();
  glPopMatrix();
}

void
render2(RenderFunc func)
{
  glPushMatrix();
  glTranslatef(-0.75, 0.5, 0.0);
  glRotatef(turn + 30, 0.0, 1.0, 0.0);
  glTranslatef(3.0, 0.0, 0.0);
  glColor4f(1, 0, 0, .5);
  func();
  glPopMatrix();
}

void
render3(RenderFunc func)
{
  glPushMatrix();
  glTranslatef(-0.75, 0.5, 0.0);
  glRotatef(turn + 60, 0.0, 1.0, 0.0);
  glTranslatef(3.0, 0.0, 0.0);
  glColor4f(0, 0, 1, .0);
  func();
  glPopMatrix();
}

void
render4(RenderFunc func)
{
  glPushMatrix();
  glTranslatef(-0.75, -0.75, 0.0);
  glRotatef(turn + 60, 0.0, 1.0, 0.0);
  glTranslatef(3.0, 0.0, 0.0);
  glColor4f(1, 1, 0, .75);
  func();
  glPopMatrix();
}

void
render5(RenderFunc func)
{
  glPushMatrix();
  glTranslatef(-0.75, -0.25, 0.0);
  glRotatef(turn + 45, 0.0, 1.0, 0.0);
  glTranslatef(3.0, 0.0, 0.0);
  glColor4f(1, 0, 1, .12);
  func();
  glPopMatrix();
}

void
display(void)
{
  int teapot1, teapot2, teapot3, teapot4, teapot5;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw();

  /* Draw all teapots. */
  teapot1 = teapot2 = teapot3 = teapot4 = teapot5 = 1;
#ifdef GL_EXT_histogram
  if (occlusionDectection && !noOcclude) {
    GLuint count_buffer[8];
    int i;

    glDisable(GL_LIGHTING);
    if (!showBoxes) {
      glDepthMask(GL_FALSE);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    }
    /* Render the bounding box for each teapot. */
    render1(boundingBox);
    render2(boundingBox);
    render3(boundingBox);
    render4(boundingBox);
    render5(boundingBox);

    glEnable(GL_HISTOGRAM_EXT);
    /* Sort of cheat.  I know all the teapots move in the center horizontal
       half of the screen so only take the histogram over that area instead
       of the full window. */
    glCopyPixels(0, H / 4, W, H / 2, GL_COLOR);
    glGetHistogramEXT(GL_HISTOGRAM_EXT, GL_TRUE,
      GL_ALPHA, GL_UNSIGNED_INT, count_buffer);
    glDisable(GL_HISTOGRAM_EXT);
    if (showHistogram) {
      printf("%2d: ", turn);
      for (i = 0; i < 8; i++)
        printf(" %7d", count_buffer[i]);
      printf("\n");
    }
    /* Get the count from each histogram bucket for each teapot's bounding
       box. */
    teapot1 = count_buffer[2];
    teapot2 = count_buffer[3];
    teapot3 = count_buffer[0];
    teapot4 = count_buffer[5];
    teapot5 = count_buffer[1];

    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  }
#endif
  /* If each teapot needs to be drawn (ie, not occluded when occlusion
     detection is enabled, draw it. */
  if (teapot1) {
    render1(teapot);
    renderCount++;
  } else {
    occludedCount++;
  }
  if (teapot2) {
    render2(teapot);
    renderCount++;
  } else {
    occludedCount++;
  }
  if (teapot3) {
    render3(teapot);
    renderCount++;
  } else {
    occludedCount++;
  }
  if (teapot4) {
    render4(teapot);
    renderCount++;
  } else {
    occludedCount++;
  }
  if (teapot5) {
    render5(teapot);
    renderCount++;
  } else {
    occludedCount++;
  }

  /* To help see when occlusions take place, render the teapot number of each 

     occluded teapot. */
  if (nameOccludedTeapots) {
    if (!teapot1 || !teapot2 || !teapot3 || !teapot4 || !teapot5) {
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);

      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      gluOrtho2D(0, 3000, 0, 3000);
      glMatrixMode(GL_MODELVIEW);

      glColor3f(1, 1, 0);  /* Yellow text. */

      if (!teapot1) {
        glLoadIdentity();
        output(80, 2800, "1");
      }
      if (!teapot2) {
        glLoadIdentity();
        output(150, 2800, "2");
      }
      if (!teapot3) {
        glLoadIdentity();
        output(220, 2800, "3");
      }
      if (!teapot4) {
        glLoadIdentity();
        output(290, 2800, "4");
      }
      if (!teapot5) {
        glLoadIdentity();
        output(360, 2800, "5");
      }
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHTING);
    }
  }
  if (!single) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
  frames++;
}

void
reshape(int w, int h)
{
  W = w;
  H = h;
  glViewport(0, 0, W, H);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  myortho();
  glMatrixMode(GL_MODELVIEW);
}

void
main_menu(int value)
{
  switch (value) {
  case 666:
    exit(0);
    break;
  case 0:
    showBoxes = !showBoxes;
    break;
  case 1:
    showHistogram = !showHistogram;
    break;
  case 2:
    occlusionDectection = !occlusionDectection;
    break;
  case 3:
    nameOccludedTeapots = !nameOccludedTeapots;
    break;
  case 4:
    showRate = !showRate;
    break;
  case 5:
    noOcclude = !noOcclude;
    break;
  }
  glutPostRedisplay();
}

/* This timer callback will print out stats every three seconds of the frame
   rate and precent of teapots occluded. */
/* ARGSUSED */
void
timer(int value)
{
  static  int last = 0;
  int now;
  float time, total;

  now = glutGet(GLUT_ELAPSED_TIME);
  time = (now - last) / 1000;
  total = renderCount + occludedCount;
  if (showRate) {
    if (frames) {
      if (occlusionDectection) {
        printf("rate = %.1f (detection on) @ %%%.0f\n",
          frames / time, occludedCount/total*100);
      } else {
        printf("rate = %.1f @ %%%.0f\n",
          frames / time, occludedCount/total*100);
      }
    }
  }
  last = now;
  frames = 0;
  renderCount = 0;
  occludedCount = 0;
  glutTimerFunc(3000, timer, 0);
}

int
main(int argc, char **argv)
{
  int has_histogram, has_logic_op;

  glutInit(&argc, argv);
  glutInitWindowSize(W, H);
  if (argc > 1 && !strcmp(argv[1], "-single")) {
    single = 1;
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH | GLUT_ALPHA);
  } else {
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_ALPHA);
  }
  glutCreateWindow(argv[0]);
  has_histogram = glutExtensionSupported("GL_EXT_histogram");
  has_logic_op = glutExtensionSupported("GL_EXT_blend_logic_op");
  if (!has_histogram && !has_logic_op) {
    fprintf(stderr,
      "\nYour OpenGL implementation lacks support for\nEXT_histogram or EXT_blend_logic_op (or both).\n\n");
    exit(1);
  }
  myinit();
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutCreateMenu(main_menu);
  glutAddMenuEntry("Toggle occlusion detection", 2);
  glutAddMenuEntry("Toggle bounding boxes", 0);
  glutAddMenuEntry("Toggle histogram print", 1);
  glutAddMenuEntry("Toggle name occluded teapots", 3);
  glutAddMenuEntry("Toggle frame rate print", 4);
  glutAddMenuEntry("Toggle histogram without occlusion", 5);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutVisibilityFunc(visible);
  glutTimerFunc(3000, timer, 0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
