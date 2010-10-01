
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o txfdemo txfdemo.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <GL/glut.h>

#include "TexFont.h"

/* Uncomment to debug various scenarios. */
#if 0
#undef GL_VERSION_1_1
#undef GL_EXT_polygon_offset
#endif

#ifndef GL_VERSION_1_1
#ifdef GL_EXT_polygon_offset
#define GL_POLYGON_OFFSET_FILL GL_POLYGON_OFFSET_EXT
#define glPolygonOffset(s,b) glPolygonOffsetEXT(s,b*0.001);
#else
/* Gag.  No polygon offset?  Artifacts will exist. */
#define glPolygonOffset(s,b) /* nothing */
#endif
#endif

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

static int doubleBuffer = 1;
static char *filename = "rockfont.txf";
static GLfloat angle = 20;
static TexFont *txf;
static int usePolygonOffset = 1;
static int animation = 1;
static int fullscreen = 0;

void
idle(void)
{
  angle += 4;
  glutPostRedisplay();
}

void 
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (animation) {
      glutIdleFunc(idle);
    }
  } else {
    glutIdleFunc(NULL);
  }
}

void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(60.0, (GLfloat) w/(GLfloat) h, 1.0, 20.0);
   glMatrixMode(GL_MODELVIEW);
}

void
cubeSide(void)
{
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glDisable(GL_ALPHA_TEST);
  glColor3f(0.3, 0.7, 0.3);
  glRectf(-1.0, -1.0, 1.0, 1.0);
}

int alphaMode;

void
alphaModeSet(void)
{
  switch (alphaMode) {
  case GL_ALPHA_TEST:
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.5);
    break;
  case GL_BLEND:
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case GL_ALPHA_TEST + GL_BLEND:
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.0625);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case GL_NONE:
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    break;
  }
}

void
cubeSideWithOpenGLcircle(void)
{
  int w, ow, a, d;
  char *text;
  int len;
  int i;
  GLfloat flen;

  cubeSide();

  glPushMatrix();
    alphaModeSet();
    glEnable(GL_TEXTURE_2D);
    if (usePolygonOffset) {
#if defined(GL_EXT_polygon_offset) || defined(GL_VERSION_1_1)
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0.0, -3);
#endif
    }
    glColor3f(0.2, 0.2, 0.9);

    txfGetStringMetrics(txf, "OpenGL", 6, &w, &a, &d);
    text = "OpenGL OpenGL ";
    len = (int) strlen(text);
    txfGetStringMetrics(txf, text, len, &w, &a, &d);
    txfGetStringMetrics(txf, "O", 1, &ow, &a, &d);

    glScalef(5.6/w, 5.6/w, 5.6/w);
    flen = len;
    glTranslatef(-ow/2.0, -w/(M_PI*2.0), 0.0);

    for (i=0; i<len; i++) {
      if (text[i] == 'L' && usePolygonOffset) {
	/* Hack.  The "L" in OpenGL slightly overlaps the "G". Slightly
	   raise the "L" so that it will overlap the "G" in the depth
	   buffer to avoid a double blend.. */
        glPolygonOffset(0.0, -4);
        txfRenderGlyph(txf, text[i]);
        glPolygonOffset(0.0, -3);
      } else {
        txfRenderGlyph(txf, text[i]);
      }
      glRotatef(360.0/flen, 0, 0, 1);
    }
    if (usePolygonOffset) {
#if defined(GL_EXT_polygon_offset) || defined(GL_VERSION_1_1)
      glDisable(GL_POLYGON_OFFSET_FILL);
#endif
    }
  glPopMatrix();
}

void
cubeSideWithText(char *text, int len)
{
  int w, a, d;

  cubeSide();

  glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    alphaModeSet();
    if (usePolygonOffset) {
#if defined(GL_EXT_polygon_offset) || defined(GL_VERSION_1_1)
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0.0, -3);
#endif
    }
    glColor3f(0.2, 0.2, 0.9);

    txfGetStringMetrics(txf, text, len, &w, &a, &d);

    glScalef(1.8/w, 1.8/w, 1.8/w);
    glTranslatef(-w/2.0, d-(a+d)/2.0, 0.0);

    txfRenderFancyString(txf, text, len);
    if (usePolygonOffset) {
#if defined(GL_EXT_polygon_offset) || defined(GL_VERSION_1_1)
      glDisable(GL_POLYGON_OFFSET_FILL);
#endif
    }
  glPopMatrix();
}

void
display(void)
{
  char *str;

  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  glRotatef(-angle, 0, 1, 0);

  glPushMatrix();
    glTranslatef(0.0, 0.0, 1.0);
    cubeSideWithOpenGLcircle();
  glPopMatrix();
  glPushMatrix();
    glRotatef(90.0, 0, 1, 0);
    glTranslatef(0.0, 0.0, 1.0);
    str = "MAkes";
    cubeSideWithText(str, (int) strlen(str));
  glPopMatrix();
  glPushMatrix();
    glRotatef(180.0, 0, 1, 0);
    glTranslatef(0.0, 0.0, 1.0);
    str = "Text";
    cubeSideWithText(str, (int) strlen(str));
  glPopMatrix();
  glPushMatrix();
    glRotatef(270.0, 0, 1, 0);
    glTranslatef(0.0, 0.0, 1.0);
    str = "\033T\377\000\000\000\000\3773D";
    cubeSideWithText(str, 10);
  glPopMatrix();

  glPopMatrix();

  /* Swap the buffers if necessary. */
  if (doubleBuffer) {
    glutSwapBuffers();
  }
}

