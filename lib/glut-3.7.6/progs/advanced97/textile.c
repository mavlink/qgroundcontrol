/*
** cc -o textile textile.c -lGLU -lGL -lglut -lXmu -lX11 -lm
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glut.h>
#include "texture.h"

int maxTextureSize;
int maxTextureLevel;

int imageWidth, imageHeight;
GLubyte *imageData;

int texWidthLevel0, texHeightLevel0;
int texWidthTiles, texHeightTiles;
GLubyte **texImageLevel;

GLboolean useBorder = GL_TRUE;
GLboolean useClamp = GL_TRUE;
GLboolean useLinear = GL_TRUE;
GLboolean useMipmap = GL_TRUE;
GLboolean useTextureTiling = GL_TRUE;

/* (int)floor(log2(a)) */
static int
iflog2(unsigned int a)
{
    int x = 0;
    while (a >>= 1) ++x;
    return x;
}

static void
initialize(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.5, 0.5, -0.5, 0.5, 0.5, 1.5);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -0.90);
    glRotatef( 45.0, 0, 1, 0);
    glTranslatef(-0.5, -0.5, 0.0);

#if 0
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
#else
    maxTextureSize = 32;
#endif
    maxTextureLevel = iflog2(maxTextureSize);

    texImageLevel = (GLubyte **) calloc(maxTextureLevel+1, sizeof(GLubyte *));
    if (texImageLevel == NULL) {
	fprintf(stderr, "texture level image allocation failed\n");
	exit(EXIT_FAILURE);
    }
    glClearColor(0.1, 0.1, 0.1, 0.1);
}

static void
imgLoad(char *filename_in, int *w_out, int *h_out, GLubyte **img_out)
{
    int comp;

    *img_out = (GLubyte *)read_texture(filename_in, w_out, h_out, &comp);
    if (img_out == NULL) {
	fprintf(stderr, "unable to read %s\n", filename_in);
	exit(EXIT_FAILURE);
    }
    if (comp != 3 && comp != 4) {
	fprintf(stderr, "%s: image is not RGB or RGBA\n", filename_in);
	exit(EXIT_FAILURE);
    }
}

static void
buildMipmaps(void)
{
    int level, levelWidth, levelHeight;

    if (useTextureTiling) {
	int width2 = iflog2(imageWidth);
	int height2 = iflog2(imageHeight);

	width2 = (width2 > maxTextureLevel) ? width2 : maxTextureLevel;
	height2 = (height2 > maxTextureLevel) ? height2 : maxTextureLevel;

	texWidthLevel0 = 1 << width2;
	texHeightLevel0 = 1 << height2;
	texWidthTiles = texWidthLevel0 >> maxTextureLevel;
	texHeightTiles = texHeightLevel0 >> maxTextureLevel;
    } else {
	texWidthLevel0 = maxTextureSize;
	texHeightLevel0 = maxTextureSize;
	texWidthTiles = 1;
	texHeightTiles = 1;
    }

    texImageLevel[0] = (GLubyte *)
	calloc(1, (texWidthLevel0+2)*(texHeightLevel0+2)*4*sizeof(GLubyte));

    glPixelStorei(GL_PACK_ROW_LENGTH, texWidthLevel0+2);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 1);
    glPixelStorei(GL_PACK_SKIP_ROWS, 1);

    gluScaleImage(GL_RGBA, imageWidth, imageHeight,
		  GL_UNSIGNED_BYTE, imageData,
		  texWidthLevel0, texHeightLevel0,
		  GL_UNSIGNED_BYTE, texImageLevel[0]);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 1);

    levelWidth = texWidthLevel0;
    levelHeight = texHeightLevel0;
    for (level=0; level<maxTextureLevel; ++level) {
	int newLevelWidth = (levelWidth > 1) ? levelWidth / 2 : 1;
	int newLevelHeight = (levelHeight > 1) ? levelHeight / 2 : 1;
	
	texImageLevel[level+1] = (GLubyte *)
	    calloc(1, (newLevelWidth+2)*(newLevelHeight+2)*4*sizeof(GLubyte));

	glPixelStorei(GL_PACK_ROW_LENGTH, newLevelWidth+2);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, levelWidth+2);

	gluScaleImage(GL_RGBA, levelWidth, levelHeight,
		      GL_UNSIGNED_BYTE, texImageLevel[level],
		      newLevelWidth, newLevelHeight,
		      GL_UNSIGNED_BYTE, texImageLevel[level+1]);

	levelWidth = newLevelWidth;
	levelHeight = newLevelHeight;
    }

    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
}

static void
freeMipmaps(void)
{
    int i;

    for (i=0; i<=maxTextureLevel; ++i) {
	if (texImageLevel[i] != NULL) {
	    free(texImageLevel[i]);
	    texImageLevel[i] = NULL;
	}
    }
}

