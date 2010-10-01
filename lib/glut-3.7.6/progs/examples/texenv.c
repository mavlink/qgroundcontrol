
/* Copyright (c) Mark J. Kilgard, 1994. */

/**
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

/* Demonstrates texture environment modes and internal image formats.
   Requires the GL_EXT_texture extension.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#undef max
#undef min
#define max(a,b)	((a) >= (b) ? (a) : (b))
#define min(a,b)	((a) <= (b) ? (a) : (b))
/* *INDENT-OFF* */
GLfloat lightCheck[4] = {0.7, 0.7, 0.7, 1.0};
GLfloat darkCheck[4] = {0.3, 0.3, 0.3, 1.0};

GLfloat labelColor0[4] = {1.0, 1.0, 1.0, 1.0};
GLfloat labelColor1[4] = {1.0, 1.0, 0.4, 1.0};
GLfloat *labelInfoColor = labelColor0;
GLfloat labelLevelColor0[4] = {0.8, 0.8, 0.1, 1.0};
GLfloat labelLevelColor1[4] = {0.0, 0.0, 0.0, 1.0};
/* *INDENT-ON* */

GLboolean doubleBuffered = GL_FALSE;
GLboolean drawBackground = GL_FALSE;
GLboolean drawBlended = GL_TRUE;
GLboolean drawSmooth = GL_FALSE;
GLboolean drawTextured = GL_TRUE;
GLboolean displayLevelInfo = GL_FALSE;

int textureWidth = 64;
int textureHeight = 64;

int winWidth = 580, winHeight = 720;

struct formatInfo {
  GLenum baseFormat;
  GLenum internalFormat;
  char *name;
};

#define NUM_LUMINANCE_FORMATS \
	    (sizeof(luminanceFormats) / sizeof(luminanceFormats[0]))
struct formatInfo luminanceFormats[] =
{
  {GL_LUMINANCE, 1, "LUMINANCE"},
#if GL_EXT_texture
  {GL_LUMINANCE, GL_LUMINANCE4_EXT, "LUMINANCE4"},
  {GL_LUMINANCE, GL_LUMINANCE8_EXT, "LUMINANCE8"},
  {GL_LUMINANCE, GL_LUMINANCE12_EXT, "LUMINANCE12"},
  {GL_LUMINANCE, GL_LUMINANCE16_EXT, "LUMINANCE16"},
#endif
};

#define NUM_ALPHA_FORMATS \
	    (sizeof(alphaFormats) / sizeof(alphaFormats[0]))
struct formatInfo alphaFormats[] =
{
  {GL_ALPHA, GL_ALPHA, "ALPHA"},
#if GL_EXT_texture
  {GL_ALPHA, GL_ALPHA4_EXT, "ALPHA4"},
  {GL_ALPHA, GL_ALPHA8_EXT, "ALPHA8"},
  {GL_ALPHA, GL_ALPHA12_EXT, "ALPHA12"},
  {GL_ALPHA, GL_ALPHA16_EXT, "ALPHA16"},
#endif
};

#if GL_EXT_texture
#define NUM_INTENSITY_FORMATS \
	    (sizeof(intensityFormats) / sizeof(intensityFormats[0]))
struct formatInfo intensityFormats[] =
{
  {GL_INTENSITY_EXT, GL_INTENSITY_EXT, "INTENSITY"},
  {GL_INTENSITY_EXT, GL_INTENSITY4_EXT, "INTENSITY4"},
  {GL_INTENSITY_EXT, GL_INTENSITY8_EXT, "INTENSITY8"},
  {GL_INTENSITY_EXT, GL_INTENSITY12_EXT, "INTENSITY12"},
  {GL_INTENSITY_EXT, GL_INTENSITY16_EXT, "INTENSITY16"},
};
#endif

#define NUM_LUMINANCE_ALPHA_FORMATS \
	    (sizeof(luminanceAlphaFormats) / sizeof(luminanceAlphaFormats[0]))
