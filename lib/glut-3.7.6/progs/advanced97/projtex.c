/*
** Demonstrates simple projective texture mapping.
**
** Button1 changes view, Button2 moves texture.
**
** (See: Segal, Korobkin, van Widenfelt, Foran, and Haeberli
**  "Fast Shadows and Lighting Effects Using Texture Mapping", SIGGRAPH	'92)
**
** 1994,1995 -- David G Yu
**
** cc -o projtex projtex.c texture.c -lglut -lGLU -lGL -lX11 -lm
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int winWidth, winHeight;

GLboolean redrawContinuously;

float angle, axis[3];
enum MoveModes { MoveNone, MoveView, MoveObject, MoveTexture };
enum MoveModes mode = MoveNone;

GLfloat objectXform[4][4];
GLfloat textureXform[4][4];

void (*drawObject)(void);
void (*loadTexture)(void);
GLboolean textureEnabled = GL_TRUE;
GLboolean showProjection = GL_TRUE;
GLboolean linearFilter = GL_TRUE;

char *texFilename = NULL;

GLfloat zoomFactor = 1.0;

/*****************************************************************/

/* matrix = identity */
void
matrixIdentity(GLfloat matrix[16])
{
    matrix[ 0] = 1.0;
    matrix[ 1] = 0.0;
    matrix[ 2] = 0.0;
    matrix[ 3] = 0.0;
    matrix[ 4] = 0.0;
    matrix[ 5] = 1.0;
    matrix[ 6] = 0.0;
    matrix[ 7] = 0.0;
    matrix[ 8] = 0.0;
    matrix[ 9] = 0.0;
    matrix[10] = 1.0;
    matrix[11] = 0.0;
    matrix[12] = 0.0;
    matrix[13] = 0.0;
    matrix[14] = 0.0;
    matrix[15] = 1.0;
}

/* matrix2 = transpose(matrix1) */
void
matrixTranspose(GLfloat matrix2[16], GLfloat matrix1[16])
{
    matrix2[ 0] = matrix1[ 0];
    matrix2[ 1] = matrix1[ 4];
    matrix2[ 2] = matrix1[ 8];
    matrix2[ 3] = matrix1[12];

    matrix2[ 4] = matrix1[ 1];
    matrix2[ 5] = matrix1[ 5];
    matrix2[ 6] = matrix1[ 9];
    matrix2[ 7] = matrix1[13];

    matrix2[ 8] = matrix1[ 2];
    matrix2[ 9] = matrix1[ 6];
    matrix2[10] = matrix1[10];
    matrix2[11] = matrix1[14];

    matrix2[12] = matrix1[ 3];
    matrix2[13] = matrix1[ 7];
    matrix2[14] = matrix1[14];
    matrix2[15] = matrix1[15];
}

/*****************************************************************/

/* load SGI .rgb image (pad with a border of the specified width and color) */
static void
imgLoad(char *filenameIn, int borderIn, GLfloat borderColorIn[4],
	int *wOut, int *hOut, GLubyte **imgOut)
{
    int border = borderIn;
    int width, height;
    int w, h;
    GLubyte *image, *img, *p;
    int i, j, components;

    image = (GLubyte *)read_texture(filenameIn, &width, &height, &components);
    w = width + 2*border;
    h = height + 2*border;
    img = (GLubyte *) calloc(w*h, 4*sizeof(unsigned char));

    p = img;
    for (j=-border; j<height+border; ++j) {
	for (i=-border; i<width+border; ++i) {
	    if (0 <= j && j <= height-1 && 0 <= i && i <= width-1) {
		p[0] = image[4*(j*width+i)+0];
		p[1] = image[4*(j*width+i)+1];
		p[2] = image[4*(j*width+i)+2];
		p[3] = 0xff;
	    } else {
		p[0] = borderColorIn[0] * 0xff;
		p[1] = borderColorIn[1] * 0xff;
		p[2] = borderColorIn[2] * 0xff;
		p[3] = borderColorIn[3] * 0xff;
	    }
	    p += 4;
	}
    }
    free(image);
    *wOut = w;
    *hOut = h;
    *imgOut = img;
}

