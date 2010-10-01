
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int on = 0;
int independent = 0;
int main_w, hidden_w, s1, s2;
float x = 0, y = 0;

void
overlay_display(void)
{
  printf("overlay_display: damaged=%d\n", glutLayerGet(GLUT_OVERLAY_DAMAGED));
  if (on) {
    glutUseLayer(GLUT_OVERLAY);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
    glVertex2f(.2 + x, .2 + y);
    glVertex2f(.5 + x, .5 + y);
    glVertex2f(.2 + x, .5 + y);
    glEnd();
    glFlush();
  }
}

void
display(void)
{
  printf("normal_display: damaged=%d\n", glutLayerGet(GLUT_NORMAL_DAMAGED));
  glutUseLayer(GLUT_NORMAL);
  glClear(GL_COLOR_BUFFER_BIT);
  glColor3f(1.0, 0.0, 0.0);
  glBegin(GL_POLYGON);
  glVertex2f(.2, .28);
  glVertex2f(.5, .58);
  glVertex2f(.2, .58);
  glEnd();

  if (!independent) {
    overlay_display();
  } else {
    printf("not calling overlay_display\n");
  }
}

void
hidden_display(void)
{
  printf("hidden_display: this should not be called ever\n");
}

void
reshape(int w, int h)
{
  glutUseLayer(GLUT_NORMAL);
  glViewport(0, 0, w, h);

  if (on) {
    glutUseLayer(GLUT_OVERLAY);
    glViewport(0, 0, w, h);
    printf("w=%d, h=%d\n", w, h);
  }
}

void
special(int c, int w, int h)
{
  printf("special %d  w=%d h=%d\n", c, w, h);
  if (on) {
    switch (c) {
    case GLUT_KEY_LEFT:
      x -= 0.1;
      break;
    case GLUT_KEY_RIGHT:
      x += 0.1;
      break;
    case GLUT_KEY_UP:
      y += 0.1;
      break;
    case GLUT_KEY_DOWN:
      y -= 0.1;
      break;
    }
    glutPostOverlayRedisplay();
  }
}

void
key(unsigned char c, int w, int h)
{
  int transP;

  printf("c=%d  w=%d h=%d\n", c, w, h);
  switch (c) {
  case 'e':
    glutEstablishOverlay();
    independent = 0;
    transP = glutLayerGet(GLUT_TRANSPARENT_INDEX);
    glClearIndex(transP);
    glutSetColor((transP + 1) % 2, 1.0, 1.0, 0.0);
    glIndexi((transP + 1) % 2);
    on = 1;
    break;
  case 'r':
    glutRemoveOverlay();
    on = 0;
    break;
  case 'm':
    if (glutLayerGet(GLUT_HAS_OVERLAY)) {
      int pixel;
      GLfloat red, green, blue;

      transP = glutLayerGet(GLUT_TRANSPARENT_INDEX);
      pixel = (transP + 1) % 2;
      red = glutGetColor(pixel, GLUT_RED) + 0.2;
      if (red > 1.0)
        red = red - 1.0;
      green = glutGetColor(pixel, GLUT_GREEN) - 0.1;
      if (green > 1.0)
        green = green - 1.0;
      blue = glutGetColor(pixel, GLUT_BLUE) + 0.1;
      if (blue > 1.0)
        blue = blue - 1.0;
      glutSetColor(pixel, red, green, blue);
    }
    break;
  case 'h':
    glutSetWindow(hidden_w);
    glutHideWindow();
    glutSetWindow(s2);
    glutHideWindow();
    break;
  case 's':
    glutSetWindow(hidden_w);
    glutShowWindow();
    glutSetWindow(s2);
    glutShowWindow();
    break;
  case 'H':
    glutHideOverlay();
    break;
  case 'S':
    glutShowOverlay();
    break;
  case 'D':
    glutDestroyWindow(main_w);
    exit(0);
    break;
  case ' ':
    printf("overlay possible: %d\n", glutLayerGet(GLUT_OVERLAY_POSSIBLE));
    printf("layer in  use: %d\n", glutLayerGet(GLUT_LAYER_IN_USE));
    printf("has overlay: %d\n", glutLayerGet(GLUT_HAS_OVERLAY));
    printf("transparent index: %d\n", glutLayerGet(GLUT_TRANSPARENT_INDEX));
    break;
  }
}

