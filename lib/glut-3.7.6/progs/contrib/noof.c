
/* XXX Very crufty code follows. */

#include <stdlib.h>
#include <GL/glut.h>

#include <math.h>
#ifndef _WIN32
#include <unistd.h>
#else
#define random rand
#define srandom srand
#include <process.h>
#endif
/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <sys/types.h>
#include <stdio.h>

/* For portability... */
#undef fcos
#undef fsin
#define fcos  cos
#define fsin  sin

/* --- shape parameters def'n --- */
#define N_SHAPES 7
float pos[N_SHAPES * 3];
float dir[N_SHAPES * 3];
float acc[N_SHAPES * 3];
float col[N_SHAPES * 3];
float hsv[N_SHAPES * 3];
float hpr[N_SHAPES * 3];
float ang[N_SHAPES];
float spn[N_SHAPES];
float sca[N_SHAPES];
float geep[N_SHAPES];
float peep[N_SHAPES];
float speedsq[N_SHAPES];
int blad[N_SHAPES];

float ht, wd;

void
initshapes(int i)
{
  int k;
  float f;

  /* random init of pos, dir, color */
  for (k = i * 3; k <= i * 3 + 2; k++) {
    f = random() / (float)RAND_MAX;
    pos[k] = f;
    f = random() / (float)RAND_MAX;
    f = (f - 0.5) * 0.05;
    dir[k] = f;
    f = random() / (float)RAND_MAX;
    f = (f - 0.5) * 0.0002;
    acc[k] = f;
    f = random() / (float)RAND_MAX;
    col[k] = f;
  }

  speedsq[i] = dir[i * 3] * dir[i * 3] + dir[i * 3 + 1] * dir[i * 3 + 1];
  f = random() / (float)RAND_MAX;
  blad[i] = 2 + (int) (f * 17.0);
  f = random() / (float)RAND_MAX;
  ang[i] = f;
  f = random() / (float)RAND_MAX;
  spn[i] = (f - 0.5) * 40.0 / (10 + blad[i]);
  f = random() / (float)RAND_MAX;
  sca[i] = (f * 0.1 + 0.08);
  dir[i * 3] *= sca[i];
  dir[i * 3 + 1] *= sca[i];

  f = random() / (float)RAND_MAX;
  hsv[i * 3] = f * 360.0;

  f = random() / (float)RAND_MAX;
  hsv[i * 3 + 1] = f * 0.6 + 0.4;

  f = random() / (float)RAND_MAX;
  hsv[i * 3 + 2] = f * 0.7 + 0.3;

  f = random() / (float)RAND_MAX;
  hpr[i * 3] = f * 0.005 * 360.0;
  f = random() / (float)RAND_MAX;
  hpr[i * 3 + 1] = f * 0.03;
  f = random() / (float)RAND_MAX;
  hpr[i * 3 + 2] = f * 0.02;

  geep[i] = 0;
  f = random() / (float)RAND_MAX;
  peep[i] = 0.01 + f * 0.2;
}

int tko = 0;

float bladeratio[] =
{
  /* nblades = 2..7 */
  0.0, 0.0, 3.00000, 1.73205, 1.00000, 0.72654, 0.57735, 0.48157,
  /* 8..13 */
  0.41421, 0.36397, 0.19076, 0.29363, 0.26795, 0.24648,
  /* 14..19 */
  0.22824, 0.21256, 0.19891, 0.18693, 0.17633, 0.16687,
};