/*****************************************************************/

/* Load the image file specified on the command line as the current texture */
void
loadImageTexture(void)
{
    static int texWidth, texHeight;
    static GLubyte *texData; 
    GLfloat borderColor[4] = { 1.0, 1.0, 1.0, 1.0 };

    if (!texData) {
	imgLoad(texFilename, 2, borderColor, &texWidth, &texHeight, &texData);
    }

    if (linearFilter) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 4, texWidth, texHeight,
		      GL_RGBA, GL_UNSIGNED_BYTE, texData);
}

/* Create a simple spotlight pattern and make it the current texture */
void
loadSpotlightTexture(void)
{
    static int texWidth = 64, texHeight = 64;
    static GLubyte *texData; 
    GLfloat borderColor[4] = { 0.1, 0.1, 0.1, 1.0 };

    if (!texData) {
	GLubyte *p;
	int i, j;

	texData = (GLubyte *) malloc(texWidth*texHeight*4*sizeof(GLubyte));

	p = texData;
	for (j=0; j<texHeight; ++j) {
	    float dy = (texHeight*0.5 - j+0.5) / (texHeight*0.5);

	    for (i=0; i<texWidth; ++i) {
		float dx = (texWidth*0.5 - i+0.5) / (texWidth*0.5);
		float r = cos(M_PI/2.0 * sqrt(dx*dx +dy*dy));
		float c;

		r = (r < 0) ? 0 : r*r;
		c = 0xff * (r + borderColor[0]);
		p[0] = (c <= 0xff) ? c : 0xff;
		c = 0xff * (r + borderColor[1]);
		p[1] = (c <= 0xff) ? c : 0xff;
		c = 0xff * (r + borderColor[2]);
		p[2] = (c <= 0xff) ? c : 0xff;
		c = 0xff * (r + borderColor[3]);
		p[3] = (c <= 0xff) ? c : 0xff;
		p += 4;
	    }
	}
    }

    if (linearFilter) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 4, texWidth, texHeight,
		      GL_RGBA, GL_UNSIGNED_BYTE, texData);
}

/*****************************************************************/

void
checkErrors(void)
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
	fprintf(stderr, "Error: %s\n", (char *) gluErrorString(error));
    }
}

void
drawCube(void)
{
    glBegin(GL_QUADS);

    glNormal3f(-1.0, 0.0, 0.0 );
    glColor3f(0.80, 0.50, 0.50);
    glVertex3f(-0.5,-0.5,-0.5 ); glVertex3f(-0.5,-0.5, 0.5 );
    glVertex3f(-0.5, 0.5, 0.5 ); glVertex3f(-0.5, 0.5,-0.5 );

    glNormal3f( 1.0, 0.0, 0.0 );
    glColor3f(0.50, 0.80, 0.50);
    glVertex3f( 0.5, 0.5, 0.5 ); glVertex3f( 0.5,-0.5, 0.5 );
    glVertex3f( 0.5,-0.5,-0.5 ); glVertex3f( 0.5, 0.5,-0.5 );

    glNormal3f( 0.0,-1.0, 0.0 );
    glColor3f(0.50, 0.50, 0.80);
    glVertex3f(-0.5,-0.5,-0.5 ); glVertex3f( 0.5,-0.5,-0.5 );
    glVertex3f( 0.5,-0.5, 0.5 ); glVertex3f(-0.5,-0.5, 0.5 );

    glNormal3f( 0.0, 1.0, 0.0 );
    glColor3f(0.50, 0.80, 0.80);
    glVertex3f( 0.5, 0.5, 0.5 ); glVertex3f( 0.5, 0.5,-0.5 );
    glVertex3f(-0.5, 0.5,-0.5 ); glVertex3f(-0.5, 0.5, 0.5 );

    glNormal3f( 0.0, 0.0,-1.0 );
    glColor3f(0.80, 0.50, 0.80);
    glVertex3f(-0.5,-0.5,-0.5 ); glVertex3f(-0.5, 0.5,-0.5 );
    glVertex3f( 0.5, 0.5,-0.5 ); glVertex3f( 0.5,-0.5,-0.5 );

    glNormal3f( 0.0, 0.0, 1.0 );
    glColor3f(1.00, 0.80, 0.50);
    glVertex3f( 0.5, 0.5, 0.5 ); glVertex3f(-0.5, 0.5, 0.5 );
    glVertex3f(-0.5,-0.5, 0.5 ); glVertex3f( 0.5,-0.5, 0.5 );
    glEnd();
}

