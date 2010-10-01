
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#include <GL/glut.h>

extern unsigned char mjk_image[];
extern int mjk_depth;
extern int mjk_height;
extern int mjk_width;

float tick1 = 0;
float tick2 = 0;
float angle;
float size;
int set_timeout = 0;
int visible = 0;
int spinning = 1;
int scaling = 1;
int interval = 100;
#define CUBE 1
#define SQUARES 2
#define DRUM 3
int mode = SQUARES;

void
animate(int value)
{
  if (visible) {
    if (spinning || scaling) {
      if (value) {
        if (spinning) {
          tick1 += 4 * (interval / 100.0);
          angle = ((int) tick1) % 360;
        }
        if (scaling) {
          tick2 += 2 * (interval / 100.0);
          size = .7 - .5 * sin(tick2 / 20.0);
        }
      }
      glutPostRedisplay();
      set_timeout = 1;
    }
  }
}

#define TIMEDELTA(dest, src1, src2) { \
        if(((dest).tv_usec = (src1).tv_usec - (src2).tv_usec) < 0) {\
              (dest).tv_usec += 1000000;\
              (dest).tv_sec = (src1).tv_sec - (src2).tv_sec - 1;\
        } else  (dest).tv_sec = (src1).tv_sec - (src2).tv_sec;  }

void
redraw(void)
{
  int begin, end, elapsed;
  int i, j;
  float amplitude;

  if (set_timeout) {
    begin = glutGet(GLUT_ELAPSED_TIME);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();

  if (mode != DRUM) {
    glScalef(size, size, size);
  }
  switch (mode) {
  case SQUARES:

#define COLS 6
#define TILE_TEX_W (1.0/COLS)
#define ROWS 6
#define TILE_TEX_H (1.0/ROWS)

    glTranslatef(-COLS / 2.0 + .5, -ROWS / 2.0 + .5, 0);
    for (i = 0; i < COLS; i++) {
      for (j = 0; j < ROWS; j++) {

        glPushMatrix();
        glTranslatef(i, j, 0);
        glRotatef(angle, 0, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(i * TILE_TEX_W, j * TILE_TEX_H);
        glVertex2f(-.5, -.5);
        glTexCoord2f((i + 1) * TILE_TEX_W, j * TILE_TEX_H);
        glVertex2f(.5, -.5);
        glTexCoord2f((i + 1) * TILE_TEX_W, (j + 1) * TILE_TEX_H);
        glVertex2f(.5, .5);
        glTexCoord2f(i * TILE_TEX_W, (j + 1) * TILE_TEX_H);
        glVertex2f(-.5, .5);
        glEnd();
        glPopMatrix();

      }
    }
    break;
  case DRUM:

#undef COLS
#undef TILE_TEX_W
#undef ROWS
#undef TILE_TEX_H
#define COLS 12
#define TILE_TEX_W (1.0/COLS)
#define ROWS 12
#define TILE_TEX_H (1.0/ROWS)

    glRotatef(angle, 0, 0, 1);
    glTranslatef(-COLS / 2.0 + .5, -ROWS / 2.0 + .5, 0);
    amplitude = 0.4 * sin(tick2 / 6.0);
    for (i = 0; i < COLS; i++) {
      for (j = 0; j < ROWS; j++) {

#define Z(x,y)	(((COLS-(x))*(x) + (ROWS-(y))*(y)) * amplitude) - 28.0

        glPushMatrix();
        glTranslatef(i, j, 0);
        glBegin(GL_QUADS);
        glTexCoord2f(i * TILE_TEX_W, j * TILE_TEX_H);
        glVertex3f(-.5, -.5, Z(i, j));
        glTexCoord2f((i + 1) * TILE_TEX_W, j * TILE_TEX_H);
        glVertex3f(.5, -.5, Z(i + 1, j));
        glTexCoord2f((i + 1) * TILE_TEX_W, (j + 1) * TILE_TEX_H);
        glVertex3f(.5, .5, Z(i + 1, j + 1));
        glTexCoord2f(i * TILE_TEX_W, (j + 1) * TILE_TEX_H);
        glVertex3f(-.5, .5, Z(i, j + 1));
        glEnd();
        glPopMatrix();

      }
    }
    break;
  case CUBE:
    glRotatef(angle, 0, 1, 0);
    glBegin(GL_QUADS);

    /* front */
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, -1.0, 1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, -1.0, 1.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, 1.0, 1.0);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, 1.0, 1.0);

    /* back */
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, 1.0, -1.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, 1.0, -1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, -1.0, -1.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, -1.0, -1.0);

    /* left */
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, -1.0, -1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(-1.0, -1.0, 1.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(-1.0, 1.0, 1.0);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, 1.0, -1.0);

    /* right */
    glTexCoord2f(0.0, 1.0);
    glVertex3f(1.0, 1.0, -1.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, 1.0, 1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, -1.0, 1.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(1.0, -1.0, -1.0);

    glEnd();
  }

  glPopMatrix();
  glutSwapBuffers();
  if (set_timeout) {
    set_timeout = 0;
    end = glutGet(GLUT_ELAPSED_TIME);
    elapsed = end - begin;
    if (elapsed > interval) {
      glutTimerFunc(0, animate, 1);
    } else {
      glutTimerFunc(interval - elapsed, animate, 1);
    }
  }
}