void
drawleaf(int l)
{

  int b, blades;
  float x, y;
  float wobble;

  blades = blad[l];

  y = 0.10 * fsin(geep[l] * M_PI / 180.0) + 0.099 * fsin(geep[l] * 5.12 * M_PI / 180.0);
  if (y < 0)
    y = -y;
  x = 0.15 * fcos(geep[l] * M_PI / 180.0) + 0.149 * fcos(geep[l] * 5.12 * M_PI / 180.0);
  if (x < 0.0)
    x = 0.0 - x;
  if (y < 0.001 && x > 0.000002 && ((tko & 0x1) == 0)) {
    initshapes(l);      /* let it become reborn as something
                           else */
    tko++;
    return;
  } {
    float w1 = fsin(geep[l] * 15.3 * M_PI / 180.0);
    wobble = 3.0 + 2.00 * fsin(geep[l] * 0.4 * M_PI / 180.0) + 3.94261 * w1;
  }

  /**
  if(blades == 2) if (y > 3.000*x) y = x*3.000;
  if(blades == 3) if (y > 1.732*x) y = x*1.732;
  if(blades == 4) if (y >       x) y = x;
  if(blades == 5) if (y > 0.726*x) y = x*0.726;
  if(blades == 6) if (y > 0.577*x) y = x*0.577;
  if(blades == 7) if (y > 0.481*x) y = x*0.481;
  if(blades == 8) if (y > 0.414*x) y = x*0.414;
  */
  if (y > x * bladeratio[blades])
    y = x * bladeratio[blades];

  for (b = 0; b < blades; b++) {
    glPushMatrix();
    glTranslatef(pos[l * 3], pos[l * 3 + 1], pos[l * 3 + 2]);
    glRotatef(ang[l] + b * (360.0 / blades), 0.0, 0.0, 1.0);
    glScalef(wobble * sca[l], wobble * sca[l], wobble * sca[l]);
    /**
    if(tko & 0x40000) glColor3f(col[l*3], col[l*3+1], col[l*3+2]); 
    else
    */
    glColor4ub(0, 0, 0, 0x60);

    /* constrain geep cooridinates here XXX */
    glEnable(GL_BLEND);

    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(x * sca[l], 0.0);
    glVertex2f(x, y);
    glVertex2f(x, -y);  /* C */
    glVertex2f(0.3, 0.0);  /* D */
    glEnd();

    /**
    if(tko++ & 0x40000) glColor3f(0,0,0);
    else
    */
    glColor3f(col[l * 3], col[l * 3 + 1], col[l * 3 + 2]);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x * sca[l], 0.0);
    glVertex2f(x, y);
    glVertex2f(0.3, 0.0);  /* D */
    glVertex2f(x, -y);  /* C */
    glEnd();
    glDisable(GL_BLEND);

    glPopMatrix();
  }
}

void
motionUpdate(int t)
{
  if (pos[t * 3] < -sca[t] * wd && dir[t * 3] < 0.0) {
    dir[t * 3] = -dir[t * 3];
  /**
  acc[t*3+1] += 0.8*acc[t*3];
  acc[t*3] = -0.8*acc[t*3];
  */
  } else if (pos[t * 3] > (1 + sca[t]) * wd && dir[t * 3] > 0.0) {
    dir[t * 3] = -dir[t * 3];
    /**
    acc[t*3+1] += 0.8*acc[t*3];
    acc[t*3] = -0.8*acc[t*3];
    */
  } else if (pos[t * 3 + 1] < -sca[t] * ht && dir[t * 3 + 1] < 0.0) {
    dir[t * 3 + 1] = -dir[t * 3 + 1];
    /**
    acc[t*3] += 0.8*acc[t*3+1];
    acc[t*3+1] = -0.8*acc[t*3+1];
    */
  } else if (pos[t * 3 + 1] > (1 + sca[t]) * ht && dir[t * 3 + 1] > 0.0) {
    dir[t * 3 + 1] = -dir[t * 3 + 1];
    /**
    acc[t*3] += 0.8*acc[t*3+1];
    acc[t*3+1] = -0.8*acc[t*3+1];
    */
  }

  pos[t * 3] += dir[t * 3];
  pos[t * 3 + 1] += dir[t * 3 + 1];
  /**
  dir[t*3]   += acc[t*3];
  dir[t*3+1] += acc[t*3+1];
  */
  ang[t] += spn[t];
  geep[t] += peep[t];
  if (geep[t] > 360 * 5.0)
    geep[t] -= 360 * 5.0;
  if (ang[t] < 0.0) {
    ang[t] += 360.0;
  }
  if (ang[t] > 360.0) {
    ang[t] -= 360.0;
  }
}

void
colorUpdate(int i)
{
  if (hsv[i * 3 + 1] <= 0.5 && hpr[i * 3 + 1] < 0.0)
    hpr[i * 3 + 1] = -hpr[i * 3 + 1];  /* adjust s */
  if (hsv[i * 3 + 1] >= 1.0 && hpr[i * 3 + 1] > 0.0)
    hpr[i * 3 + 1] = -hpr[i * 3 + 1];  /* adjust s */
  if (hsv[i * 3 + 2] <= 0.4 && hpr[i * 3 + 2] < 0.0)
    hpr[i * 3 + 2] = -hpr[i * 3 + 2];  /* adjust s */
  if (hsv[i * 3 + 2] >= 1.0 && hpr[i * 3 + 2] > 0.0)
    hpr[i * 3 + 2] = -hpr[i * 3 + 2];  /* adjust s */

  hsv[i * 3] += hpr[i * 3];
  hsv[i * 3 + 1] += hpr[i * 3 + 1];
  hsv[i * 3 + 2] += hpr[i * 3 + 2];

  /* --- hsv -> rgb --- */
#define H(hhh) hhh[i*3  ]
#define S(hhh) hhh[i*3+1]
#define V(hhh) hhh[i*3+2]

#define R(hhh) hhh[i*3  ]
#define G(hhh) hhh[i*3+1]
#define B(hhh) hhh[i*3+2]

  if (V(hsv) < 0.0)
    V(hsv) = 0.0;
  if (V(hsv) > 1.0)
    V(hsv) = 1.0;
  if (S(hsv) <= 0.0) {
    R(col) = V(hsv);
    G(col) = V(hsv);
    B(col) = V(hsv);
  } else {
    float f, h, p, q, t, v;
    int hi;

    while (H(hsv) < 0.0)
      H(hsv) += 360.0;
    while (H(hsv) >= 360.0)
      H(hsv) -= 360.0;

    if (S(hsv) < 0.0)
      S(hsv) = 0.0;
    if (S(hsv) > 1.0)
      S(hsv) = 1.0;

    h = H(hsv) / 60.0;
    hi = (int) (h);
    f = h - hi;
    v = V(hsv);
    p = V(hsv) * (1 - S(hsv));
    q = V(hsv) * (1 - S(hsv) * f);
    t = V(hsv) * (1 - S(hsv) * (1 - f));

    if (hi <= 0) {
      R(col) = v;
      G(col) = t;
      B(col) = p;
    } else if (hi == 1) {
      R(col) = q;
      G(col) = v;
      B(col) = p;
    } else if (hi == 2) {
      R(col) = p;
      G(col) = v;
      B(col) = t;
    } else if (hi == 3) {
      R(col) = p;
      G(col) = q;
      B(col) = v;
    } else if (hi == 4) {
      R(col) = t;
      G(col) = p;
      B(col) = v;
    } else {
      R(col) = v;
      G(col) = p;
      B(col) = q;
    }
  }
}

