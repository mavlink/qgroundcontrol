
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

#define MAX_SPLATS 100

extern int logo_width;
extern int logo_height;
extern unsigned char logo_image[];

typedef struct _SplatInfo {
  int x, y;
  GLboolean alphaTest;
  GLfloat xScale, yScale;
  GLfloat scale[3];
  GLfloat bias[3];
} SplatInfo;

int winHeight;
int numSplats = 0;
SplatInfo splatConfig;
SplatInfo splatList[MAX_SPLATS];
SplatInfo splatDefault = {
  0, 0,
  GL_TRUE,
  1.0, 1.0,
  { 1.0, 1.0, 1.0 },
  { 0.0, 0.0, 0.0 }
};

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
  winHeight = h;
}

void
renderSplat(SplatInfo *splat)
{
    glRasterPos2i(splat->x, splat->y);
    if(splat->yScale >= 0)
      glBitmap(0, 0, 0, 0, 0, -logo_height * splat->yScale, 0);
    if(splat->xScale < 0)
      glBitmap(0, 0, 0, 0, logo_width * -splat->xScale, 0, 0);
    glPixelZoom(splat->xScale, splat->yScale);
    glPixelTransferf(GL_RED_SCALE, splat->scale[0]);
    glPixelTransferf(GL_GREEN_SCALE, splat->scale[1]);
    glPixelTransferf(GL_BLUE_SCALE, splat->scale[2]);
    glPixelTransferf(GL_RED_BIAS, splat->bias[0]);
    glPixelTransferf(GL_GREEN_BIAS, splat->bias[1]);
    glPixelTransferf(GL_BLUE_BIAS, splat->bias[2]);
    if (splat->alphaTest) 
      glEnable(GL_ALPHA_TEST);
    else
      glDisable(GL_ALPHA_TEST);
    glDrawPixels(logo_width, logo_height, GL_RGBA,
      GL_UNSIGNED_BYTE, logo_image);
}

void
display(void)
{
  int i;

  glClear(GL_COLOR_BUFFER_BIT);
  for (i = 0; i < numSplats; i++) {
    renderSplat(&splatList[i]);
  }
  glFlush();
}

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      if (numSplats < MAX_SPLATS) {
        splatConfig.x = x;
        splatConfig.y = winHeight - y;
	renderSplat(&splatConfig);
        splatList[numSplats] = splatConfig;
        numSplats++;
      } else {
        printf("out of splats!\n");
      }
    }
  }
}

void
mainSelect(int value)
{
  GLfloat rpos[4];
  GLboolean valid;

  switch(value) {
  case 0:
    numSplats = 0;
    glutPostRedisplay();
    break;
  case 1:
    splatConfig = splatDefault;
    break;
  case 2:
    splatConfig.xScale *= 1.25;
    splatConfig.yScale *= 1.25;
    break;
  case 3:
    splatConfig.xScale *= 0.75;
    splatConfig.yScale *= 0.75;
    break;
  case 4:
    splatConfig.xScale *= -1.0;
    break;
  case 5:
    splatConfig.yScale *= -1.0;
    break;
  case 6:
    splatConfig.alphaTest = GL_TRUE;
    break;
  case 7:
    splatConfig.alphaTest = GL_FALSE;
    break;
  case 411:
    glGetFloatv(GL_CURRENT_RASTER_POSITION, rpos);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
    printf("Raster position (%g,%g) is %s\n",
      rpos[0], rpos[1], valid ? "valid" : "INVALID");
    break;
  case 666:
    exit(0);
    break;
  }
}

void
scaleBiasSelect(int value)
{
  int color = value >> 4;
  int option = value & 0xf;

  switch(option) {
  case 1:
    splatConfig.bias[color] += 0.25;
    break;
  case 2:
    splatConfig.bias[color] -= 0.25;
    break;
  case 3:
    splatConfig.scale[color] *= 2.0;
    break;
  case 4:
    splatConfig.scale[color] *= 0.75;
    break;
  }
}

int
glutScaleBiasMenu(int mask)
{
  int menu;

  menu = glutCreateMenu(scaleBiasSelect);
  glutAddMenuEntry("+25% bias", mask | 1);
  glutAddMenuEntry("-25% bias", mask | 2);
  glutAddMenuEntry("+25% scale", mask | 3);
  glutAddMenuEntry("-25% scale", mask | 4);
  return menu;
}

int
main(int argc, char *argv[])
{
  int mainMenu, redMenu, greenMenu, blueMenu;

  glutInitWindowSize(680, 440);
  glutInit(&argc, argv);
  splatConfig = splatDefault;

  glutCreateWindow("splatlogo");

  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glAlphaFunc(GL_GEQUAL, 0.5);
  glDisable(GL_ALPHA_TEST);
  glEnable(GL_DITHER);
  glClearColor(1.0, 1.0, 1.0, 0.0);

  redMenu = glutScaleBiasMenu(0 << 4);
  greenMenu = glutScaleBiasMenu(1 << 4);
  blueMenu = glutScaleBiasMenu(2 << 4);

  mainMenu = glutCreateMenu(mainSelect);
  glutAddMenuEntry("Reset splays", 0);
  glutAddMenuEntry("Reset splat config", 1);
  glutAddSubMenu("Red control", redMenu);
  glutAddSubMenu("Green control", greenMenu);
  glutAddSubMenu("Blue control", blueMenu);
  glutAddMenuEntry("+25% zoom", 2);
  glutAddMenuEntry("-25% zoom", 3);
  glutAddMenuEntry("X flip", 4);
  glutAddMenuEntry("Y flip", 5);
  glutAddMenuEntry("Enable alpha test", 6);
  glutAddMenuEntry("Disable alpha test", 7);
  glutSetMenu(mainMenu);
  glutAddMenuEntry("Query raster position", 411);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0; /* Never reached; make ANSI C happy. */
}