int width;
int height;
int depth;
unsigned char *bits;

void
visibility(int state)
{
  if (state == GLUT_VISIBLE) {
    visible = 1;
    animate(0);
  } else {
    visible = 0;
  }
}

void
minify_select(int value)
{
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
  gluBuild2DMipmaps(GL_TEXTURE_2D, depth, width, height,
    GL_RGB, GL_UNSIGNED_BYTE, bits);
  glutPostRedisplay();
}

void
rate_select(int value)
{
  interval = value;
}

void
menu_select(int value)
{
  switch (value) {
  case 1:
    spinning = !spinning;
    if (spinning)
      animate(0);
    break;
  case 2:
    scaling = !scaling;
    if (scaling)
      animate(0);
    break;
  case 3:
    mode++;
    if (mode > DRUM)
      mode = CUBE;
    switch (mode) {
    case CUBE:
      glEnable(GL_CULL_FACE);
      glDisable(GL_DEPTH_TEST);
      break;
    case SQUARES:
      glDisable(GL_CULL_FACE);
      glDisable(GL_DEPTH_TEST);
      break;
    case DRUM:
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      break;
    }
    glutPostRedisplay();
    break;
  case 666:
    exit(0);
  }
}

int
main(int argc, char **argv)
{
  int minify_menu, rate_menu;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("mjkwarp");
  glutDisplayFunc(redraw);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 70.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,30) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */
  depth = mjk_depth;
  width = mjk_width;
  height = mjk_height;
  bits = mjk_image;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  gluBuild2DMipmaps(GL_TEXTURE_2D, depth, width, height,
    GL_RGB, GL_UNSIGNED_BYTE, bits);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glEnable(GL_TEXTURE_2D);
  glutVisibilityFunc(visibility);
  minify_menu = glutCreateMenu(minify_select);
  glutAddMenuEntry("Nearest", GL_NEAREST);
  glutAddMenuEntry("Linear", GL_LINEAR);
  glutAddMenuEntry("Nearest mipmap nearest", GL_NEAREST_MIPMAP_NEAREST);
  glutAddMenuEntry("Linear mipmap nearest", GL_LINEAR_MIPMAP_NEAREST);
  glutAddMenuEntry("Nearest mipmap linear", GL_NEAREST_MIPMAP_LINEAR);
  glutAddMenuEntry("Linear mipmap linear", GL_LINEAR_MIPMAP_LINEAR);
  rate_menu = glutCreateMenu(rate_select);
  glutAddMenuEntry(" 2/sec", 500);
  glutAddMenuEntry(" 6/sec", 166);
  glutAddMenuEntry("10/sec", 100);
  glutAddMenuEntry("20/sec", 50);
  glutAddMenuEntry("30/sec", 33);
  glutAddMenuEntry("60/sec", 16);
  glutCreateMenu(menu_select);
  glutAddMenuEntry("Toggle spinning", 1);
  glutAddMenuEntry("Toggle scaling", 2);
  glutAddMenuEntry("Switch mode", 3);
  glutAddSubMenu("Minimum frame rate", rate_menu);
  glutAddSubMenu("Minify modes", minify_menu);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  menu_select(3);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