/* ARGSUSED1 */
void
key2(unsigned char c, int w, int h)
{
  int transP;

  printf("c=%d\n", c);
  switch (c) {
  case 'g':
    glutReshapeWindow(
      glutGet(GLUT_WINDOW_WIDTH) + 2, glutGet(GLUT_WINDOW_HEIGHT) + 2);
    break;
  case 's':
    glutReshapeWindow(
      glutGet(GLUT_WINDOW_WIDTH) - 2, glutGet(GLUT_WINDOW_HEIGHT) - 2);
    break;
  case 'u':
    glutPopWindow();
    break;
  case 'd':
    glutPushWindow();
    break;
  case 'e':
    glutEstablishOverlay();
    transP = glutLayerGet(GLUT_TRANSPARENT_INDEX);
    glClearIndex(transP);
    glutSetColor((transP + 1) % 2, 0.0, 0.25, 0.0);
    glIndexi((transP + 1) % 2);
    break;
  case 'c':
    if (glutLayerGet(GLUT_HAS_OVERLAY)) {
      glutUseLayer(GLUT_OVERLAY);
      glutCopyColormap(main_w);
    }
    break;
  case 'r':
    glutRemoveOverlay();
    break;
  case ' ':
    printf("overlay possible: %d\n", glutLayerGet(GLUT_OVERLAY_POSSIBLE));
    printf("layer in  use: %d\n", glutLayerGet(GLUT_LAYER_IN_USE));
    printf("has overlay: %d\n", glutLayerGet(GLUT_HAS_OVERLAY));
    printf("transparent index: %d\n", glutLayerGet(GLUT_TRANSPARENT_INDEX));
    break;
  }
}

void
vis(int state)
{
  if (state == GLUT_VISIBLE)
    printf("visible %d\n", glutGetWindow());
  else
    printf("NOT visible %d\n", glutGetWindow());
}

void
entry(int state)
{
  if (state == GLUT_LEFT)
    printf("LEFT %d\n", glutGetWindow());
  else
    printf("entered %d\n", glutGetWindow());
}

void
motion(int x, int y)
{
  printf("motion x=%x y=%d\n", x, y);
}

void
mouse(int b, int s, int x, int y)
{
  printf("b=%d  s=%d  x=%d y=%d\n", b, s, x, y);
}

void
display2(void)
{
  glutUseLayer(GLUT_NORMAL);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();

  if (glutLayerGet(GLUT_HAS_OVERLAY)) {
    glutUseLayer(GLUT_OVERLAY);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
    glVertex2f(.2, .28);
    glVertex2f(.5, .58);
    glVertex2f(.2, .58);
    glEnd();
    glFlush();
  }
}

void
dial(int dial, int value)
{
  printf("dial %d %d (%d)\n", dial, value, glutGetWindow());
}

void
box(int button, int state)
{
  printf("box %d %d (%d)\n", button, state, glutGetWindow());
}

void
main_menu(int option)
{
  switch (option) {
  case 1:
    if (glutLayerGet(GLUT_HAS_OVERLAY)) {
      independent = 1;
      glutOverlayDisplayFunc(overlay_display);
    }
    break;
  case 2:
    if (glutLayerGet(GLUT_HAS_OVERLAY)) {
      independent = 0;
      glutOverlayDisplayFunc(NULL);
    }
    break;
  case 666:
    exit(0);
    break;
  }
}

void
s2_menu(int option)
{
  int transP;

  switch (option) {
  case 1:
    glutRemoveOverlay();
    break;
  case 2:
    glutEstablishOverlay();
    transP = glutLayerGet(GLUT_TRANSPARENT_INDEX);
    glClearIndex(transP);
    glutSetColor((transP + 1) % 2, 0.0, 0.25, 0.0);
    glIndexi((transP + 1) % 2);
    break;
  case 666:
    exit(0);
    break;
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB);
  glutInitWindowSize(210, 210);

  main_w = glutCreateWindow("overlay test");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glClearColor(1.0, 0.0, 1.0, 1.0);

  glutKeyboardFunc(key);
  glutVisibilityFunc(vis);
  glutEntryFunc(entry);
  glutSpecialFunc(special);

  glutMotionFunc(motion);
  glutMouseFunc(mouse);

  glutCreateMenu(main_menu);
  glutAddMenuEntry("Dual display callbacks", 1);
  glutAddMenuEntry("Single display callbacks", 2);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  hidden_w = glutCreateSubWindow(main_w, 10, 10, 100, 90);
  /* hidden_w is completely obscured by its own s1 subwindow.
     While display, entry and visibility callbacks are 
     registered, they will never be called. */
  glutDisplayFunc(hidden_display);
  glutEntryFunc(entry);
  glutVisibilityFunc(vis);

  s1 = glutCreateSubWindow(hidden_w, 0, 0, 100, 90);
  glClearColor(0.0, 0.0, 1.0, 1.0);
  glutDisplayFunc(display2);
#if 0
  glutKeyboardFunc(key2);
#endif
  glutVisibilityFunc(vis);
  glutEntryFunc(entry);

  s2 = glutCreateSubWindow(main_w, 35, 35, 100, 90);
  glClearColor(0.5, 0.0, 0.5, 1.0);
  glutDisplayFunc(display2);
#if 1
  glutKeyboardFunc(key2);
#endif
  glutVisibilityFunc(vis);
  glutEntryFunc(entry);

#if 1
  glutCreateMenu(s2_menu);
  glutAddMenuEntry("Remove overlay", 1);
  glutAddMenuEntry("Establish overlay", 2);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
#endif

  glutInitDisplayMode(GLUT_INDEX);

#if 1
  glutSetWindow(main_w);
  glutDialsFunc(dial);
  glutButtonBoxFunc(box);
#endif

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
