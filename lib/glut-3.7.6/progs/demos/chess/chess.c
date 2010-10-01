/*
 * chess.c - part of the chess demo in the glut distribution.
 *
 * (C) Henk Kok (kok@wins.uva.nl)
 *
 * This file can be freely copied, changed, redistributed, etc. as long as
 * this copyright notice stays intact.
 */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "chess.h"

#if 0
/* Uncomment to debug various scenarios. */
#undef GL_VERSION_1_1
#undef GL_EXT_texture_object
#undef GL_EXT_texture
#endif

#ifndef GL_VERSION_1_1

#if defined(GL_EXT_texture_object) && defined(GL_EXT_texture)
#define glGenTextures glGenTexturesEXT
#define glBindTexture glBindTextureEXT
#else
#define USE_DISPLAY_LISTS
#endif

#endif

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int texturing = 0;
int reflection = 0;
int chaos = 0;
int chaosPieces = 0;
int animating = 1;
static GLuint texName[3];

extern int path[10][10], piece, piece2;

extern GLfloat CX1, CY1, CX2, CY2, CZ2;

#define WIT	0
#define ZWART	16

int board[10][10];

GLubyte white_square[TXSX][TXSY][3];
GLubyte black_square[TXSX][TXSY][3];
GLubyte wood[TXSX][TXSY][3];

extern GLfloat lightpos[];

GLfloat buf[256], phase;
GLfloat transl[48];
int list[48];

GLfloat width[144], height[144];
GLfloat bwidth, bheight;

int cycle[10][10], cyclem, cycle2;
int stunt[10][10], stuntm, stunt2;

GLfloat blackamb[4] = { 0.2, 0.1, 0.1, 0.5 };
GLfloat blackdif[4] = { 0.2, 0.1, 0.0, 0.5 };
GLfloat blackspec[4] = { 0.5, 0.5, 0.5, 0.5 };

GLfloat whiteamb[4] = { 0.7, 0.7, 0.4, 0.5 };
GLfloat whitedif[4] = { 0.8, 0.7, 0.4, 0.5 };
GLfloat whitespec[4] = { 0.8, 0.7, 0.4, 0.5 };

GLfloat copperamb[4] = { 0.24, 0.2, 0.07, 1.0 };
GLfloat copperdif[4] = { 0.75, 0.61, 0.22, 1.0 };
GLfloat copperspec[4] = { 0.32, 0.25, 0.17, 1.0 };

GLfloat darkamb[4] = { 0.10, 0.10, 0.10, 1.0 };
GLfloat darkdif[4] = { 0.6, 0.6, 0.6, 1.0 };
GLfloat darkspec[4] = { 0.25, 0.25, 0.25, 1.0 };

GLdouble ClipPlane[4] = { 0.0, 1.0, 0.0, 0.0 };

void white_texture(void)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, whitedif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, whiteamb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, whitespec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40.0);
}

void black_texture(void)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, blackdif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, blackamb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, blackspec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40.0);
}

void copper_texture(void)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, copperdif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, copperamb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, copperspec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40.0);
}

void dark_texture(void)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, darkdif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, darkamb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, darkspec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40.0);
}

void border_texture(void)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, copperdif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, copperamb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, copperspec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 90.0);
}

