#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

static char defaultFile[] = "../data/mandrill.rgb";
GLuint *img;
GLsizei w, h;
GLint comp;

GLfloat scale[] = {1, 1, 1}, bias[] = {0, 0, 0};
GLboolean changeScale = 1, changeBias = 1;
GLboolean changeR = 1, changeG = 1, changeB = 1;

void init(void)
{
  glDrawBuffer(GL_FRONT);
  glReadBuffer(GL_BACK);
}

void load_img(const char *fname)
{
  img = read_texture(fname, &w, &h, &comp);
  if (!img) {
    fprintf(stderr, "Could not open %s\n", fname);
    exit(1);
  }
}

void reshape(GLsizei winW, GLsizei winH) 
{
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(0, winW, 0, winH, 0, 5);
}

void draw(void)
{
    GLenum err;

    glPixelTransferf(GL_RED_SCALE, 1);
    glPixelTransferf(GL_GREEN_SCALE, 1);
    glPixelTransferf(GL_BLUE_SCALE, 1);
    glPixelTransferf(GL_RED_BIAS, 0);
    glPixelTransferf(GL_GREEN_BIAS, 0);
    glPixelTransferf(GL_BLUE_BIAS, 0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffer(GL_BACK);
    glRasterPos2i(0, 0);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);


    glPixelTransferf(GL_RED_SCALE, scale[0]);
    glPixelTransferf(GL_GREEN_SCALE, scale[1]);
    glPixelTransferf(GL_BLUE_SCALE, scale[2]);
    glPixelTransferf(GL_RED_BIAS, bias[0]);
    glPixelTransferf(GL_GREEN_BIAS, bias[1]);
    glPixelTransferf(GL_BLUE_BIAS, bias[2]);
    glDrawBuffer(GL_FRONT);
    glCopyPixels(0, 0, w, h, GL_COLOR);

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  char change[][30] = {"Not changing", "Changing"};

  switch(key) {
  case 27:
    exit(0);
  case 's':  case 'S':
    changeScale = (changeScale == 0);
    printf("%s scale\n", change[changeScale]);
    break;
  case 'i':  case 'I':
    changeBias = (changeBias == 0);
    printf("%s bias\n", change[changeBias]);
    break;    
  case 'r':  case 'R':
    changeR = (changeR == 0);
    printf("%s red channel\n", change[changeR]);
    break;
  case 'g':  case 'G':
    changeG = (changeG == 0);
    printf("%s green channel\n", change[changeG]);
    break;
  case 'b':  case 'B':
    changeB = (changeB == 0);
    printf("%s blue channel\n", change[changeB]);
    break;
  case ' ':
    changeScale = changeBias = changeR = changeG = changeB = 1;
    scale[0] = scale[1] = scale[2] = 1;
    bias[0] = bias[1] = bias[2] = 0;
    printf("Resetting all\n");
    draw();
    break;
  case '?':
    printf("Scale:\n");
    printf("\tR:  %f\n", scale[0]);
    printf("\tG:  %f\n", scale[1]);
    printf("\tB:  %f\n", scale[2]);
    printf("Bias:\n");
    printf("\tR:  %f\n", bias[0]);
    printf("\tG:  %f\n", bias[1]);
    printf("\tB:  %f\n\n", bias[2]);
  }
}

int lastX, lastY, curX, curY;

void idle(void)
{
  float dScale, dBias;

  if (lastX != curX || lastY != curY) {
    if (changeScale) {
      dScale = (curX - lastX) / (float)w;
      if (changeR) scale[0] += dScale;
      if (changeG) scale[1] += dScale;
      if (changeB) scale[2] += dScale;
    }
    if (changeBias) {
      dBias = (curY - lastY) / (float)h;
      if (changeR) bias[0] += dBias;
      if (changeG) bias[1] += dBias;
      if (changeB) bias[2] += dBias;
    }      

    glPixelTransferf(GL_RED_SCALE, scale[0]);
    glPixelTransferf(GL_GREEN_SCALE, scale[1]);
    glPixelTransferf(GL_BLUE_SCALE, scale[2]);
    glPixelTransferf(GL_RED_BIAS, bias[0]);
    glPixelTransferf(GL_GREEN_BIAS, bias[1]);
    glPixelTransferf(GL_BLUE_BIAS, bias[2]);

    glRasterPos2i(0, 0);
    glCopyPixels(0, 0, w, h, GL_COLOR);

    lastX = curX;
    lastY = curY;
  }
}

void motion(int xpos, int ypos)
{
  curX = xpos;
  curY = (h - ypos);
}

/* ARGSUSED */
void button(int button, int state, int xpos, int ypos)
{
  if (state == GLUT_DOWN) {
    glutIdleFunc(idle); 
    curX = lastX = xpos;
    curY = lastY = (h - ypos);
    return;
  } else {
    glutIdleFunc(0);
  }
}

main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    if (argc > 1) {
      load_img(argv[1]);
    } else {
      load_img(defaultFile);
    }
    glutInitWindowSize(w, h);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(button);
    glutMotionFunc(motion);
    init();

    glutMainLoop();
    return 0;
}