static void
loadTile(int row, int col)
{
    int border = useBorder ? 1 : 0;
    int level, levelWidth, levelHeight;

    levelWidth = texWidthLevel0;
    levelHeight = texHeightLevel0;
    for (level=0; level<=maxTextureLevel; ++level) {
	int tileWidth = levelWidth / texWidthTiles;
	int tileHeight = levelHeight / texHeightTiles;
	int skipPixels = col * tileWidth + (1 - border);
	int skipRows = row * tileHeight + (1 - border);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, levelWidth+2);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);

	glTexImage2D(GL_TEXTURE_2D, level, 4,
		     tileWidth + 2*border, tileHeight + 2*border,
		     border, GL_RGBA, GL_UNSIGNED_BYTE, texImageLevel[level]);
	
	if (levelWidth > 1) levelWidth = levelWidth / 2;
	if (levelHeight > 1) levelHeight = levelHeight / 2;
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
}

static void
redraw(void)
{
    GLenum minFilterMode, magFilterMode, wrapMode;
    char *minFilterName, *magFilterName, *wrapName;
    int i, j;

    if (useLinear) {
	if (useMipmap) {
	    minFilterMode = GL_LINEAR_MIPMAP_LINEAR;
	    minFilterName = "LINEAR_MIPMAP_LINEAR";
	} else {
	    minFilterMode = GL_LINEAR;
	    minFilterName = "LINEAR";
	}
	magFilterMode = GL_LINEAR;
	magFilterName = "LINEAR";
    } else {
	if (useMipmap) {
	    minFilterMode = GL_NEAREST_MIPMAP_LINEAR;
	    minFilterName = "NEAREST_MIPMAP_LINEAR";
	} else {
	    minFilterMode = GL_NEAREST;
	    minFilterName = "NEAREST";
	}
	magFilterMode = GL_NEAREST;
	magFilterName = "NEAREST";
    }

    if (useClamp) {
	wrapMode = GL_CLAMP;
	wrapName = "CLAMP";
    } else {
	wrapMode = GL_REPEAT;
	wrapName = "REPEAT";
    }

    fprintf(stderr, "tile(%s) ", useTextureTiling ? "yes" : "no");
    fprintf(stderr, "border(%s) ", useBorder ? "yes" : "no");
    fprintf(stderr, "filter(%s, %s) ", minFilterName, magFilterName);
    fprintf(stderr, "wrap(%s) ", wrapName);
    fprintf(stderr, "\n");

    glClear(GL_COLOR_BUFFER_BIT);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    buildMipmaps();

    glEnable(GL_TEXTURE_2D);

    for (i=0; i<texHeightTiles; ++i) {
	float ySize = 1.0 / texHeightTiles;
	float y0 = i * ySize;
	float y1 = y0 + ySize;

	for (j=0; j<texWidthTiles; ++j) {
	    float xSize = 1.0 / texWidthTiles;
	    float x0 = j * xSize;
	    float x1 = x0 + xSize;

	    loadTile(i, j);

	    glBegin(GL_TRIANGLE_STRIP);
	    glTexCoord2f(0.0, 1.0); glVertex2f(x0, y1);
	    glTexCoord2f(0.0, 0.0); glVertex2f(x0, y0);
	    glTexCoord2f(1.0, 1.0); glVertex2f(x1, y1);
	    glTexCoord2f(1.0, 0.0); glVertex2f(x1, y0);
	    glEnd();
	}
    }

    glDisable(GL_TEXTURE_2D);

    freeMipmaps();
}

static void
usage(char *argv[])
{
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [ options ] filename\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "    Demonstrates using texture borders\n");
    fprintf(stderr, "    to tile a large texture\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -sb  single buffered\n");
    fprintf(stderr, "    -db  double buffered\n");
    fprintf(stderr, "\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    case 'b':	useBorder = !useBorder; break;
    case 'w':	useClamp = !useClamp; break;
    case 'l':	useLinear = !useLinear; break;
    case 'm':	useMipmap = !useMipmap; break;
    case 't':	useTextureTiling = !useTextureTiling; break;
    default:	break;
    }
    glutPostRedisplay();
}

int doubleBuffered = GL_FALSE;

void display(void) {
    GLenum error;
    redraw();
    if (doubleBuffered)
	glutSwapBuffers();
    else
	glFlush();
    while ((error = glGetError()) != GL_NO_ERROR) {
	fprintf(stderr, "Error: %s\n", (char *) gluErrorString(error));
    }
}

void
reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

int
main(int argc, char *argv[])
{
    char *name = "Texture Tiling Test";
    int width = 300, height = 300;
    char *filename = "../data/fendi.rgb";
    int i;

    glutInitWindowSize(width, height);
    glutInit(&argc, argv);

    for (i=1; i<argc; ++i) {
	if (!strcmp("-sb", argv[i])) {
	    doubleBuffered = GL_FALSE;

	} else if (!strcmp("-db", argv[i])) {
	    doubleBuffered = GL_TRUE;

	} else if (argv[i][0] != '-' && i==argc-1) {
	    filename = argv[i];

	} else {
	    usage(argv);
	    exit(EXIT_FAILURE);
	}
    }

    if (filename == NULL) {
	usage(argv);
	exit(EXIT_FAILURE);
    }

    imgLoad(filename, &imageWidth, &imageHeight, &imageData);

    glutInitDisplayMode(GLUT_RGBA|(doubleBuffered ? GLUT_DOUBLE : 0));
    (void)glutCreateWindow(name);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    initialize();
    glutMainLoop();
    return 0;
}