int savex;

/* ARGSUSED23 */
void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      glutIdleFunc(NULL);
      savex = x;
    } else {
      if (animation)
        glutIdleFunc(idle);
    }
  }
}

/* ARGSUSED1 */
void
motion(int x, int y)
{
  angle += (savex - x);
  savex = x;
  glutPostRedisplay();
}

int minifyMenu;

void
minifySelect(int value)
{
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
  glutPostRedisplay();
}

int alphaMenu;

void
alphaSelect(int value)
{
  alphaMode = value;
  glutPostRedisplay();
}

int polygonOffsetMenu;

void
polygonOffsetSelect(int value)
{
  usePolygonOffset = value;
  glutPostRedisplay();
}

int animationMenu;

void
animationSelect(int value)
{
  animation = value;
  if (animation) {
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}

/* ARGSUSED1 */
void
keyboard(unsigned char c, int x, int y)
{
  switch(c) {
  case 27:
    exit(0);
    break;
  case ' ':
    animation = 1 - animation;
    if (animation) {
      glutIdleFunc(idle);
    } else  {
      glutIdleFunc(NULL);
    }
    break;
  }
}

void
mainSelect(int value)
{
  if (value == 666) {
    exit(0);
  }
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-sb")) {
      doubleBuffer = 0;
    } else if (!strcmp(argv[i], "-fullscreen")) {
      fullscreen = 1;
    } else {
      filename = argv[i];
    }
  }
  if (filename == NULL) {
    fprintf(stderr, "usage: txfdemo [GLUT-options] [-sb] txf-file\n");
    exit(1);
  }

  txf = txfLoadFont(filename);
  if (txf == NULL) {
    fprintf(stderr, "Problem loading %s, %s\n", filename, txfErrorString());
    exit(1);
  }

  if (doubleBuffer) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  } else {
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  }
  glutInitWindowSize(300, 300);
  if (fullscreen) {
    glutGameModeString("640x480:16@60");
    glutEnterGameMode();
  } else {
    glutCreateWindow("txfdemo");
  }
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutKeyboardFunc(keyboard);
  glutVisibilityFunc(visible);
  glutReshapeFunc(reshape);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, 1.0, 0.1, 20.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 4.0,
    0.0, 0.0, 0.0,
    0.0, 1.0, 0.0);

  /* Use a gray background so the teximage with black backgrounds will show
     against showtxf's background. */
  glClearColor(0.0, 0.0, 0.0, 0.0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  alphaSelect(GL_ALPHA_TEST);
  minifySelect(GL_NEAREST);

  txfEstablishTexture(txf, 1, GL_TRUE);

  if (!fullscreen) {
    minifyMenu = glutCreateMenu(minifySelect);
    glutAddMenuEntry("Nearest", GL_NEAREST);
    glutAddMenuEntry("Linear", GL_LINEAR);
    glutAddMenuEntry("Nearest mipmap nearest", GL_NEAREST_MIPMAP_NEAREST);
    glutAddMenuEntry("Linear mipmap nearest", GL_LINEAR_MIPMAP_NEAREST);
    glutAddMenuEntry("Nearest mipmap linear", GL_NEAREST_MIPMAP_LINEAR);
    glutAddMenuEntry("Linear mipmap linear", GL_LINEAR_MIPMAP_LINEAR);

    alphaMenu = glutCreateMenu(alphaSelect);
    glutAddMenuEntry("Alpha testing", GL_ALPHA_TEST);
    glutAddMenuEntry("Alpha blending", GL_BLEND);
    glutAddMenuEntry("Both", GL_ALPHA_TEST + GL_BLEND);
    glutAddMenuEntry("Nothing", GL_NONE);

    polygonOffsetMenu = glutCreateMenu(polygonOffsetSelect);
    glutAddMenuEntry("Enable", 1);
    glutAddMenuEntry("Disable", 0);

    animationMenu = glutCreateMenu(animationSelect);
    glutAddMenuEntry("Start", 1);
    glutAddMenuEntry("Stop", 0);

    glutCreateMenu(mainSelect);
    glutAddSubMenu("Filtering", minifyMenu);
    glutAddSubMenu("Alpha", alphaMenu);
    glutAddSubMenu("Polygon Offset", polygonOffsetMenu);
    glutAddSubMenu("Animation", animationMenu);
    glutAddMenuEntry("Quit", 666);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
  }

#if !defined(GL_EXT_polygon_offset) && !defined(GL_VERSION_1_1)
  fprintf(stderr, "Warning: polygon offset not available; artifacts will results.\n");
#endif

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