void
gravity(float fx)
{
  int a, b;

  for (a = 0; a < N_SHAPES; a++) {
    for (b = 0; b < a; b++) {
      float t, d2;

      t = pos[b * 3] - pos[a * 3];
      d2 = t * t;
      t = pos[b * 3 + 1] - pos[a * 3 + 1];
      d2 += t * t;
      if (d2 < 0.000001)
        d2 = 0.00001;
      if (d2 < 0.1) {

        float v0, v1, z;
        v0 = pos[b * 3] - pos[a * 3];
        v1 = pos[b * 3 + 1] - pos[a * 3 + 1];

        z = 0.00000001 * fx / (d2);

        dir[a * 3] += v0 * z * sca[b];
        dir[b * 3] += -v0 * z * sca[a];
        dir[a * 3 + 1] += v1 * z * sca[b];
        dir[b * 3 + 1] += -v1 * z * sca[a];

      }
    }
    /** apply brakes
    if(dir[a*3]*dir[a*3] + dir[a*3+1]*dir[a*3+1]
      > 0.0001) {
      dir[a*3] *= 0.9;
      dir[a*3+1] *= 0.9;
    }
    */
  }
}
void
oneFrame(void)
{
  int i;

  /**
  if((random() & 0xff) == 0x34){
    glClear(GL_COLOR_BUFFER_BIT);
  }

  if((tko & 0x1f) == 0x1f){
    glEnable(GL_BLEND);
    glColor4f(0.0, 0.0, 0.0, 0.09);
    glRectf(0.0, 0.0, wd, ht);
    glDisable(GL_BLEND);
#ifdef __sgi
    sginap(0);
#endif
  }
  */
  gravity(-2.0);
  for (i = 0; i < N_SHAPES; i++) {
    motionUpdate(i);
#ifdef __sgi
    sginap(0);
#endif
    colorUpdate(i);
#ifdef __sgi
    sginap(0);
#endif
    drawleaf(i);
#ifdef __sgi
    sginap(0);
#endif

  }
  glFlush();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
}

void
myReshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (w <= h) {
    wd = 1.0;
    ht = (GLfloat) h / (GLfloat) w;
    glOrtho(0.0, 1.0,
      0.0, 1.0 * (GLfloat) h / (GLfloat) w,
      -16.0, 4.0);
  } else {
    wd = (GLfloat) w / (GLfloat) h;
    ht = 1.0;
    glOrtho(0.0, 1.0 * (GLfloat) w / (GLfloat) h,
      0.0, 1.0,
      -16.0, 4.0);
  }
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void
visibility(int status)
{
  if (status == GLUT_VISIBLE) {
    glutIdleFunc(oneFrame);
  } else {
    glutIdleFunc(NULL);
  }

}

void
myinit(void)
{
  int i;
  srandom(getpid());
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glEnable(GL_LINE_SMOOTH);
  glShadeModel(GL_FLAT);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (i = 0; i < N_SHAPES; i++)
    initshapes(i);
  myReshape(200, 200);
}

/* ARGSUSED1 */
void
keys(unsigned char c, int x, int y)
{

  if (c == 0x1b)
    exit(0);            /* escape */
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(300, 300);
  glutCreateWindow(argv[0]);

  myinit();
  glutReshapeFunc(myReshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keys);
  glutVisibilityFunc(visibility);
  glutIdleFunc(oneFrame);
  glutPostRedisplay();
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