struct formatInfo luminanceAlphaFormats[] =
{
  {GL_LUMINANCE_ALPHA, 2, "LUMINANCE_ALPHA"},
#if GL_EXT_texture
  {GL_LUMINANCE_ALPHA, GL_LUMINANCE4_ALPHA4_EXT, "LUMINANCE4_ALPHA4"},
  {GL_LUMINANCE_ALPHA, GL_LUMINANCE6_ALPHA2_EXT, "LUMINANCE6_ALPHA2"},
  {GL_LUMINANCE_ALPHA, GL_LUMINANCE8_ALPHA8_EXT, "LUMINANCE8_ALPHA8"},
  {GL_LUMINANCE_ALPHA, GL_LUMINANCE12_ALPHA4_EXT, "LUMINANCE12_ALPHA4"},
  {GL_LUMINANCE_ALPHA, GL_LUMINANCE12_ALPHA12_EXT, "LUMINANCE12_ALPHA12"},
  {GL_LUMINANCE_ALPHA, GL_LUMINANCE16_ALPHA16_EXT, "LUMINANCE16_ALPHA16"},
#endif
};

#define NUM_RGB_FORMATS \
	    (sizeof(rgbFormats) / sizeof(rgbFormats[0]))
struct formatInfo rgbFormats[] =
{
  {GL_RGB, 3, "RGB"},
#if GL_EXT_texture
  {GL_RGB, GL_RGB2_EXT, "RGB2"},
  {GL_RGB, GL_RGB4_EXT, "RGB4"},
  {GL_RGB, GL_RGB5_EXT, "RGB5"},
  {GL_RGB, GL_RGB8_EXT, "RGB8"},
  {GL_RGB, GL_RGB10_EXT, "RGB10"},
  {GL_RGB, GL_RGB12_EXT, "RGB12"},
  {GL_RGB, GL_RGB16_EXT, "RGB16"},
#endif
};

#define NUM_RGBA_FORMATS \
	    (sizeof(rgbaFormats) / sizeof(rgbaFormats[0]))
struct formatInfo rgbaFormats[] =
{
  {GL_RGBA, 4, "RGBA"},
#if GL_EXT_texture
  {GL_RGBA, GL_RGBA2_EXT, "RGBA2"},
  {GL_RGBA, GL_RGBA4_EXT, "RGBA4"},
  {GL_RGBA, GL_RGBA8_EXT, "RGBA8"},
  {GL_RGBA, GL_RGBA12_EXT, "RGBA12"},
  {GL_RGBA, GL_RGBA16_EXT, "RGBA16"},
  {GL_RGBA, GL_RGB5_A1_EXT, "RGB5_A1"},
  {GL_RGBA, GL_RGB10_A2_EXT, "RGB10_A2"},
#endif
};

struct baseFormatInfo {
  struct formatInfo *format;
  int current, number;
};

#define NUM_BASE_FORMATS \
	    (sizeof(baseFormats) / sizeof(baseFormats[0]))
int baseFormat;
struct baseFormatInfo baseFormats[] =
{
  {luminanceFormats, 0, NUM_LUMINANCE_FORMATS},
  {alphaFormats, 0, NUM_ALPHA_FORMATS},
#if GL_EXT_texture
  {intensityFormats, 0, NUM_INTENSITY_FORMATS},
#endif
  {luminanceAlphaFormats, 0, NUM_LUMINANCE_ALPHA_FORMATS},
  {rgbFormats, 0, NUM_RGB_FORMATS},
  {rgbaFormats, 0, NUM_RGBA_FORMATS},
};

#define NUM_ENV_COLORS \
	    (sizeof(envColors) / sizeof(envColors[0]))
int envColor;
GLfloat envColors[][4] =
{
  {0.0, 0.0, 0.0, 1.0},
  {1.0, 0.0, 0.0, 1.0},
  {0.0, 1.0, 0.0, 1.0},
  {0.0, 0.0, 1.0, 1.0},
  {1.0, 1.0, 1.0, 1.0},
};

struct envModeInfo {
  GLenum mode;
  char *name;
};

#define NUM_ENV_MODES \
	    (sizeof(envModes) / sizeof(envModes[0]))
struct envModeInfo envModes[] =
{
#if GL_EXT_texture
  {GL_REPLACE_EXT, "REPLACE"},
#endif
  {GL_MODULATE, "MODULATE"},
  {GL_BLEND, "BLEND"},
  {GL_DECAL, "DECAL"},
};

void
checkErrors(void)
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "Error: %s\n", (char *) gluErrorString(error));
  }
}

static void
drawString(char *string, GLfloat x, GLfloat y, GLfloat color[4])
{
  glColor4fv(color);
  glRasterPos2f(x, y);
  while (*string) {
    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *string);
    string++;
  }
}