void
drawDodecahedron(void)
{
#define A (0.5 * 1.61803)	/* (sqrt(5) + 1) / 2 */
#define B (0.5 * 0.61803)	/* (sqrt(5) - 1) / 2 */
#define C (0.5 * 1.0)
    GLfloat vertexes[20][3] = {
	{-A, 0.0, B }, {-A, 0.0,-B }, { A, 0.0,-B }, { A, 0.0, B },
	{ B,-A, 0.0 }, {-B,-A, 0.0 }, {-B, A, 0.0 }, { B, A, 0.0 },
	{ 0.0, B,-A }, { 0.0,-B,-A }, { 0.0,-B, A }, { 0.0, B, A },
	{-C,-C, C }, {-C,-C,-C }, { C,-C,-C }, { C,-C, C },
	{-C, C, C }, {-C, C,-C }, { C, C,-C }, { C, C, C },
    };
#undef A
#undef B
#undef C
    GLint polygons[12][5] = {
	{  0, 12, 10, 11, 16 },
	{  1, 17,  8,  9, 13 },
	{  2, 14,  9,  8, 18 },
	{  3, 19, 11, 10, 15 },
	{  4, 14,  2,  3, 15 },
	{  5, 12,  0,  1, 13 },
	{  6, 17,  1,  0, 16 },
	{  7, 19,  3,  2, 18 },
	{  8, 17,  6,  7, 18 },
	{  9, 14,  4,  5, 13 },
	{ 10, 12,  5,  4, 15 },
	{ 11, 19,  7,  6, 16 },
    };
    int i;

    glColor3f(0.75, 0.75, 0.75);
    for (i=0; i<12; ++i) {
	GLfloat *p0, *p1, *p2, d;
	GLfloat u[3], v[3], n[3];

	p0 = &vertexes[polygons[i][0]][0];
	p1 = &vertexes[polygons[i][1]][0];
	p2 = &vertexes[polygons[i][2]][0];

	u[0] = p2[0] - p1[0];
	u[1] = p2[1] - p1[1];
	u[2] = p2[2] - p1[2];

	v[0] = p0[0] - p1[0];
	v[1] = p0[1] - p1[1];
	v[2] = p0[2] - p1[2];

	n[0] = u[1]*v[2] - u[2]*v[1];
	n[1] = u[2]*v[0] - u[0]*v[2];
	n[2] = u[0]*v[1] - u[1]*v[0];

	d = 1.0 / sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
	n[0] *= d;
	n[1] *= d;
	n[2] *= d;

	glBegin(GL_POLYGON);
	glNormal3fv(n);
	glVertex3fv(p0);
	glVertex3fv(p1);
	glVertex3fv(p2);
	glVertex3fv(vertexes[polygons[i][3]]);
	glVertex3fv(vertexes[polygons[i][4]]);
	glEnd();
    }
}

void
drawSphere(void)
{
    int numMajor = 24;
    int numMinor = 32;
    float radius = 0.8;
    double majorStep = (M_PI / numMajor);
    double minorStep = (2.0*M_PI / numMinor);
    int i, j;

    glColor3f(0.50, 0.50, 0.50);
    for (i=0; i<numMajor; ++i) {
	double a = i * majorStep;
	double b = a + majorStep;
	double r0 = radius * sin(a);
	double r1 = radius * sin(b);
	GLfloat z0 = radius * cos(a);
	GLfloat z1 = radius * cos(b);

	glBegin(GL_TRIANGLE_STRIP);
	for (j=0; j<=numMinor; ++j) {
	    double c = j * minorStep;
	    GLfloat x = cos(c);
	    GLfloat y = sin(c);

	    glNormal3f((x*r0)/radius, (y*r0)/radius, z0/radius);
	    glTexCoord2f(j/(GLfloat) numMinor, i/(GLfloat) numMajor);
	    glVertex3f(x*r0, y*r0, z0);

	    glNormal3f((x*r1)/radius, (y*r1)/radius, z1/radius);
	    glTexCoord2f(j/(GLfloat) numMinor, (i+1)/(GLfloat) numMajor);
	    glVertex3f(x*r1, y*r1, z1);
	}
	glEnd();
    }
}

