
/* textrim.c - by David Blythe, SGI */

/* Trimming textures: demonstrates how alpha blending or alpha testing
   can be used to "trim" the shape of textures to arbitrary shapes. 
   Alpha testing is generally cheaper than alpha blending, but
   blending permits antialiased edges. */

/* Try: "textrim tree.rgb" where tree.rgb is a SGI .rgb file including
   an alpha component. */
   
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"

static float scale = 1.4;
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2

void
pan(int x, int y)
{
  transx += (x - ox) / 500.;
  transy -= (y - oy) / 500.;
  ox = x;
  oy = y;
  glutPostRedisplay();
}

void
rotate(int x, int y)
{
  rotx += x - ox;
  if (rotx > 360.)
    rotx -= 360.;
  else if (rotx < -360.)
    rotx += 360.;
  roty += y - oy;
  if (roty > 360.)
    roty -= 360.;
  else if (roty < -360.)
    roty += 360.;
  ox = x;
  oy = y;
  glutPostRedisplay();
}

void
motion(int x, int y)
{
  if (mot == PAN)
    pan(x, y);
  else if (mot == ROT)
    rotate(x, y);
}

void
mouse(int button, int state, int x, int y)
{
  if (state == GLUT_DOWN) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      mot = PAN;
      motion(ox = x, oy = y);
      break;
    case GLUT_MIDDLE_BUTTON:
      mot = ROT;
      motion(ox = x, oy = y);
      break;
    }
  } else if (state == GLUT_UP) {
    mot = 0;
  }
}

void 
afunc(void)
{
  static int state;
  if (state ^= 1) {
    glAlphaFunc(GL_GREATER, .01);
    glEnable(GL_ALPHA_TEST);
  } else {
    glDisable(GL_ALPHA_TEST);
  }
}

void 
bfunc(void)
{
  static int state;
  if (state ^= 1) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  } else {
    glDisable(GL_BLEND);
  }
}

void 
up(void)
{
  scale += .1;
}

void 
down(void)
{
  scale -= .1;
}

void 
help(void)
{
  printf("Usage: textrim [image]\n");
  printf("'h'            - help\n");
  printf("'a'            - toggle alpha test\n");
  printf("'b'            - toggle blend\n");
  printf("'UP'           - scale up\n");
  printf("'DOWN'         - scale down\n");
  printf("left mouse     - pan\n");
  printf("middle mouse   - rotate\n");
}

void 
init(char *filename)
{
  static unsigned *image;
  static int width, height, components;
  if (filename) {
    image = read_texture(filename, &width, &height, &components);
    if (image == NULL) {
      fprintf(stderr, "Error: Can't load image file \"%s\".\n",
        filename);
      exit(1);
    } else {
      printf("%d x %d image loaded\n", width, height);
    }
    if (components != 4) {
      printf("must be an RGBA image\n");
      exit(1);
    }
  } else {
    int i, j;
    unsigned char *img;
    components = 4;
    width = height = 512;
    image = (unsigned *) malloc(width * height * sizeof(unsigned));
    img = (unsigned char *) image;
    for (j = 0; j < height; j++)
      for (i = 0; i < width; i++) {
        int w2 = width / 2, h2 = height / 2;
        if (i & 32)
          img[4 * (i + j * width) + 0] = 0xff;
        else
          img[4 * (i + j * width) + 1] = 0xff;
        if (j & 32)
          img[4 * (i + j * width) + 2] = 0xff;
        if ((i - w2) * (i - w2) + (j - h2) * (j - h2) > 64 * 64 &&
          (i - w2) * (i - w2) + (j - h2) * (j - h2) < 300 * 300)
          img[4 * (i + j * width) + 3] = 0xff;
      }

  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, components, width,
    height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
    image);
  glEnable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50., 1., .1, 10.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0., 0., -5.5);
  glClearColor(.25f, .25f, .25f, .25f);
}

void 
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glPushMatrix();
  glTranslatef(transx, transy, 0.f);
  glRotatef(rotx, 0., 1., 0.);
  glRotatef(roty, 1., 0., 0.);
  glScalef(scale, scale, 0.);
  glBegin(GL_POLYGON);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(-1.0, -1.0);
  glTexCoord2f(1.0, 0.0);
  glVertex2f(1.0, -1.0);
  glTexCoord2f(1.0, 1.0);
  glVertex2f(1.0, 1.0);
  glTexCoord2f(0.0, 1.0);
  glVertex2f(-1.0, 1.0);
  glEnd();
  glPopMatrix();
  glutSwapBuffers();
}

void 
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 'A':
  case 'a':
    afunc();
    break;
  case 'B':
  case 'b':
    bfunc();
    break;
  case 'H':
  case 'h':
    help();
    break;
  case '\033':
    exit(0);
    break;
  default:
    break;
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_UP:
    up();
    break;
  case GLUT_KEY_DOWN:
    down();
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

void
menu(int value)
{
  key((unsigned char) value, 0, 0);
}

int 
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  (void) glutCreateWindow("textrim");
  init(argv[1]);
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutCreateMenu(menu);
  glutAddMenuEntry("Toggle alpha testing", 'a');
  glutAddMenuEntry("Toggle alpha blending", 'b');
  glutAddMenuEntry("Quit", '\033');
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