static void
drawStringOutline(char *string, GLfloat x, GLfloat y,
  GLfloat color[4], GLfloat outline[4])
{
  drawString(string, x - 1, y, outline);
  drawString(string, x + 1, y, outline);
  drawString(string, x, y - 1, outline);
  drawString(string, x, y + 1, outline);
  drawString(string, x, y, color);
}

static void
begin2D(int width, int height)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, width, 0, height, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}

static void
end2D(void)
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

static void
initialize(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.5, 1.5, -1.5, 1.5, -1.5, 1.5);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glShadeModel(GL_FLAT);
}

/* ARGSUSED1 */
void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 'c':
    envColor = ++envColor % (int) NUM_ENV_COLORS;
    break;
  case 'g':
    drawBackground = !drawBackground;
    break;
  case 'b':
    drawBlended = !drawBlended;
    break;
  case 's':
    drawSmooth = !drawSmooth;
    break;
  case 't':
    drawTextured = !drawTextured;
    break;
  case 'i':
    displayLevelInfo = !displayLevelInfo;
    break;
  case 27:             /* Escape key should force exit. */
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
  case GLUT_KEY_DOWN:
    if (++baseFormat > NUM_BASE_FORMATS - 1)
      baseFormat = 0;
    break;
  case GLUT_KEY_UP:
    if (--baseFormat < 0)
      baseFormat = NUM_BASE_FORMATS - 1;
    break;
  case GLUT_KEY_LEFT:
    --baseFormats[baseFormat].current;
    if (baseFormats[baseFormat].current < 0)
      baseFormats[baseFormat].current =
        baseFormats[baseFormat].number - 1;
    break;
  case GLUT_KEY_RIGHT:
    ++baseFormats[baseFormat].current;
    if (baseFormats[baseFormat].current > baseFormats[baseFormat].number - 1)
      baseFormats[baseFormat].current = 0;
    break;
  default:
    break;
  }
  glutPostRedisplay();
}

void
reshape(int w, int h)
{
  winWidth = w;
  winHeight = h;
  /* No need to call glViewPort here since "draw" calls it! */
}

static void
loadTexture(int width, int height, struct formatInfo *format)
{
  int luminanceSize = 0;
  int alphaSize = 0;
  int rgbSize = 0;
  GLenum textureFormat;
  GLubyte *texImage, *p;
  int elementsPerGroup, elementSize, groupSize, rowSize;
  int i, j;

  switch (format->baseFormat) {
  case GL_LUMINANCE:
#if GL_EXT_texture
  case GL_INTENSITY_EXT:
#endif
    luminanceSize = 1;
    textureFormat = GL_LUMINANCE;
    break;
  case GL_ALPHA:
    alphaSize = 1;
    textureFormat = GL_ALPHA;
    break;
  case GL_LUMINANCE_ALPHA:
    luminanceSize = 1;
    alphaSize = 1;
    textureFormat = GL_LUMINANCE_ALPHA;
    break;
  case GL_RGB:
    rgbSize = 3;
    textureFormat = GL_RGB;
    break;
  case GL_RGBA:
    rgbSize = 3;
    alphaSize = 1;
    textureFormat = GL_RGBA;
    break;
  default:
    fprintf(stderr, "bad internal format info\n");
    return;
  }

  elementsPerGroup = luminanceSize + alphaSize + rgbSize;
  elementSize = sizeof(GLubyte);
  groupSize = elementsPerGroup * elementSize;
  rowSize = width * groupSize;

  if ((texImage = (GLubyte *) malloc(height * rowSize)) == NULL) {
    fprintf(stderr, "texture malloc failed\n");
    return;
  }
  for (i = 0; i < height; ++i) {
    p = texImage + i * rowSize;
    for (j = 0; j < width; ++j) {
      if (luminanceSize > 0) {
        /** 
         ** +-----+-----+
	 ** |     |     |
	 ** |  W  | LG  |
	 ** |     |     |
	 ** +-----+-----+
	 ** |     |     |
	 ** | DG  |  B  |
	 ** |     |     |
	 ** +-----+-----+
	 **/
        if (i > height / 2) {
          if (j < width / 2) {
            p[0] = 0xff;
          } else {
            p[0] = 0xaa;
          }
        } else {
          if (j < width / 2) {
            p[0] = 0x55;
          } else {
            p[0] = 0x00;
          }
        }
        p += elementSize;
      }
      if (rgbSize > 0) {
        /**
         ** +-----+-----+
	 ** |     |     |
	 ** |  R  |  G  |
         ** |     |     |
	 ** +-----+-----+
	 ** |     |     |
         ** |  Y  |  B  |
	 ** |     |     |
	 ** +-----+-----+
	 **/
        if (i > height / 2) {
          if (j < width / 2) {
            p[0] = 0xff;
            p[1] = 0x00;
            p[2] = 0x00;
          } else {
            p[0] = 0x00;
            p[1] = 0xff;
            p[2] = 0x00;
          }
        } else {
          if (j < width / 2) {
            p[0] = 0xff;
            p[1] = 0xff;
            p[2] = 0x00;
          } else {
            p[0] = 0x00;
            p[1] = 0x00;
            p[2] = 0xff;
          }
        }
        p += 3 * elementSize;
      }
      if (alphaSize > 0) {
        /**
         ** +-----------+
	 ** |     W     |
	 ** |  +-----+  |
         ** |  |     |  |
	 ** |  |  B  |  |
	 ** |  |     |  |
         ** |  +-----+  |
	 ** |           |
	 ** +-----------+
	 **/
        int i2 = i - height / 2;
        int j2 = j - width / 2;
        int h8 = height / 8;
        int w8 = width / 8;
        if (-h8 <= i2 && i2 <= h8 && -w8 <= j2 && j2 <= w8) {
          p[0] = 0x00;
        } else if (-2 * h8 <= i2 && i2 <= 2 * h8 && -2 * w8 <= j2 && j2 <= 2 * w8) {
          p[0] = 0x55;
        } else if (-3 * h8 <= i2 && i2 <= 3 * h8 && -3 * w8 <= j2 && j2 <= 3 * w8) {
          p[0] = 0xaa;
        } else {
          p[0] = 0xff;
        }
        p += elementSize;
      }
    }
  }

  glTexImage2D(GL_TEXTURE_2D, 0,
    format->internalFormat, width, height, 0,
    textureFormat, GL_UNSIGNED_BYTE, texImage);

  free(texImage);
}