/*****************************************************************/

float xmin = -0.035, xmax = 0.035;
float ymin = -0.035, ymax = 0.035;
float znear = 0.1;
float zfar = 1.9;
float distance = -1.0;

static void
loadTextureProjection(GLfloat m[16])
{
    GLfloat mInverse[4][4];

    /*
    ** Should use true inverse, but since m consists only
    ** of rotations, we can just use the transpose.
    */
    matrixTranspose((GLfloat *) mInverse, m);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.5, 0.5, 0.0);
    glScalef(0.5, 0.5, 1.0);
    glFrustum(xmin, xmax, ymin, ymax, znear, zfar);
    glTranslatef(0.0, 0.0, distance);
    glMultMatrixf((GLfloat *) mInverse);
    glMatrixMode(GL_MODELVIEW);
}

static void
drawTextureProjection(void)
{
    float t = zfar / znear;
    GLfloat n[4][3];
    GLfloat f[4][3];

    n[0][0] = xmin;
    n[0][1] = ymin;
    n[0][2] = -(znear + distance);

    n[1][0] = xmax;
    n[1][1] = ymin;
    n[1][2] = -(znear + distance);

    n[2][0] = xmax;
    n[2][1] = ymax;
    n[2][2] = -(znear + distance);

    n[3][0] = xmin;
    n[3][1] = ymax;
    n[3][2] = -(znear + distance);

    f[0][0] = xmin * t;
    f[0][1] = ymin * t;
    f[0][2] = -(zfar + distance);

    f[1][0] = xmax * t;
    f[1][1] = ymin * t;
    f[1][2] = -(zfar + distance);

    f[2][0] = xmax * t;
    f[2][1] = ymax * t;
    f[2][2] = -(zfar + distance);

    f[3][0] = xmin * t;
    f[3][1] = ymax * t;
    f[3][2] = -(zfar + distance);

    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_LINE_LOOP);
      glVertex3fv(n[0]);
      glVertex3fv(n[1]);
      glVertex3fv(n[2]);
      glVertex3fv(n[3]);
      glVertex3fv(f[3]);
      glVertex3fv(f[2]);
      glVertex3fv(f[1]);
      glVertex3fv(f[0]);
      glVertex3fv(n[0]);
      glVertex3fv(n[1]);
      glVertex3fv(f[1]);
      glVertex3fv(f[0]);
      glVertex3fv(f[3]);
      glVertex3fv(f[2]);
      glVertex3fv(n[2]);
      glVertex3fv(n[3]);
    glEnd();
}

/*****************************************************************/

void
initialize(void)
{
    GLfloat light0Pos[4] = { 0.3, 0.3, 0.0, 1.0 };
    GLfloat matAmb[4] = { 0.01, 0.01, 0.01, 1.00 };
    GLfloat matDiff[4] = { 0.65, 0.65, 0.65, 1.00 };
    GLfloat matSpec[4] = { 0.30, 0.30, 0.30, 1.00 };
    GLfloat matShine = 10.0;
    GLfloat eyePlaneS[] = { 1.0, 0.0, 0.0, 0.0 };
    GLfloat eyePlaneT[] = { 0.0, 1.0, 0.0, 0.0 };
    GLfloat eyePlaneR[] = { 0.0, 0.0, 1.0, 0.0 };
    GLfloat eyePlaneQ[] = { 0.0, 0.0, 0.0, 1.0 };

    /*
    ** Setup Misc.
    */
    glClearColor(0.41, 0.41, 0.31, 0.0);

    glEnable(GL_DEPTH_TEST);

    glLineWidth(2.0);

    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glFrustum(-0.5, 0.5, -0.5, 0.5, 1, 3);
    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0, 0, -2);

    matrixIdentity((GLfloat *) objectXform);
    matrixIdentity((GLfloat *) textureXform);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glRasterPos2i(0, 0);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    /*
    ** Setup Lighting
    */
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, matShine);

    glEnable(GL_COLOR_MATERIAL);

    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glEnable(GL_LIGHT0);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_LIGHTING);

    /*
    ** Setup Texture
    */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    (*loadTexture)();

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, eyePlaneS);

    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_T, GL_EYE_PLANE, eyePlaneT);

    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_R, GL_EYE_PLANE, eyePlaneR);

    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_Q, GL_EYE_PLANE, eyePlaneQ);
}