void init_textures(void)
{
#if !defined(USE_DISPLAY_LISTS)
    glGenTextures(3, texName);
#else
    texName[0] = 1000;
    texName[1] = 1001;
    texName[2] = 1002;
#endif
    GenerateTextures();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#if !defined(USE_DISPLAY_LISTS)
    glBindTexture(GL_TEXTURE_2D, texName[0]);
#else
    glNewList(texName[0], GL_COMPILE);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, 3, TXSX, TXSY, 0, GL_RGB,
	GL_UNSIGNED_BYTE, &wood[0][0][0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
#if defined(USE_DISPLAY_LISTS)
    glEndList();
#endif

#if !defined(USE_DISPLAY_LISTS)
    glBindTexture(GL_TEXTURE_2D, texName[1]);
#else
    glNewList(texName[1], GL_COMPILE);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, 3, TXSX, TXSY, 0, GL_RGB,
	GL_UNSIGNED_BYTE, &white_square[0][0][0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
#if defined(USE_DISPLAY_LISTS)
    glEndList();
#endif

#if !defined(USE_DISPLAY_LISTS)
    glBindTexture(GL_TEXTURE_2D, texName[2]);
#else
    glNewList(texName[2], GL_COMPILE);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, 3, TXSX, TXSY, 0, GL_RGB,
	GL_UNSIGNED_BYTE, &black_square[0][0][0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
#if defined(USE_DISPLAY_LISTS)
    glEndList();
#endif
}

void do_border(void)
{
    glPushMatrix();
    glTranslatef(-0.5, 0.0, -0.5);
    if (texturing)
    {
#if !defined(USE_DISPLAY_LISTS)
	glBindTexture(GL_TEXTURE_2D, texName[0]);
#else
        glCallList(texName[0]);
#endif
	glEnable(GL_TEXTURE_2D);
    } else
	border_texture();

    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(0.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(8.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(8.5, 0.08, -0.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(-0.5, 0.08, -0.5);

    glTexCoord2f(0.0, 0.0); glVertex3f(8.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(8.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(8.5, 0.08, 8.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(8.5, 0.08, -0.5);

    glTexCoord2f(0.0, 0.0); glVertex3f(8.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(0.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(-0.5, 0.08, 8.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(8.5, 0.08, 8.5);

    glTexCoord2f(0.0, 0.0); glVertex3f(0.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(0.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(-0.5, 0.08, -0.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(-0.5, 0.08, 8.5);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(0.0, 0.0, 1.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(0.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(8.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(8.0, -0.08, 0.0);
    glTexCoord2f(0.0, 0.6); glVertex3f(0.0, -0.08, 0.0);

    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(8.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(8.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(8.0, -0.08, 8.0);
    glTexCoord2f(0.0, 0.6); glVertex3f(8.0, -0.08, 0.0);

    glNormal3f(0.0, 0.0, 1.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(8.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(0.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(0.0, -0.08, 8.0);
    glTexCoord2f(0.0, 0.6); glVertex3f(8.0, -0.08, 8.0);

    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(0.0, 0.08, 8.0);
    glTexCoord2f(0.6, 0.0); glVertex3f(0.0, 0.08, 0.0);
    glTexCoord2f(0.6, 0.6); glVertex3f(0.0, -0.08, 0.0);
    glTexCoord2f(0.0, 0.6); glVertex3f(0.0, -0.08, 8.0);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(0.0, 0.0, 1.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, 0.08, -0.5);
    glTexCoord2f(0.6, 0.0); glVertex3f(8.5, 0.08, -0.5);
    glTexCoord2f(0.6, 0.6); glVertex3f(8.5, -0.08, -0.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(-0.5, -0.08, -0.5);

    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(8.5, 0.08, -0.5);
    glTexCoord2f(0.6, 0.0); glVertex3f(8.5, 0.08, 8.5);
    glTexCoord2f(0.6, 0.6); glVertex3f(8.5, -0.08, 8.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(8.5, -0.08, -0.5);

    glNormal3f(0.0, 0.0, 1.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(8.5, 0.08, 8.5);
    glTexCoord2f(0.6, 0.0); glVertex3f(-0.5, 0.08, 8.5);
    glTexCoord2f(0.6, 0.6); glVertex3f(-0.5, -0.08, 8.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(8.5, -0.08, 8.5);

    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, 0.08, 8.5);
    glTexCoord2f(0.6, 0.0); glVertex3f(-0.5, 0.08, -0.5);
    glTexCoord2f(0.6, 0.6); glVertex3f(-0.5, -0.08, -0.5);
    glTexCoord2f(0.0, 0.6); glVertex3f(-0.5, -0.08, 8.5);
    glEnd();

    if (texturing)
	glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

void do_vlakje(void)
{
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(-0.5, 0.0, -0.5);
    glBegin(GL_QUADS);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(8.0, 0.0, 0.0);
    glVertex3f(8.0, 0.0, 8.0);
    glVertex3f(0.0, 0.0, 8.0);
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void do_board(void)
{
    int x,y;

    glPushMatrix();
    glTranslatef(-0.5, 0.0, -0.5);
    white_texture();
    if (texturing)
    {
#if !defined(USE_DISPLAY_LISTS)
	glBindTexture(GL_TEXTURE_2D, texName[1]);
#else
        glCallList(texName[1]);
#endif
	glEnable(GL_TEXTURE_2D);
    }

    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    for (x=0;x<8;x++)
    {
	for (y=x%2;y<8;y+=2)
	{
	    glTexCoord2f(0.2*x, 0.2*y); glVertex3f(x, 0, y);
	    glTexCoord2f(0.17+0.2*x, 0.2*y); glVertex3f(x+1, 0, y);
	    glTexCoord2f(0.17+0.2*x, 0.17+0.2*y); glVertex3f(x+1, 0, y+1);
	    glTexCoord2f(0.2*x, 0.17+0.2*y); glVertex3f(x, 0, y+1);
	}
    }
    glEnd();
    if (texturing)
    {
	glDisable(GL_TEXTURE_2D);

#if !defined(USE_DISPLAY_LISTS)
	glBindTexture(GL_TEXTURE_2D, texName[2]);
#else
        glCallList(texName[2]);
#endif
	glEnable(GL_TEXTURE_2D);
    } else
	black_texture();

    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    for (x=0;x<8;x++)
    {
	for (y=1-(x%2);y<8;y+=2)
	{
	    glTexCoord2f(0.2*x, 0.2*y); glVertex3f(x, 0, y);
	    glTexCoord2f(0.17+0.2*x, 0.2*y); glVertex3f(x+1, 0, y);
	    glTexCoord2f(0.17+0.2*x, 0.17+0.2*y); glVertex3f(x+1, 0, y+1);
	    glTexCoord2f(0.2*x, 0.17+0.2*y); glVertex3f(x, 0, y+1);
	}
    }
    glEnd();
    if (texturing)
	glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void do_solid(GLfloat *f, int sz, GLfloat width)
{
    GLfloat nx, ny, s;
    GLfloat length;
    int i,j;

    for (i=0;i<sz;i++)
	buf[i] = f[i]/4.2;

    for (i=0;i<sz;i+=2)
    {
	buf[i+1] = buf[i+1] * (1 - cos(phase)/3);
	buf[i] = buf[i] * (1+sin(M_PI*buf[i+1]/buf[1])*cos(phase)*0.7);
/*
	if (buf[i] > bwidth)
	    bwidth = buf[i];
*/
	if (buf[i+1] > bheight)
	    bheight = buf[i+1];
    }

    glBegin(GL_QUAD_STRIP);
    for (i=2;i<sz;i+=2)
    {
	if (buf[i+3] == buf[i+1] && buf[i+2] == buf[i])
	    continue;
	nx = buf[i-1] + buf[i+3];
	ny = buf[i+2] - buf[i];
	length = sqrt(nx*nx+ny*ny);
	nx = nx/length;
	ny = ny/length;

	s = (1+cos(phase)*0.7);
	glNormal3f(0.0, ny, nx);
	glVertex3f(width*s, buf[i+1], buf[i]);
	glVertex3f(-width*s, buf[i+1], buf[i]);
    }
    glEnd();

    glNormal3f(-1.0, 0.0, 0.0);
    i = 2; j = sz-4;
    glBegin(GL_TRIANGLE_STRIP);
    while (i < j)
    {
	while (buf[i-1] == buf[i+1] && buf[i-2] == buf[i] && i < j)
	    i += 2;
	if (i < j)
	{
	    s = (1+cos(phase)*0.7);
	    glVertex3f(-width*s, buf[i+1], buf[i]);
	}
	i += 2;
	while (buf[j-1] == buf[j+1] && buf[j-2] == buf[j] && i < j)
	    j -= 2;
	if (i < j)
	{
	    s = (1+cos(phase)*0.7);
	    glVertex3f(-width*s, buf[j+1], buf[j]);
	}
	j -= 2;
    }
    glEnd();

    i = 2; j = sz-4;
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(1.0, 0.0, 0.0);
    while (i < j)
    {
	while (buf[i-1] == buf[i+1] && buf[i-2] == buf[i] && i < j)
	    i += 2;
	if (i < j)
	{
	    s = (1+cos(phase)*0.7);
	    glVertex3f(width*s, buf[i+1], buf[i]);
	}
	i += 2;
	while (buf[j-1] == buf[j+1] && buf[j-2] == buf[j] && i < j)
	    j -= 2;
	if (i < j)
	{
	    s = (1+cos(phase)*0.7);
	    glVertex3f(width*s, buf[j+1], buf[j]);
	}
	j -= 2;
    }
    glEnd();
}

void do_rotate(GLfloat *f, int sz)
{
    GLfloat nx, ny;
    GLfloat length;
    GLfloat a, c, s;
    int i,j;

    bheight = 0;
    bwidth = 0;

    for (i=0;i<sz;i++)
	buf[i] = f[i] / 4.2;

    for (i=0;i<sz;i+=2)
    {
	buf[i+1] = buf[i+1] * (1 - cos(phase)/3);
	buf[i] = buf[i] * (1+sin(M_PI*buf[i+1]/buf[1])*cos(phase)*0.7);
	if (buf[i] > bwidth)
	    bwidth = buf[i];
	if (buf[i+1] > bheight)
	    bheight = buf[i+1];
    }

    for (i=2;i<sz-4;i+=2)
    {
	if (fabs(buf[i+3]+buf[i+1]) +fabs(buf[i+2]+buf[i]) < 0.0001)
	    continue;
	glBegin(GL_QUAD_STRIP);
	for (j=0;j<=ACC;j++)
	{
	    a = ((float) j)*M_PI*2/ACC;
	    c = cos(a);
	    s = sin(a);
	    nx = buf[i-1] + buf[i+3];
	    ny = buf[i+2] - buf[i];
	    length = sqrt(nx*nx+ny*ny);
	    nx = nx/length;
	    ny = ny/length;
	    glNormal3f(c*nx, ny, s*nx);
	    glVertex3f(c*buf[i], buf[i+1], s*buf[i]);

	    nx = buf[i-1] + buf[i+3];
	    ny = buf[i+2] - buf[i];
	    length = sqrt(nx*nx+ny*ny);
	    nx = nx/length;
	    ny = ny/length;
	    glNormal3f(c*nx, ny, s*nx);
	    glVertex3f(c*buf[i+2], buf[i+3], s*buf[i+2]);
	}
	glEnd();
    }
}

GLfloat pion_data[] = {
	-0.04, 3.000,   0.000, 0.000,   0.000, 0.000,   0.000, 0.000,
	0.000, 0.000,   0.000, 0.000,   0.000, 0.000,   0.000, 0.000,
	0.200, 2.000,   0.560, 1.900,   0.560, 1.800,   0.300, 1.800,
	0.300, 1.800,   0.520, 1.030,   0.520, 1.030,   0.550, 1.030,
	0.550, 1.030,   0.670, 0.670,   0.940, 0.450,   0.940, 0.300,
	0.840, 0.300,   0.840, 0.150,   0.940, 0.150,   0.940, 0.000,
	0.940, 0.000,	0.000, 0.000,	0.000, 0.000
};

void prepare_pion(void)
{
    int i;
    GLfloat a, c, s;
    for (i=0;i<7;i++)
    {
	a = ((GLfloat) i)*M_PI/8;
	s = sin(a);
	c = cos(a);
	pion_data[2+i*2] = s*0.56;
	pion_data[3+i*2] = c*0.56+2.44;
    }
    pion_data[2+i*2] = pion_data[i*2];
    pion_data[3+i*2] = pion_data[i*2+1];
}

void do_pion(void)
{
    do_rotate(pion_data, sizeof(pion_data)/sizeof(GLfloat));
}

GLfloat toren_data[] = {
	-0.04, 3.000,	0.000, 3.000,	0.600, 3.000,	0.600, 3.000,
	0.600, 3.200,	0.600, 3.200,	0.800, 3.200,	0.800, 3.200,
	0.800, 3.200,	0.600, 2.700,	0.600, 2.700,	0.550, 2.700,
	0.550, 2.700,	0.700, 1.200,	0.700, 1.200,	0.730, 1.200,
	0.730, 1.200,   0.850, 0.850,	1.050, 0.500,	1.050, 0.500,
	1.050, 0.300,	0.950, 0.350,	0.950, 0.150,	1.050, 0.150,
	1.050, 0.000,	1.050, 0.000,	0.000, 0.000,	0.000, 0.000
};
	
void do_toren(void)
{
    int i;
    GLfloat a1, a2, c1, s1, c2, s2, h1, h2;
    do_rotate(toren_data, sizeof(toren_data)/sizeof(GLfloat));
    h1 = buf[9];
    h2 = buf[9] + buf[9] - buf[3];
    for (i=0;i<ACC;i++)
    {
	if ((i*8/ACC)%2)
	{
	    a1 = ((GLfloat) i)*M_PI*2/ACC;
	    s1 = cos(a1);
	    c1 = sin(a1);
	    a2 = ((GLfloat) i+1)*M_PI*2/ACC;
	    s2 = cos(a2);
	    c2 = sin(a2);

	    glBegin(GL_QUADS);
	    glNormal3f(c1, 0.0, s1);
	    glVertex3f(0.143*c1, h1, 0.143*s1);
	    glVertex3f(0.143*c1, h2, 0.143*s1);
	    glNormal3f(c2, 0.0, s2);
	    glVertex3f(0.143*c2, h2, 0.143*s2);
	    glVertex3f(0.143*c2, h1, 0.143*s2);
	    glEnd();

	    glBegin(GL_QUADS);
	    glNormal3f(0.0, 1.0, 0.0);
	    glVertex3f(0.143*c1, h2, 0.143*s1);
	    glVertex3f(0.190*c1, h2, 0.190*s1);
	    glVertex3f(0.190*c2, h2, 0.190*s2);
	    glVertex3f(0.143*c2, h2, 0.143*s2);
	    glEnd();

	    glBegin(GL_QUADS);
	    glNormal3f(c1, 0.0, s1);
	    glVertex3f(0.190*c1, h1, 0.190*s1);
	    glVertex3f(0.190*c1, h2, 0.190*s1);
	    glNormal3f(c2, 0.0, s2);
	    glVertex3f(0.190*c2, h2, 0.190*s2);
	    glVertex3f(0.190*c2, h1, 0.190*s2);
	    glEnd();
	}
    }
    for (i=0;i<ACC;i++)
    {
	if (!((i*8) % ACC))
	{
	    a1 = ((GLfloat) i)*M_PI*2/ACC;
	    s1 = cos(a1);
	    c1 = sin(a1);
	    glBegin(GL_QUADS);
	    glNormal3f(s1, 0.0, -c1);
	    glVertex3f(c1*0.143, h1, s1*0.143);
	    glVertex3f(c1*0.190, h1, s1*0.190);
	    glVertex3f(c1*0.190, h2, s1*0.190);
	    glVertex3f(c1*0.143, h2, s1*0.143);
	    glEnd();
	}
    }
}

GLfloat paard_data[] = {
	-0.04, 1.950,	0.200, 1.950,	0.200, 1.950,	0.200, 1.550,
	0.200, 1.550,	0.650, 1.550,	0.650, 1.550,	0.700, 1.400,
	0.700, 1.100,
	0.850, 0.850,	1.050, 0.500,	1.050, 0.500,	1.050, 0.300,
	0.950, 0.350,	0.950, 0.150,	1.050, 0.150,	1.050, 0.000,
	1.050, 0.000,	0.000, 0.000,	0.000, 0.000
};

GLfloat paard_data2[] = {
	0.000, 1.600,	0.500, 1.600,	0.500, 1.600,	0.800, 2.800,
	0.870, 3.000,	0.550, 3.500,	0.000, 4.000,	-0.30, 4.300,
	-0.30, 4.300,	-0.50, 3.850,	-0.50, 3.850,	-0.85, 3.500,
	-0.85, 3.500,	-0.85, 3.200,	-0.85, 3.200,	-0.20, 3.000,
	-0.20, 3.000,	-0.45, 2.500,	-0.80, 2.350,	-0.80, 2.350,
	-0.65, 1.600,	0.000, 1.600
};

void do_paard(void)
{
    do_rotate(paard_data, sizeof(paard_data)/sizeof(GLfloat));
    do_solid(paard_data2, sizeof(paard_data2)/sizeof(GLfloat), 0.08);
}
    
GLfloat loper_data[] = {
	-0.20, 4.700,	0.000, 4.600,	0.250, 4.450,	0.150, 4.350,
	0.150, 4.350,	0.500, 3.900,	0.640, 3.500,	0.450, 3.100,
	0.450, 3.100,	0.580, 3.050,	0.450, 3.000,	0.450, 3.000,
	0.450, 2.700,	0.450, 2.700,	0.560, 2.650,	0.520, 2.600,
	0.520, 2.600,	0.700, 2.500,   0.740, 2.450,	0.700, 2.400,
	0.300, 2.300,	0.300, 2.300,	0.500, 1.150,	0.500, 1.150,
	0.550, 1.150,	0.550, 1.150,
	0.850, 0.850,	1.050, 0.500,	1.050, 0.500,	1.050, 0.300,
	0.950, 0.350,	0.950, 0.150,	1.050, 0.150,	1.050, 0.000,
	1.050, 0.000,	0.000, 0.000,	0.000, 0.000
};

void do_loper(void)
{
    do_rotate(loper_data, sizeof(loper_data)/sizeof(GLfloat));
}

GLfloat koning_data[] = {
	-0.20, 5.600,
	0.000, 5.600,	0.300, 5.600,	0.550, 5.400,	0.850, 5.400,
	0.554, 4.350,	0.554, 4.350,	0.650, 4.250,	0.550, 4.150,
	0.550, 4.150,	0.550, 3.900,	0.551, 3.900,	0.750, 3.650,
	0.750, 3.651,	0.751, 3.651,	0.920, 3.550,	0.500, 3.450,
	0.500, 3.450,
	0.500, 3.400,	0.650, 1.600,	0.700, 1.600,	0.750, 1.600,
	1.150, 0.850,	1.200, 0.800,	1.200, 0.500,	1.200, 0.500,
	1.100, 0.400,	1.200, 0.300,	1.200, 0.300,	1.200, 0.000,
	1.200, 0.000,	0.000, 0.000,	0.000, 0.000
};

GLfloat koning_data2[] = {
	0.000, 5.400,	0.200, 5.400,
	0.200, 5.400,	0.200, 7.000,	0.200, 7.000,	-0.20, 7.000,
	-0.20, 7.000,	-0.20, 5.400,	-0.20, 5.400,	0.000, 5.400,
};

GLfloat koning_data3[] = {
	0.000, 6.100,	0.700, 6.100,	0.700, 6.100,	0.700, 6.500,
	0.700, 6.500,	-0.70, 6.500,	-0.70, 6.500,	-0.70, 6.100,
	-0.70, 6.100,	0.000, 6.100
};

void prepare_koning(void)
{
/*
 * I used much data from the dame for the koning, but the koning it a little
 * bit bigger, so....
 */
    int i;
    for (i=0;i<sizeof(koning_data)/sizeof(GLfloat);i+=2)
    {
	koning_data[i] = koning_data[i] * 0.95;
	koning_data[i+1] = koning_data[i+1] * 1.05;
    }
    for (i=0;i<sizeof(koning_data2)/sizeof(GLfloat);i++)
	koning_data2[i] = koning_data2[i] * 1.05;
    for (i=0;i<sizeof(koning_data3)/sizeof(GLfloat);i++)
	koning_data3[i] = koning_data3[i] * 1.05;
}

void do_koning(void)
{
    do_rotate(koning_data, sizeof(koning_data)/sizeof(GLfloat));
    glRotatef(90.0, 0.0, 1.0, 0.0);
    do_solid(koning_data2, sizeof(koning_data2)/sizeof(GLfloat), 0.05);
    do_solid(koning_data3, sizeof(koning_data3)/sizeof(GLfloat), 0.05);
}

GLfloat dame_data[] = {
	-0.20, 6.000,	0.000, 6.000,	0.300, 5.850,	0.200, 5.700,
	0.300, 5.600,
	0.301, 5.600,	0.550, 5.400,	0.550, 5.400,	0.850, 5.400,
	0.800, 5.400,	0.800, 5.000,	0.800, 5.000,	0.650, 4.750,
	0.554, 4.350,	0.554, 4.350,	0.650, 4.250,	0.550, 4.150,
	0.550, 4.150,	0.550, 3.900,	0.551, 3.900,	0.750, 3.650,
	0.750, 3.651,	0.751, 3.651,	0.920, 3.550,	0.500, 3.450,
	0.500, 3.400,	0.650, 1.600,	0.700, 1.600,	0.750, 1.600,
	1.150, 0.850,	1.200, 0.800,	1.200, 0.500,	1.200, 0.500,
	1.100, 0.400,	1.200, 0.300,	1.200, 0.300,	1.200, 0.000,
	1.200, 0.000,	0.000, 0.000,	0.000, 0.000
};

void do_dame(void)
{
    do_rotate(dame_data, sizeof(dame_data)/sizeof(GLfloat));
}

void init_lists(void)
{
    int i,j;

    printf("Generating textures.\n");
    init_textures();

    for (i=0;i<17;i++)
	transl[i+12] = transl[44-i] = sin(((GLfloat) i)*M_PI/32)/1.5;

    for (i=0;i<17;i++)
	list[i+8] = list[40-i] = i*8;

    for (i=0;i<8;i++)
	list[40+i] = list[7-i] = i*8;

    printf("Generating display lists.\n");

    prepare_pion();
    prepare_koning();
    for (i=0;i<=16;i++)
    {
	phase = ((GLfloat) i) * M_PI/16.0;
	glNewList(PION + 8*i, GL_COMPILE);
	width[PION+8*i] = bwidth;
	height[PION+8*i] = bheight;
	do_pion();
	glEndList();

	glNewList(TOREN + 8*i, GL_COMPILE);
	do_toren();
	width[TOREN+8*i] = bwidth;
	height[TOREN+8*i] = bheight;
	glEndList();

	glNewList(PAARD + 8*i, GL_COMPILE);
	do_paard();
	width[PAARD+8*i] = bwidth;
	height[PAARD+8*i] = bheight;
	glEndList();

	glNewList(LOPER + 8*i, GL_COMPILE);
	do_loper();
	width[LOPER+8*i] = bwidth;
	height[LOPER+8*i] = bheight;
	glEndList();

	glNewList(KONING + 8*i, GL_COMPILE);
	do_koning();
	width[KONING+8*i] = bwidth;
	height[KONING+8*i] = bheight;
	glEndList();

	glNewList(DAME + 8*i, GL_COMPILE);
	do_dame();
	width[DAME+8*i] = bwidth;
	height[DAME+8*i] = bheight;
	glEndList();
    }

    for (i=0;i<10;i++)
	for (j=0;j<10;j++)
	{
	    board[i][j] = 0;
	    cycle[i][j] = -1;
	}

    for (i=1;i<9;i++)
    {
	board[i][2] = PION + WIT;
	board[i][7] = PION + ZWART;
    }
    board[1][1] = board[8][1] = TOREN + WIT;
    board[2][1] = board[7][1] = PAARD + WIT;
    board[3][1] = board[6][1] = LOPER + WIT;
    board[4][1] = DAME + WIT;
    board[5][1] = KONING + WIT;

    board[1][8] = board[8][8] = TOREN + ZWART;
    board[2][8] = board[7][8] = PAARD + ZWART;
    board[3][8] = board[6][8] = LOPER + ZWART;
    board[4][8] = DAME + ZWART;
    board[5][8] = KONING + ZWART;
    read_move();
}

extern int speed;

void do_piece(int pc, GLfloat x, GLfloat y, int *st, int *cl, int color)
{
    GLfloat a, s;

    if (*cl >= 0)
	(*cl)++;
    if (*cl < 0 && ((rand()%300) < 4) && chaos)
    {
	chaosPieces++;
	*cl = 0;
	*st = rand() % 6;
    }
    if (*cl >= 48) {
	chaosPieces--;
	if (chaosPieces == 0 && !chaos) {
	    if (!animating && (speed == 0))
		glutIdleFunc(NULL);
	}
	*cl = -1;
    }

    if (*cl < 0)
    {
	glPushMatrix();
	glTranslatef(x - 1.0, ((x==CX2 && y==CY2)?CZ2:0.0), 8.0 - y);
	if (color == ZWART && pc == PAARD)
	    glRotatef(180.0, 0.0, 1.0, 0.0);
	glScalef(1.2, 1.2, 1.2);
	glCallList(pc+list[0]);
	glPopMatrix();
	return;
    }

    glPushMatrix();
    switch (*st)
    {
	case 0:
	    glTranslatef(x - 1.0, transl[(*cl)>=0?*cl:0] +
		((x==CX2 && y==CY2)?CZ2:0.0), 8.0 - y);
	    if (color == ZWART && pc == PAARD)
		glRotatef(180.0, 0.0, 1.0, 0.0);
	    glScalef(1.2, 1.2, 1.2);
	    glCallList(list[(*cl)>=0?*cl:0]+pc);
	    break;
	case 1:
	case 2:
	    glTranslatef(x - 1.0, transl[(*cl)>=0?*cl:0] +
		((x==CX2 && y==CY2)?CZ2:0.0), 8.0 - y);
	    if (color == ZWART && pc == PAARD)
		glRotatef(180.0, 0.0, 1.0, 0.0);
	    if ((*cl > 16) && (*cl < 32))
	    {
		glTranslatef(0.0, height[list[*cl]+pc]/2, 0.0);
		if (*st == 1)
		    glRotatef(((*cl)-16) * 22.5, 1.0, 0.0, 0.0);
		else
		    glRotatef(-((*cl)-16) * 22.5, 1.0, 0.0, 0.0);
		glTranslatef(0.0, -height[list[*cl]+pc]/2, 0.0);
	    }
	    glScalef(1.2, 1.2, 1.2);
	    glCallList(list[*cl]+pc);
	    break;
	case 3:
	    glTranslatef(x - 1.0, ((x==CX2 && y==CY2)?CZ2:0.0), 8.0 - y);
	    if (color == ZWART && pc == PAARD)
		glRotatef(180.0, 0.0, 1.0, 0.0);
	    a = ((GLfloat) (*cl)) * M_PI / 12;
	    s = sin(a);
	    glRotatef(15*s, 0.0, 0.0, 1.0);
	    glTranslatef(0.0, width[list[0]+pc]*s*s, 0.0);
	    glScalef(1.2, 1.2, 1.2);
	    glCallList(list[0] + pc);
	    break;
	default:
	    glTranslatef(x - 1.0, ((x==CX2 && y==CY2)?CZ2:0.0), 8.0 - y);
	    if (color == ZWART && pc == PAARD)
		glRotatef(180.0, 0.0, 1.0, 0.0);
	    a = ((GLfloat) (*cl)) * M_PI / 12;
	    s = sin(a);
	    glRotatef(15*s, 0.0, 0.0, 1.0);
	    glRotatef((*cl) * 30, 0.0, 1.0, 0.0);
	    glTranslatef(0.0, width[list[0]+pc]*s*s, 0.0);
	    glScalef(1.2, 1.2, 1.2);
	    glCallList(list[0]+pc);
	    break;
    }
    glPopMatrix();
}

void do_pieces(void)
{
    int i,j;

    copper_texture();
    for (i=0;i<10;i++)
    {
	for (j=0;j<10;j++)
	{
	    if (board[i][j]&16 || !(board[i][j]&15))
		continue;
	    do_piece(board[i][j]&15, i, j, &stunt[i][j], &cycle[i][j], WIT);
	}
    }

    if ((piece&16) == WIT && piece > 0)
    {
	glPushMatrix();
	glTranslatef(0.0, 0.2, 0.0);
	do_piece(piece&15, CX1, CY1, &stuntm, &cyclem, WIT);
	glPopMatrix();
    }

    if ((piece2&16) == WIT && piece2 > 0)
	do_piece(piece2&15, CX2, CY1, &stunt2, &cycle2, WIT);

    dark_texture();
    for (i=0;i<10;i++)
    {
	for (j=0;j<10;j++)
	{
	    if (!(board[i][j]&16) || !board[i][j])
		continue;
	    do_piece(board[i][j]&15, i, j, &stunt[i][j], &cycle[i][j], ZWART);
	}
    }

    if ((piece&16) == ZWART && piece > 0)
    {
	glPushMatrix();
	glTranslatef(0.0, 0.2, 0.0);
	do_piece(piece&15, CX1, CY1, &stuntm, &cyclem, ZWART);
	glPopMatrix();
    }

    if ((piece2&16) == ZWART && piece2 > 0)
	do_piece(piece2&15, CX2, CY2, &stunt2, &cycle2, ZWART);
}

void do_display(void)
{
    glDisable(GL_DEPTH_TEST);
    /* glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE); */
    if (reflection) {
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
    }
    do_vlakje();
    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if (reflection)
    {
	glStencilFunc(GL_EQUAL, 1, 0xffffffff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glPushMatrix();
	glScalef(1.0, -1.0, 1.0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glClipPlane(GL_CLIP_PLANE1, ClipPlane);
	glEnable(GL_CLIP_PLANE1);
	do_pieces();
	glPopMatrix();
	glDisable(GL_CLIP_PLANE1);
        glDisable(GL_STENCIL_TEST);

	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    }

/*
 * Also without texturing I want to blend, to keep the contrast of the board
 * consistent.
 */

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    do_board();

    glDisable(GL_BLEND);

    do_border();

    glClipPlane(GL_CLIP_PLANE1, ClipPlane);
    glEnable(GL_CLIP_PLANE1);
    do_pieces();
    glDisable(GL_CLIP_PLANE1);
}