static void
drawCheck(int w, int h, GLfloat lightCheck[4], GLfloat darkCheck[4])
{
  float dw = 2.0 / w;
  float dh = 2.0 / h;
  int i, j;

  for (i = 0; i < w; ++i) {
    GLfloat x0 = -1.0 + i * dw;
    GLfloat x1 = x0 + dw;

    glBegin(GL_QUAD_STRIP);
    for (j = 0; j <= h; ++j) {
      GLfloat y = -1.0 + j * dh;

      if ((i ^ j) & 1) {
        glColor4fv(lightCheck);
      } else {
        glColor4fv(darkCheck);
      }

      glVertex2f(x0, y);
      glVertex2f(x1, y);
    }
    glEnd();
  }
}

static void
drawSample(int x, int y, int w, int h,
  struct formatInfo *format, struct envModeInfo *envMode)
{
  glViewport(x, y, w, h);
  glScissor(x, y, w, h);

  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  begin2D(w, h);
  drawString(format->name, 10, h - 15, labelInfoColor);
  drawString(envMode->name, 10, 5, labelInfoColor);
  end2D();

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, envMode->mode);
  glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envColors[envColor]);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  loadTexture(textureWidth, textureHeight, format);

  if (drawBackground) {
    drawCheck(15, 15, lightCheck, darkCheck);
  }
  if (drawBlended) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  }
  if (drawSmooth) {
    glShadeModel(GL_SMOOTH);
  }
  if (drawTextured) {
    glEnable(GL_TEXTURE_2D);
  }
  glBegin(GL_QUADS);
  glColor4f(1.0, 0.0, 0.0, 1.0);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(-0.8, -0.8);
  glColor4f(0.0, 1.0, 0.0, 1.0);
  glTexCoord2f(1.0, 0.0);
  glVertex2f(0.8, -0.8);
  glColor4f(0.0, 0.0, 1.0, 1.0);
  glTexCoord2f(1.0, 1.0);
  glVertex2f(0.8, 0.8);
  glColor4f(1.0, 1.0, 1.0, 1.0);
  glTexCoord2f(0.0, 1.0);
  glVertex2f(-0.8, 0.8);
  glEnd();

  glDisable(GL_BLEND);
  glShadeModel(GL_FLAT);
  glDisable(GL_TEXTURE_2D);

  if (displayLevelInfo) {
    GLint width, height, border, components;
#if GL_EXT_texture
    GLint redSize, greenSize, blueSize, alphaSize;
    GLint luminanceSize, intensitySize;
#endif
    char buf[255];

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_BORDER, &border);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_COMPONENTS, &components);
#if GL_EXT_texture
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_RED_SIZE_EXT, &redSize);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_GREEN_SIZE_EXT, &greenSize);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_BLUE_SIZE_EXT, &blueSize);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_ALPHA_SIZE_EXT, &alphaSize);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_LUMINANCE_SIZE_EXT, &luminanceSize);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
      GL_TEXTURE_INTENSITY_SIZE_EXT, &intensitySize);