void
display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (textureEnabled) {
	if (mode == MoveTexture || mode == MoveView) {
	    /* Have OpenGL compute the new transformation (simple but slow). */
	    glPushMatrix();
	    glLoadIdentity();
	    glRotatef(angle, axis[0], axis[1], axis[2]);
	    glMultMatrixf((GLfloat *) textureXform);
	    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) textureXform);
	    glPopMatrix();
	}

	loadTextureProjection((GLfloat *) textureXform);

	if (showProjection) {
	    glPushMatrix();
	    glMultMatrixf((GLfloat *) textureXform);
	    glDisable(GL_LIGHTING);
	    drawTextureProjection();
	    glEnable(GL_LIGHTING);
	    glPopMatrix();
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
    }

    if (mode == MoveObject || mode == MoveView) {
	/* Have OpenGL compute the new transformation (simple but slow). */
	glPushMatrix();
	glLoadIdentity();
	glRotatef(angle, axis[0], axis[1], axis[2]);
	glMultMatrixf((GLfloat *) objectXform);
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) objectXform);
	glPopMatrix();
    }

    glPushMatrix();
    glMultMatrixf((GLfloat *) objectXform);
    (*drawObject)();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);

    if (zoomFactor > 1.0) {
	glDisable(GL_DEPTH_TEST);
	glCopyPixels(0, 0, winWidth/zoomFactor, winHeight/zoomFactor, GL_COLOR);
	glEnable(GL_DEPTH_TEST);
    }
    glFlush();
    glutSwapBuffers();
    checkErrors();
}

/*****************************************************************/

/* simple trackball-like motion control */
GLboolean trackingMotion = GL_FALSE;
float lastPos[3];
int lastTime;

void
ptov(int x, int y, int width, int height, float v[3])
{
    float d, a;

    /* project x,y onto a hemi-sphere centered within width, height */
    v[0] = (2.0*x - width) / width;
    v[1] = (height - 2.0*y) / height;
    d = sqrt(v[0]*v[0] + v[1]*v[1]);
    v[2] = cos((M_PI/2.0) * ((d < 1.0) ? d : 1.0));
    a = 1.0 / sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    v[0] *= a;
    v[1] *= a;
    v[2] *= a;
}

void
startMotion(int x, int y, int but, int time)
{
    if (but == GLUT_LEFT_BUTTON) {
	mode = MoveView;
    } else if (but == GLUT_RIGHT_BUTTON) {
	mode = MoveTexture;
    } else {
	return;
    }

    trackingMotion = GL_TRUE;
    redrawContinuously = GL_FALSE;
    lastTime = time;
    ptov(x, y, winWidth, winHeight, lastPos);
}

/*ARGSUSED*/
void
stopMotion(int x, int y, int but, int time)
{
    if ((but == GLUT_LEFT_BUTTON && mode == MoveView) ||
	(but == GLUT_RIGHT_BUTTON && mode == MoveTexture))
    {
	trackingMotion = GL_FALSE;
    } else {
	return;
    }

    if (time == lastTime) {
	redrawContinuously = GL_TRUE;
	glutIdleFunc(display);
    } else {
	angle = 0.0;
	redrawContinuously = GL_FALSE;
	glutIdleFunc(0);
    }
    if (!redrawContinuously) {
	mode = MoveNone;
    }
}

