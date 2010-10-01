
/* Copyright (c) Mark J. Kilgard, 1997, 1998. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void initWindow(void);

float angle = 0.0;
int left, right;
int leftTime, rightTime;
int thrust, thrustTime;
int joyThrust = 0, joyLeft = 0, joyRight = 0;
float x = 20, y = 20, xv, yv, v;
int shield = 0, joyShield = 0, cursor = 1;
int lastTime;
int paused = 0;
int resuming = 1;
int originalWindow = 0, currentWindow;

typedef struct {
  int inuse;

  float x;
  float y;

  float v;
  float xv;
  float yv;

  int expire;
} Bullet;

#define MAX_BULLETS 10

Bullet bullet[MAX_BULLETS];

int
allocBullet(void)
{
  int i;

  for (i=0; i<MAX_BULLETS; i++) {
    if (!bullet[i].inuse) {
      return i;
    }
  }
  return -1;
}

void
initBullet(int i, int time)
{
  float c = cos(angle*M_PI/180.0);
  float s = sin(angle*M_PI/180.0);

  bullet[i].inuse = 1;
  bullet[i].x = x + 2 * c;
  bullet[i].y = y + 2 * s;
  bullet[i].v = 0.025;
  bullet[i].xv = xv + c * bullet[i].v;
  bullet[i].yv = yv + s * bullet[i].v;
  bullet[i].expire = time + 1000;
}

void
advanceBullets(int delta, int time)
{
  int i;

  for (i=0; i<MAX_BULLETS; i++) {
    if (bullet[i].inuse) {
      float x, y;

      if (time > bullet[i].expire) {
	bullet[i].inuse = 0;
	continue;
      }
      x = bullet[i].x + bullet[i].xv * delta;
      y = bullet[i].y + bullet[i].yv * delta;
      x = x / 40.0;
      bullet[i].x = (x - floor(x))*40.0;
      y = y / 40.0;
      bullet[i].y = (y - floor(y))*40.0;
    }
  }
}

void
shotBullet(void)
{
  int entry;

  entry = allocBullet();
  if (entry >= 0) {
    initBullet(entry, glutGet(GLUT_ELAPSED_TIME));
  }
}

void
drawBullets(void)
{
  int i;

  glBegin(GL_POINTS);
  glColor3f(1.0, 0.0, 1.0);
  for (i=0; i<MAX_BULLETS; i++) {
    if (bullet[i].inuse) {
      glVertex2f(bullet[i].x, bullet[i].y);
    }
  }
  glEnd();
}

void
drawShip(float angle)
{
  float rad;

  glPushMatrix();
  glTranslatef(x, y, 0.0);
  glRotatef(angle, 0.0, 0.0, 1.0);
  if (thrust) {
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.75, -0.5);
    glVertex2f(-1.75, 0);
    glVertex2f(-0.75, 0.5);
    glEnd();
  }
  glColor3f(1.0, 1.0, 0.0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(2.0, 0.0);
  glVertex2f(-1.0, -1.0);
  glVertex2f(-0.5, 0.0);
  glVertex2f(-1.0, 1.0);
  glVertex2f(2.0, 0.0);
  glEnd();
  if (shield) {
    glColor3f(0.1, 0.1, 1.0);
    glBegin(GL_LINE_LOOP);
    for (rad=0.0; rad<12.0; rad += 1.0) {
      glVertex2f(2.3 * cos(2*rad/M_PI)+0.2, 2.0 * sin(2*rad/M_PI));
    }
    glEnd();
  }
  glPopMatrix();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  drawShip(angle);
  drawBullets();
  glutSwapBuffers();
}

void
idle(void)
{
  int time, delta;

  time = glutGet(GLUT_ELAPSED_TIME);
  if (resuming) {
    lastTime = time;
    resuming = 0;
  }
  if (left) {
    delta = time - leftTime;
    angle = angle + delta * 0.4;
    leftTime = time;
  }
  if (right) {
    delta = time - rightTime;
    angle = angle - delta * 0.4;
    rightTime = time;
  }
  if (thrust) {
    delta = time - thrustTime;
    v = delta * 0.00004;
    xv = xv + cos(angle*M_PI/180.0) * v;
    yv = yv + sin(angle*M_PI/180.0) * v;
    thrustTime = time;
  }
  delta = time - lastTime;
  x = x + xv * delta;
  y = y + yv * delta;
  x = x / 40.0;
  x = (x - floor(x))*40.0;
  y = y / 40.0;
  y = (y - floor(y))*40.0;
  lastTime = time;
  advanceBullets(delta, time);
  glutPostWindowRedisplay(currentWindow);
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (!paused) {
      glutIdleFunc(idle);
    }
  } else {
    glutIdleFunc(NULL);
  }
}

/* ARGSUSED1 */
void
key(unsigned char key, int px, int py)
{
  switch (key) {
  case 27:
    exit(0);
    break;
  case 'A':
  case 'a':
    thrust = 1;
    thrustTime = glutGet(GLUT_ELAPSED_TIME);
    break;
  case 'S':
  case 's':
    shield = 1;
    break;
  case 'C':
  case 'c':
    cursor = !cursor;
    glutSetCursor(
      cursor ? GLUT_CURSOR_INHERIT : GLUT_CURSOR_NONE);
    break;
  case 'z':
  case 'Z':
    x = 20;
    y = 20;
    xv = 0;
    yv = 0;
    break;
  case 'f':
    glutGameModeString("400x300:16@60");
    glutEnterGameMode();
    initWindow();
    break;
  case 'g':
    glutGameModeString("640x480:16@60");
    glutEnterGameMode();
    initWindow();
    break;
  case 'l':
    if (originalWindow != 0 && currentWindow != originalWindow) {
      glutLeaveGameMode();
      currentWindow = originalWindow;
    }
    break;
  case 'P':
  case 'p':
    paused = !paused;
    if (paused) {
      glutIdleFunc(NULL);
    } else {
      glutIdleFunc(idle);
      resuming = 1;
    }
    break;
  case 'Q':
  case 'q':
  case ' ':
    shotBullet();
    break;
  }
}

