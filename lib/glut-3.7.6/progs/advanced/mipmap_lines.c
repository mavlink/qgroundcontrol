
/* mipmap_lines.c - by David Blythe, SGI */

/* Different mipmap filters. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include "texture.h"
#include "izoom.h"

static int w = 1024, h = 512;

void 
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, (GLfloat) w / (GLfloat) h, 1.0, 20.0);

  gluLookAt(0, 1, 3,
    0, 0, 0,
    0, 1, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

int original_width, reduced_width, global_comp;
unsigned *original, *reduced;

void 
getimgrow(short *buf, int y)
{
  int i;
  unsigned *p = &original[y * original_width];
  int shift = global_comp * 8;
  unsigned int mask = 0xff << shift;

  for (i = 0; i < original_width; i++) {
    buf[i] = (p[i] & mask) >> shift;
  }
}

void 
putimgrow(short *buf, int y)
{
  int i;
  unsigned *p = &reduced[y * reduced_width];
  int shift = global_comp * 8;
  unsigned int mask = 0xff << shift;

  for (i = 0; i < reduced_width; i++) {
    p[i] = (p[i] & ~mask) | (buf[i] << shift);
  }
}

void 
buildMitchellMipmaps(int components, int width, int height, unsigned
  *buf)
{
  int level = 0;

  original_width = width;
  original = buf;
  glTexImage2D(GL_TEXTURE_2D, level, components, width,
    height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
    original);
  while (width) {
    reduced_width = width / 2;
    for (global_comp = 0; global_comp < 4; global_comp++) {
      filterzoom(getimgrow, putimgrow, width, height, width / 2,
        height / 2, MITCHELL, 1.);
    }
    glTexImage2D(GL_TEXTURE_2D, ++level, components, width / 2,
      height / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE,
      reduced);
    width /= 2, height /= 2;
    memcpy(original, reduced, width * height * sizeof(unsigned));
    original_width = width;
    printf("build level %d\n", level);
  }
}

int width = 256, height = 256;
int grid_space = 4;

void 
init_textures(char *filename)
{
  unsigned *buf;
  int components;

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
    components = 4;
    buf = (unsigned *) malloc(width * height * sizeof(unsigned));
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
        if ((i % grid_space) && (j % grid_space))
          buf[i * width + j] = 0xffffffff;
        else
          buf[i * width + j] = 0;
  }

#ifdef GL_EXT_texture_object
  glBindTextureEXT(GL_TEXTURE_2D, 1);
#else
  glNewList(1001, GL_COMPILE);
#endif
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gluBuild2DMipmaps(GL_TEXTURE_2D, components, width, height, GL_RGBA,
    GL_UNSIGNED_BYTE, buf);
#ifndef GL_EXT_texture_object
  glEndList();
#endif

#ifdef GL_EXT_texture_object
  glBindTextureEXT(GL_TEXTURE_2D, 2);
#else
  glNewList(1002, GL_COMPILE);
#endif
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  reduced = (unsigned *) malloc(width * height * sizeof(unsigned));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  buildMitchellMipmaps(components, width, height, buf);
#ifndef GL_EXT_texture_object
  glEndList();
#endif

  free(buf);
  free(reduced);
}

void 
init(char *filename)
{
  glEnable(GL_TEXTURE_2D);
  init_textures(filename);
  glNewList(1, GL_COMPILE);
  glColor3f(1., 1., 1.);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 1);
  glVertex3f(-4, 0, -10);
  glTexCoord2f(1, 1);
  glVertex3f(4, 0, -10);
  glTexCoord2f(1, 0);
  glVertex3f(4, 0, 3);
  glTexCoord2f(0, 0);
  glVertex3f(-4, 0, 3);
  glEnd();
  glEndList();
  glClearColor(.2, 0, .9, 0);
}

void 
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(0, 0, w / 2, h);
#ifdef GL_EXT_texture_object
  glBindTextureEXT(GL_TEXTURE_2D, 1);
#else
  glCallList(1001);
#endif
  glCallList(1);

  glViewport(w / 2, 0, w / 2, h);
#ifdef GL_EXT_texture_object
  glBindTextureEXT(GL_TEXTURE_2D, 2);
#else
  glCallList(1002);
#endif
  glCallList(1);
  glutSwapBuffers();
}

void 
idle(void)
{
  glRotatef(.1, 1, 0, 0);
  glutPostRedisplay();
}

void 
bidle(void)
{
  glRotatef(-.1, 1, 0, 0);
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
mouse(int button, int state, int x, int y)
{
  if (state == GLUT_DOWN) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      glutIdleFunc(idle);
      break;
    case GLUT_MIDDLE_BUTTON:
      glutIdleFunc(bidle);
      break;
    case GLUT_RIGHT_BUTTON:
      break;
    }
  } else {
    glutIdleFunc(NULL);
  }
}

void 
help(void)
{
  printf("'h'   - help\n");
  printf("'g'   - increase line spacing\n");
  printf("'G'   - decrease line spacing\n");
  printf("'s'   - double texture dimensions\n");
  printf("'S'   - halve texture dimensions\n");
}

char *filename;

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 'h':
    help();
    break;
  case 'g':
    grid_space++;
    init_textures(filename);
    printf("grid spacing %d\n", grid_space);
    break;
  case 'G':
    grid_space--;
    if (grid_space <= 0)
      grid_space = 1;
    init_textures(filename);
    printf("grid spacing %d\n", grid_space);
    break;
  case 's':
    height = width *= 2;
    if (height > 1024)
      height = width = 1024;
    init_textures(filename);
    printf("texture size %d\n", height);
    break;
  case 'S':
    height = width /= 2;
    init_textures(filename);
    printf("texture size %d\n", height);
    break;
  default:
    return;
  case '\033':
    exit(0);
    break;
  }
  glutPostRedisplay();
}

int 
main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(w, h);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  (void) glutCreateWindow("Left: GLU mipmaps, Right: Mitchell mipmaps");
  if (argc > 1)
    init(filename = argv[1]);
  else
    init(filename = 0);
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