void
trackMotion(int x, int y)
{
    if (trackingMotion) {
	float curPos[3], dx, dy, dz;

	ptov(x, y, winWidth, winHeight, curPos);

	dx = curPos[0] - lastPos[0];
	dy = curPos[1] - lastPos[1];
	dz = curPos[2] - lastPos[2];
	angle = 90.0 * sqrt(dx*dx + dy*dy + dz*dz);

	axis[0] = lastPos[1]*curPos[2] - lastPos[2]*curPos[1];
	axis[1] = lastPos[2]*curPos[0] - lastPos[0]*curPos[2];
	axis[2] = lastPos[0]*curPos[1] - lastPos[1]*curPos[0];

	lastTime = glutGet(GLUT_ELAPSED_TIME);
	lastPos[0] = curPos[0];
	lastPos[1] = curPos[1];
	lastPos[2] = curPos[2];
	glutPostRedisplay();
    }
}

/*****************************************************************/

void
object(void)
{
    static int object;
    object++; object %= 3;
    switch (object) {
      case 0:
	drawObject = drawCube;
	break;
      case 1:
	drawObject = drawDodecahedron;
	break;
      case 2:
	drawObject = drawSphere;
	break;
      default:
	break;
    }
}

void
texture(void)
{
    static int texture;

    texture++; texture %= 3;
    switch (texture) {
      case 0:
	loadTexture = NULL;
	textureEnabled = GL_FALSE;
	break;
      case 1:
	loadTexture = loadImageTexture;
	(*loadTexture)();
	textureEnabled = GL_TRUE;
	break;
      case 2:
	loadTexture = loadSpotlightTexture;
	(*loadTexture)();
	textureEnabled = GL_TRUE;
	break;
      default:
	break;
    }
}

void help(void) {
    printf("'h'   - help\n");
    printf("'l'   - toggle linear/nearest filter\n");
    printf("'s'   - toggle projection frustum\n");
    printf("'t'   - toggle projected texture\n");
    printf("'o'   - toggle object\n");
    printf("'z'   - increase zoom factor\n");
    printf("'Z'   - decrease zoom factor\n");
    printf("left mouse    - move view\n");
    printf("right mouse   - move projection\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033':
	exit(EXIT_SUCCESS);
	break;
    case 'l':
	linearFilter = !linearFilter;
	(*loadTexture)();
	break;
    case 's':
	showProjection = !showProjection;
	break;
    case 't':
	texture();
	break;
    case 'o':
	object();
	break;
    case 'z':
	zoomFactor += 1.0;
	glPixelZoom(zoomFactor, zoomFactor);
	glViewport(0, 0, winWidth/zoomFactor, winHeight/zoomFactor);
	break;
    case 'Z':
	zoomFactor -= 1.0;
	if (zoomFactor < 1.0) zoomFactor = 1.0;
	glPixelZoom(zoomFactor, zoomFactor);
	glViewport(0, 0, winWidth/zoomFactor, winHeight/zoomFactor);
	break;
    case 'h': help(); break;
    }
    glutPostRedisplay();
}

void
mouse(int button, int state, int x, int y) {
    if(state == GLUT_DOWN)
	startMotion(x, y, button, glutGet(GLUT_ELAPSED_TIME));
    else if (state == GLUT_UP)
	stopMotion(x, y, button, glutGet(GLUT_ELAPSED_TIME));
    glutPostRedisplay();
}

void
reshape(int w, int h)
{
    winWidth = w;
    winHeight = h;
    glViewport(0, 0, w/zoomFactor, h/zoomFactor);
}

void
usage(char **argv)
{
    fprintf(stderr, "usage: %s <image-filename>\n", argv[0]);
    fprintf(stderr, "\n");
}

int
main(int argc, char **argv)
{
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    (void)glutCreateWindow("projtex");
    if (argc > 1) {
	texFilename = argv[argc-1];
    } else {
	texFilename = "../data/fendi.rgb";
    }
    loadTexture = loadImageTexture;
    drawObject = drawCube;
    initialize();
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(trackMotion);
    glutMainLoop();
    return 0;
}