#endif

    begin2D(w, h);
    sprintf(buf, "dimensions: %d x %d", width, height);
    drawStringOutline(buf, 15, h / 2 + 20, labelLevelColor0, labelLevelColor1);

    sprintf(buf, "border: %d", border);
    drawStringOutline(buf, 15, h / 2 + 10, labelLevelColor0, labelLevelColor1);

    sprintf(buf, "components: 0x%04X", components);
    drawStringOutline(buf, 15, h / 2, labelLevelColor0, labelLevelColor1);

    sprintf(buf, "sizes:");
    drawStringOutline(buf, 15, h / 2 - 10, labelLevelColor0, labelLevelColor1);

#if GL_EXT_texture
    sprintf(buf, "  %d/%d/%d/%d/%d/%d",
      redSize, greenSize, blueSize, alphaSize,
      luminanceSize, intensitySize);
    drawStringOutline(buf, 15, h / 2 - 20, labelLevelColor0, labelLevelColor1);
#endif
    end2D();
  }
}

static void
display(void)
{
  int numX = NUM_ENV_MODES, numY = NUM_BASE_FORMATS;
  float xBase = (float) winWidth * 0.01;
  float xOffset = (winWidth - xBase) / numX;
  float xSize = max(xOffset - xBase, 1);
  float yBase = (float) winHeight * 0.01;
  float yOffset = (winHeight - yBase) / numY;
  float ySize = max(yOffset - yBase, 1);
  float x, y;
  int i, j;

  glViewport(0, 0, winWidth, winHeight);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_SCISSOR_TEST);

  x = xBase;
  y = (winHeight - 1) - yOffset;
  for (i = 0; i < NUM_BASE_FORMATS; ++i) {
    struct formatInfo *format;

    if (i == baseFormat) {
      labelInfoColor = labelColor1;
    } else {
      labelInfoColor = labelColor0;
    }

    format = &baseFormats[i].format[baseFormats[i].current];
    for (j = 0; j < NUM_ENV_MODES; ++j) {
      struct envModeInfo *envMode;

      envMode = &envModes[j];
      drawSample(x, y, xSize, ySize, format, envMode);
      x += xOffset;
    }
    x = xBase;
    y -= yOffset;
  }

  if (doubleBuffered) {
    glutSwapBuffers();
  } else {
    glFlush();
  }

  checkErrors();
}

static void
usage(char *name)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "usage: %s [ options ]\n", name);
  fprintf(stderr, "\n");
  fprintf(stderr, "    Tests texture environments and internal formats\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  Options:\n");
  fprintf(stderr, "    -sb  single buffered\n");
  fprintf(stderr, "    -db  double buffered\n");
  fprintf(stderr, "\n");
}

int
main(int argc, char *argv[])
{
  int i;

  glutInit(&argc, argv);
  for (i = 1; i < argc; ++i) {
    if (!strcmp("-sb", argv[i])) {
      doubleBuffered = GL_FALSE;
    } else if (!strcmp("-db", argv[i])) {
      doubleBuffered = GL_TRUE;
    } else {
      usage(argv[0]);
      exit(1);
    }
  }

#if !GL_EXT_texture
  printf("WARNING: client-side OpenGL implementation lacks\n");
  printf("         the GL_EXT_texture extension!\n");
  printf("         Skipping GL_EXT_texture functionality...\n");
#endif

  if (doubleBuffered) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  } else {
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  }

  glutInitWindowSize(winWidth, winHeight);
  glutCreateWindow("Texture Environment Test");

  if (!glutExtensionSupported("GL_EXT_texture")) {
    fprintf(stderr, "missing extension: GL_EXT_texture\n");
    exit(1);
  }
  initialize();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
