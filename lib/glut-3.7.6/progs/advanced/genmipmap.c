
/* genmipmap.c - by David Blythe, SGI */

/* Example of how to generate texture mipmap levels with the
   accumulation buffer. */

/* Usage example: genmipmap [file.rgb] */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"

static int w = 512, h = 512;
static int pause;

void 
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, 0, h, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void 
init_textures(char *filename)
{
  unsigned *buf;
  int width, height, components;

  if (filename) {
    buf = read_texture(filename, &width, &height, &components);
    if (buf == NULL) {
      fprintf(stderr, "Error: Can't load image file \"%s\".\n",
        filename);
      exit(1);
    } else {
      printf("%d x %d texture loaded\n", width, height);
    }
  } else {
    int i, j;
    GLubyte *p;
    components = 4;
    width = height = 512;
    buf = (unsigned *) malloc(width * height * sizeof(unsigned));
    p = (GLubyte *) buf;
    for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) {
        if (i & 1)
          p[4 * (i + j * width) + 0] = 0xff;
        else
          p[4 * (i + j * width) + 1] = 0xff;
        if (j & 1)
          p[4 * (i + j * width) + 2] = 0xff;
      }
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, components, width,
    height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
    buf);
  free(buf);
}

void 
init(char *filename)
{
  glEnable(GL_TEXTURE_2D);
  init_textures(filename);
}

void 
draw_rect(int w, int h)
{
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(0, 0);
  glTexCoord2f(1, 0);
  glVertex2f(w, 0);
  glTexCoord2f(1, 1);
  glVertex2f(w, h);
  glTexCoord2f(0, 1);
  glVertex2f(0, h);
  glEnd();
}

void 
stop(void)
{
  if (pause) {
    printf("? ");
    fflush(stdout);
    getchar();
  }
}

void 
acfilter(int width, int height)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
  glMatrixMode(GL_TEXTURE);

  if (pause) {
    draw_rect(width, height);
    stop();
    glClear(GL_COLOR_BUFFER_BIT);
  }
  draw_rect(width / 2, height / 2);
  stop();
  glAccum(GL_ACCUM, 0.25);

  glTranslatef(1.0 / width, 0, 0);
  draw_rect(width / 2, height / 2);
  stop();
  glAccum(GL_ACCUM, 0.25);

  glLoadIdentity();
  glTranslatef(0, 1.0 / height, 0);
  draw_rect(width / 2, height / 2);
  stop();
  glAccum(GL_ACCUM, 0.25);

  glLoadIdentity();
  glTranslatef(1.0 / width, 1.0 / height, 0);
  draw_rect(width / 2, height / 2);
  stop();
  glAccum(GL_ACCUM, 0.25);

  glAccum(GL_RETURN, 1.0);
  glMatrixMode(GL_MODELVIEW);
}

void 
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  draw_rect(w, h);
  acfilter(w, h);
  glFlush();
}

void 
help(void)
{
  printf("'h'   - help\n");
  printf("'s'   - toggle single step mode\n");
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 'h':
    help();
    break;
  case 's':
    pause ^= 1;
    break;
  case '\033':
    exit(0);
    break;
  default:
    glutPostRedisplay();
  }
}

int 
main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(w, h);
  glutInitDisplayMode(GLUT_RGBA | GLUT_ACCUM);
  (void) glutCreateWindow("genmipmap");
  if (argc > 1)
    init(argv[1]);
  else
    init(0);
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutReshapeFunc(reshape);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