/* ARGSUSED1 */
void
keyup(unsigned char key, int x, int y)
{
  switch (key) {
  case 'A':
  case 'a':
    thrust = 0;
    break;
  case 'S':
  case 's':
    shield = 0;
    break;
  }
}

/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_F1:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    break;
  case GLUT_KEY_F2:
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    break;
  case GLUT_KEY_UP:
    thrust = 1;
    thrustTime = glutGet(GLUT_ELAPSED_TIME);
    break;
  case GLUT_KEY_LEFT:
    left = 1;
    leftTime = glutGet(GLUT_ELAPSED_TIME);
    break;
  case GLUT_KEY_RIGHT:
    right = 1;
    rightTime = glutGet(GLUT_ELAPSED_TIME);
    break;
  }
}

/* ARGSUSED1 */
void
specialup(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_UP:
    thrust = 0;
    break;
  case GLUT_KEY_LEFT:
    left = 0;
    break;
  case GLUT_KEY_RIGHT:
    right = 0;
    break;
  }
}

/* ARGSUSED3 */
void
joystick(unsigned int buttons, int x, int y, int z)
{
  if (buttons & 0x1) {
    thrust = 1;
    thrustTime = glutGet(GLUT_ELAPSED_TIME);
    joyThrust = 1;
  } else {
    if (joyThrust) {
      thrust = 0;
      joyThrust = 0;
    }
  }
  if (buttons & 0x2) {
    shotBullet();
  }
  if (buttons & 0x4) {
    shield = 1;
    joyShield = 1;
  } else {
    if (joyShield) {
      shield = 0;
      joyShield = 0;
    }
  }
  if (x < -300) {
    left = 1;
    leftTime = glutGet(GLUT_ELAPSED_TIME);
    joyLeft = 1;
  } else {
    /* joyLeft helps avoid "joystick in neutral"
       from continually stopping rotation. */
    if (joyLeft) {
      left = 0;
      joyLeft = 0;
    }
  }
  if (x > 300) {
    right = 1;
    rightTime = glutGet(GLUT_ELAPSED_TIME);
    joyRight = 1;
  } else {
    /* joyRight helps avoid "joystick in neutral"
       from continually stopping rotation. */
    if (joyRight) {
      right = 0;
      joyRight = 0;
    }
  }
}

void
initWindow(void)
{
  glutIgnoreKeyRepeat(1);

  glutDisplayFunc(display);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(key);
  glutKeyboardUpFunc(keyup);
  glutSpecialFunc(special);
  glutSpecialUpFunc(specialup);
  glutJoystickFunc(joystick, 100);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 40, 0, 40, 0, 40);
  glMatrixMode(GL_MODELVIEW); 
  glPointSize(3.0);

  currentWindow = glutGetWindow();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  if (argc > 1 && !strcmp(argv[1], "-fullscreen")) {
    glutGameModeString("640x480:16@60");
    glutEnterGameMode();
  } else {
    originalWindow = glutCreateWindow("asteroids");
  }

  initWindow();

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
