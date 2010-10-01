#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include "texture.h"
#include <stdlib.h>

#ifndef __sgi
/* Most math.h's do not define float versions of the math functions. */
#define sqrtf(x) (float)sqrt((x))
#endif

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture glBindTextureEXT
#endif

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

/* display lists */
enum {SPHERE = 1, CONE, LIGHT, DISK, FLOOR, BACK, LEFT, RIGHT, CEIL};

/* texture objects */
enum {DEFAULT, LIGHTMAP, SURFMAP}; 

enum {NONE, LIGHT_XY, LIGHT_Z, LIGHT_INTENS};

#ifndef TRUE
enum {FALSE, TRUE};
#endif
enum {X, Y, Z, W};
enum {R, G, B, A};

GLfloat staticlightpos[] = {50.f, 70.f, -10.f, 1.f};
GLfloat lightpos[] = {50.f, 70.f, -10.f, 1.f};
GLfloat intensity = 1.f;
GLfloat nearScale = .49f, farScale = .0001f;

int dblbuf = TRUE;
int action = NONE;
int winWidth = 512;
int winHeight = 512;
int curtess = 1; /* current tessellation level */
int lasttess = 1; /* last set tessellation level */

void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
}

void
motion(int x, int y)
{
    switch(action)
    {
    case LIGHT_XY:
	lightpos[X] = (x - winWidth/2) * 200.f/winWidth;
	lightpos[Y] = (winHeight/2 - y) * 200.f/winHeight;
	glutPostRedisplay();
	break;
    case LIGHT_Z:
	lightpos[Z] = (winHeight/2 - y) * 200.f/winWidth;
	glutPostRedisplay();
	break;
    case LIGHT_INTENS:
	intensity = x/(GLfloat)winWidth;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* move the light */
	    action = LIGHT_XY;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    action = LIGHT_INTENS;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    action = LIGHT_Z;
	    motion(x, y);
	    break;
	}
}


/*
** Create a single component texture map
*/
GLfloat *make_texture(int maxs, int maxt)
{
    int s, t;
    GLfloat *texture;

    texture = (GLfloat *)malloc(maxs * maxt * sizeof(GLfloat));
    for(t = 0; t < maxt; t++) {
	for(s = 0; s < maxs; s++) {
	    texture[s + maxs * t] = ((s >> 3) & 0x1) ^ ((t >> 3) & 0x1);
	}
    }
    return texture;
}

/* make tesselated surface for display list dlist */
/* normal tells you surface position and orientation */
/* count = tesselation count */
void
tess_surface(GLuint dlist, int count)
{
    int i, j;
    GLfloat x0, y0, z0;
    GLfloat over[3], up[3]; /* axes */
    int Sindex, Tindex; /* active texture axes */

    glNewList(dlist, GL_COMPILE);
    glBegin(GL_QUADS);

    switch(dlist)
    {
    case FLOOR:
	glNormal3f( 0.f,  1.f, 0.f);
	x0 = -100.f; y0 = -100.f; z0 = 100.f;
	over[X] = 1.f/count; over[Y] = 0.f; over[Z] = 0.f;
	up[X] = 0.f; up[Y] = 0.f; up[Z] = -1.f/count;
	Sindex = X; Tindex = Z;
	break;
    case CEIL:
	glNormal3f( 0.f, -1.f, 0.f);
	x0 = -100.f; y0 = 100.f; z0 = 100.f;
	over[X] = 0.f; over[Y] = 0.f; over[Z] = -1.f/count;
	up[X] = 1.f/count; up[Y] = 0.f; up[Z] = 0.f;
	Sindex = X; Tindex = Z;
	break;
    case LEFT:
	glNormal3f( 1.f,  0.f, 0.f);
	x0 = -100.f; y0 = -100.f; z0 = 100.f;
	over[X] = 0.f; over[Y] = 0.f; over[Z] = -1.f/count; 
	up[X] = 0.f; up[Y] = 1.f/count; up[Z] = 0.f;
	Sindex = Z; Tindex = Y;
	break;
    case RIGHT:
	glNormal3f(-1.f,  0.f, 0.f);
	x0 =  100.f; y0 = -100.f; z0 = 100.f;
	over[X] = 0.f; over[Y] = 1.f/count; over[Z] = 0.f;
	up[X] = 0.f; up[Y] = 0.f; up[Z] = -1.f/count; 
	Sindex = Z; Tindex = Y;
	break;
    case BACK:
	glNormal3f( 0.f,  0.f, 1.f);
	x0 = -100.f; y0 = -100.f; z0 = -100.f;
	over[X] = 1.f/count; over[Y] = 0.f; over[Z] = 0.f;
	up[X] = 0.f; up[Y] = 1.f/count; up[Z] = 0.f;
	Sindex = X; Tindex = Y;
	break;
    default:
	fprintf(stderr, "tess_surface(): bad display list argument %d\n",
		dlist);
	glEnd();
	glEndList();
	return;
    }

    for(j = 0; j < count; j++)
	for(i = 0; i < count; i++)
	{
	    glTexCoord2f(i * over[Sindex], j * up[Tindex]);
	    glVertex3f(x0 + i * 200 * over[X] + j * 200 * up[X],
		       y0 + i * 200 * over[Y] + j * 200 * up[Y],
		       z0 + i * 200 * over[Z] + j * 200 * up[Z]);
	    glTexCoord2f((i + 1) * over[Sindex], j * up[Tindex]);
	    glVertex3f(x0 + (i + 1) * 200 * over[X] + j * 200 * up[X],
		       y0 + (i + 1) * 200 * over[Y] + j * 200 * up[Y],
		       z0 + (i + 1) * 200 * over[Z] + j * 200 * up[Z]);
	    glTexCoord2f((i + 1) * over[Sindex], (j + 1) * up[Tindex]);
	    glVertex3f(x0 + (i + 1) * 200 * over[X] + (j + 1) * 200 * up[X],
		       y0 + (i + 1) * 200 * over[Y] + (j + 1) * 200 * up[Y],
		       z0 + (i + 1) * 200 * over[Z] + (j + 1) * 200 * up[Z]);
	    glTexCoord2f(i * over[Sindex], (j + 1) * up[Tindex]);
	    glVertex3f(x0 + i * 200 * over[X] + (j + 1) * 200 * up[X],
		       y0 + i * 200 * over[Y] + (j + 1) * 200 * up[Y],
		       z0 + i * 200 * over[Z] + (j + 1) * 200 * up[Z]);
	}
    glEnd();
    glEndList();
}





static GLfloat zero[] = {0.f, 0.f, 0.f, 1.f};
static GLfloat one[] = {1.f, 1.f, 1.f, 1.f};
static GLfloat diff[] = {.25f, .25f, .25f, .25f};


/* create a lightmap simulating a local light with attenuation */
void
make_lightmap(GLenum light, int dim)
{
    GLfloat quadratic, linear, constant;
    GLfloat diffuse[4], ambient[4]; /* light color */
    GLfloat *texture;
    GLfloat dist, scale, edge;
    int size;
    int i, j;

    glPushAttrib(GL_TEXTURE_BIT);
    /* get from light to simplify api */
    glGetLightfv(light, GL_QUADRATIC_ATTENUATION, &quadratic);
    glGetLightfv(light, GL_LINEAR_ATTENUATION, &linear);
    glGetLightfv(light, GL_CONSTANT_ATTENUATION, &constant);

    glGetLightfv(light, GL_AMBIENT, ambient);
    glGetLightfv(light, GL_DIFFUSE, diffuse);

    size = dim + 2;
    texture = (GLfloat *)malloc(sizeof(GLfloat) * 3 * size * size);

    dist = dim/2; /* 1 in from border */
    edge = 1.f/(constant + linear * dist + quadratic * dist * dist);

    for(j = 0; j < size; j++)
	for(i = 0; i < size; i++)
	{
	    dist = sqrtf((float)((size/2.f - i) * (size/2.f - i) +
				 (size/2.f - j) * (size/2.f - j)));
	    scale = 1.f/(constant + linear * dist + quadratic * dist * dist);
	    if(dist >= dim/2)
	    {
		texture[3 * (i + size * j) + 0] = 
		    diffuse[R] * edge + ambient[R];
		texture[3 * (i + size * j) + 1] = 
		    diffuse[G] * edge + ambient[G];
		texture[3 * (i + size * j) + 2] = 
		    diffuse[B] * edge + ambient[B];
	    }
	    else
	    {
		texture[3 * (i + size * j) + 0] = 
		    diffuse[R] * scale + ambient[R];
		texture[3 * (i + size * j) + 1] = 
		    diffuse[G] * scale + ambient[G];
		texture[3 * (i + size * j) + 2] = 
		    diffuse[B] * scale + ambient[B];
	    }
	}

    glBindTexture(GL_TEXTURE_2D, LIGHTMAP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
		 size, size, 1, GL_RGB, GL_FLOAT, texture);
    free(texture);
    glPopAttrib();
}

/* draw a highly tesselated disk with local light */
void
draw_lightmap(int dim, int dist)
{
    GLUquadricObj *qobj;
    GLfloat *texture;
    GLfloat light[4] = {0.f, 0.f, 1.f, 1.f};

    texture = (GLfloat *)malloc(sizeof(GLfloat) * 3 * dim * dim);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushMatrix(); /* starts as modelview */
    glLoadIdentity();
    glTranslatef(0.f, 0.f, -dist);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-dim/2., dim/2., -dim/2., dim/2., 0., (double)dist);

    glPushAttrib(GL_LIGHTING_BIT|GL_VIEWPORT_BIT);
    glViewport(0, 0, dim, dim);
    glEnable(GL_LIGHTING);

    light[Z] = dist;
    glLightfv(GL_LIGHT0, GL_POSITION, light); /* light position */

    /* XXX TODO, range check an report errors */
    glDisable(GL_TEXTURE_2D);

    qobj = gluNewQuadric();
    gluDisk(qobj, 0., dim/2. * sqrt(2.), dim/4, dim/4);
    gluDeleteQuadric(qobj);

    glEnable(GL_TEXTURE_2D);
    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glReadPixels(0, 0, dim, dim, GL_RGB, GL_FLOAT, texture);

    glBindTexture(GL_TEXTURE_2D, LIGHTMAP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dim, dim, 0, GL_RGB,
		 GL_FLOAT, texture);

    free(texture);
}


int texdim = 256;


/* draw the lightmap texture */
void redraw_lightmap(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT);

    /* assume GL_MODELVIEW */
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, LIGHTMAP);
    glDisable(GL_LIGHTING);

    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2i(-1, -1);

    glTexCoord2i(1, 0);
    glVertex2i(1, -1);

    glTexCoord2i(1, 1);
    glVertex2i(1,  1);

    glTexCoord2i(0, 1);
    glVertex2i(-1,  1);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();

    CHECK_ERROR("OpenGL Error in redraw_map()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/* draw the lightmap texture */
void redraw_combomap(void)
{
    GLfloat scale;
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|
		 GL_DEPTH_BUFFER_BIT);

    /* assume GL_MODELVIEW */
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);


    glBindTexture(GL_TEXTURE_2D, SURFMAP);

    glColor3f(intensity, intensity, intensity);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2i(-1, -1);

    glTexCoord2i(5, 0);
    glVertex2i(1, -1);

    glTexCoord2i(5, 5);
    glVertex2i(1,  1);

    glTexCoord2i(0, 5);
    glVertex2i(-1,  1);
    glEnd();

    glEnable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    glBindTexture(GL_TEXTURE_2D, LIGHTMAP);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(.5f, .5f, 0.f);
    scale = .25f + (lightpos[Z] + 100.f) * 3.75f/200.f;
    glScalef(scale, scale, 0.f);
    glTranslatef(-.5f, -.5f, 0.f);
    glTranslatef(-lightpos[X]/200.f, -lightpos[Y]/200.f, 0.f);

    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2i(-1, -1);

    glTexCoord2i(1, 0);
    glVertex2i(1, -1);

    glTexCoord2i(1, 1);
    glVertex2i(1,  1);

    glTexCoord2i(0, 1);
    glVertex2i(-1,  1);
    glEnd();

    /* GL_TEXTURE */
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();

    CHECK_ERROR("OpenGL Error in redraw_combomap()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/* draw the openGL scene with lighting off */
void render_scene(int tess)
{
    /* material properties for objects in scene */

    /* update tessellation if it changed */
    if(tess != lasttess)
    {
	tess_surface(FLOOR, tess);
	tess_surface(CEIL,  tess);
	tess_surface(LEFT,  tess);
	tess_surface(RIGHT, tess);
	tess_surface(BACK,  tess);
	lasttess = tess;
    }
    glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT);

    /* floor */
    glColor3f(.5f, .35f, .35f); /* reddish */
    glEnable(GL_TEXTURE_2D);
    glCallList(FLOOR);
    glDisable(GL_TEXTURE_2D);

    /* ceiling */
    glColor3f(.35f, .5f, .35f); /* greenish */
    glCallList(CEIL);

    /* right wall */
    glColor3f(.35f, .35f, .5f); /* bluish */
    glCallList(RIGHT);

    /* left wall */
    glCallList(LEFT);

    /* back wall */
    glCallList(BACK);

    /* draw the sphere */
    
    glCallList(SPHERE);

    /* draw the cone */
    glCallList(CONE);

    /* draw the light */
    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glCallList(LIGHT);
    glPopMatrix();

    glPopAttrib();
    CHECK_ERROR("OpenGL Error in render_unlit()");
}

void
redraw_opengl(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPushAttrib(GL_LIGHTING_BIT);

    glEnable(GL_LIGHTING);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos); /* opengl lit */ 


    render_scene(curtess);

    glPopAttrib();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}


void
redraw_unlit(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPushAttrib(GL_LIGHTING_BIT);

    glDisable(GL_LIGHTING);
    render_scene(1);

    glPopAttrib();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}


/* draw the scene white, modulated by lightmap */
/* unlit, uncolored simulated by lighting */
/* texgen overrides texture coords */
void render_white(void)
{
    GLfloat scale;

    /* texgen used to override texure coords */
    static GLfloat XZs[] = {1/200.f, 0.f, 0.f, .5f};
    static GLfloat XZt[] = {0.f, 0.f, 1/200.f, .5f};

    static GLfloat YZs[]  = {0.f, 0.f, 1/200.f, .5f};
    static GLfloat YZt[]  = {0.f, 1/200.f, 0.f, .5f};

    static GLfloat XYs[]  = {1/200.f, 0.f, 0.f, .5f};
    static GLfloat XYt[]  = {0.f, 1/200.f, 0.f, .5f};

    /* material properties for objects in scene */

    glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT);

    glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    glEnable(GL_LIGHTING);

    /* simulate unlit, color = white */
    glLightfv(GL_LIGHT0, GL_AMBIENT, diff);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, zero);
    glColorMaterial(GL_FRONT, GL_AMBIENT);
    glEnable(GL_COLOR_MATERIAL);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, LIGHTMAP);
    
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();

    glTexGenfv(GL_S, GL_OBJECT_PLANE, XZs);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, XZt);

    glLoadIdentity();
    glTranslatef(.5f, .5f, 0.f);
    scale = nearScale - (nearScale - farScale)/200.f * (100.f + lightpos[Y]);
    glScalef(scale, scale, 1.f);
    glTranslatef(-.5f, -.5f, 0.f);
    glTranslatef(-lightpos[X]/200.f, -lightpos[Z]/200.f, 0.f);
    /* add intensity (colorchange) */

    scale = lightpos[Y] + 100;
    scale /= 200.f;
    scale *= scale;
    scale = 1.f/(1.f + scale);
    scale = 1.f;
    glColor3f(scale, scale, scale);

    /* floor */
    glCallList(FLOOR);

    glLoadIdentity();
    glTranslatef(.5f, .5f, 0.f);
    scale = nearScale - (nearScale - farScale)/200.f * (100.f - lightpos[Y]);
    glScalef(scale, scale, 1.f);
    glTranslatef(-.5f, -.5f, 0.f);
    glTranslatef(-lightpos[X]/200.f, -lightpos[Z]/200.f, 0.f);

    scale = 100 - lightpos[Y];
    scale /= 100.f;
    scale *= scale;
    scale = 1.f/(1.f + scale);
    scale = 1.f;
    glColor3f(scale, scale, scale);

    /* ceiling */
     glCallList(CEIL);

    glTexGenfv(GL_S, GL_OBJECT_PLANE, YZs);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, YZt);
    glLoadIdentity();
    glTranslatef(.5f, .5f, 0.f);
    scale = nearScale - (nearScale - farScale)/200.f * (100.f - lightpos[X]);
    glScalef(scale, scale, 1.f);
    glTranslatef(-.5f, -.5f, 0.f);
    glTranslatef(-lightpos[Z]/200.f, -lightpos[Y]/200.f, 0.f);

    scale = 100 - lightpos[X];
    scale /= 100.f;
    scale *= scale;
    scale = 1.f/(1.f + scale);
    scale = 1.f;
    glColor3f(scale, scale, scale);

    /* right wall */
    glCallList(RIGHT);

    glLoadIdentity();
    glTranslatef(.5f, .5f, 0.f);
    scale = nearScale - (nearScale - farScale)/200.f * (100.f + lightpos[X]);
    glScalef(scale, scale, 1.f);
    glTranslatef(-.5f, -.5f, 0.f);
    glTranslatef(-lightpos[Z]/200.f, -lightpos[Y]/200.f, 0.f);

    scale = lightpos[X] + 100;
    scale /= 100.f;
    scale *= scale;
    scale = 1.f/(1.f + scale);
    scale = 1.f;
    glColor3f(scale, scale, scale);

    /* left wall */
    glCallList(LEFT);

    glTexGenfv(GL_S, GL_OBJECT_PLANE, XYs);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, XYt);
    glLoadIdentity();
    glTranslatef(.5f, .5f, 0.f);
    scale = nearScale - (nearScale - farScale)/200.f * (100.f + lightpos[Z]);
    glScalef(scale, scale, 1.f);
    glTranslatef(-.5f, -.5f, 0.f);
    glTranslatef(-lightpos[X]/200.f, -lightpos[Y]/200.f, 0.f);

    scale = lightpos[Z] + 100;
    scale /= 100.f;
    scale *= scale;
    scale = 1.f/(1.f + scale);
    scale = 1.f;
    glColor3f(scale, scale, scale);

    /* back wall */
    glCallList(BACK);

    /* done with texture matrix */
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    /* no contribution to highly tesselated objects */
    glMaterialfv(GL_FRONT, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT0, GL_AMBIENT, zero);

    /* draw the sphere */
    
    glCallList(SPHERE);

    /* draw the cone */
    glCallList(CONE);

    glPopAttrib();
    CHECK_ERROR("OpenGL Error in render_white()");
}

void
redraw_white(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    render_white();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}


void
redraw_lightmapped(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    render_scene(1);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    glDepthFunc(GL_LEQUAL);
    render_white();
    glPopAttrib();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

void
redraw_maponly(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    render_scene(1);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    glDepthFunc(GL_LEQUAL);
    render_white();
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_LIGHTING); /* add in unlit scene again */
    render_scene(1);
    
    glPopAttrib();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

void
redraw_complete(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_LIGHTING_BIT);

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos); /* opengl lit */ 
    glDisable(GL_LIGHTING);
    render_scene(1);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    glDepthFunc(GL_LEQUAL);
    render_white();
    glBlendFunc(GL_ONE, GL_ONE);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    render_scene(1);

    glPopAttrib();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}



/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'l': /* show lightmap */
	glutDisplayFunc(redraw_lightmap);
	glutPostRedisplay();
	break;
    case 'w': /* show white scene modulated by lightmap */
	glutDisplayFunc(redraw_white);
	glutPostRedisplay();
	break;
    case 'm': /* unlit scene modified by lightmap */
	glutDisplayFunc(redraw_lightmapped);
	glutPostRedisplay();
	break;
    case 'u': /* draw scene without lighting */
	glutDisplayFunc(redraw_unlit);
	glutPostRedisplay();
	break;
    case 'o': /* draw scene lit using OpenGL */
	glutDisplayFunc(redraw_opengl);
	glutPostRedisplay();
	break;
    case 'a': /* show scene lit with OpenGL and lightmaps */
	glutDisplayFunc(redraw_complete);
	glutPostRedisplay();
	break;
    case 'p': /* show scene lit with pure lightmaps */
	glutDisplayFunc(redraw_maponly);
	glutPostRedisplay();
	break;
    case 'x': /* surface texture with lightmap combined */
	glutDisplayFunc(redraw_combomap);
	glutPostRedisplay();
	break;
    case 'T': /* surface texture with lightmap combined */
	curtess++;
	glutPostRedisplay();
	break;
    case 't': /* surface texture with lightmap combined */
	curtess--;
	if(curtess < 1)
	    curtess = 1;
	glutPostRedisplay();
	break;
    case '1': /* surface texture with lightmap combined */
	curtess = 1;
	glutPostRedisplay();
	break;
    case 'y':
	farScale -= .0001f;
	if(farScale < .0001f)
	    farScale = .0001f;
	printf("farScale = %.4f\n", farScale);
	glutPostRedisplay();
	break;
    case 'Y':
	farScale += .0001f;
	printf("farScale = %.4f\n", farScale);
	glutPostRedisplay();
	break;
    case 'z':
	nearScale -= .01f;
	if(nearScale < .01f)
	    nearScale = .01f;
	printf("nearScale = %.2f\n", nearScale);
	glutPostRedisplay();
	break;
    case 'Z':
	nearScale += .01f;
	printf("nearScale = %.2f\n", nearScale);
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
	break;
    case '?':
    case 'h':
    default:
	fprintf(stderr, 
		"Keyboard Commands\n"
		"l - draw lightmap\n"
		"w - draw white scene modulated by lightmap\n"
		"m - draw unlit scene modified by lightmap\n"
		"u - draw unlit scene\n"
		"o - draw scene lit by OpenGL\n"
		"a - draw scene lit by OpenGL (tess = 1) + lightmaps\n"
		"p - draw scene lit only by lightmaps\n"
		"x - interactive lightmap on brick wall\n"
		"T - increase surface tessellation\n"
		"t - decrease surface tessellation\n"
		"1 - set tessellation to one\n"
		"y - decrease far scale for lightmaps\n"
		"Y - increase far scale for lightmaps\n"
		"z - decrease near scale for lightmaps\n"
		"Z - increase near scale for lightmaps\n");
	break;
    }
    glutPostRedisplay();
}




main(int argc, char *argv[])
{
    GLfloat *tex;

    GLUquadricObj *qobj;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	int done = FALSE;
	while(!done)
	{
	    switch(*args)
	    {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = TRUE;
		break;
	    }
	    args++;
	}
    }

    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);


    (void)glutCreateWindow("example program");
    glutDisplayFunc(redraw_opengl);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 300., 501.); 
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 400) */
    gluLookAt(0., 0., 400., 0., 0., 0., 0., 1., 0.);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, staticlightpos);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);


    /* make a display list containing a sphere */
    glNewList(SPHERE, GL_COMPILE);
    {
	glPushMatrix();
	glTranslatef(0.f, -80.f, -80.f);
	qobj = gluNewQuadric();
	gluQuadricTexture(qobj, GL_TRUE);
	glColor3f(.5f, .25f, 0.f);
	gluSphere(qobj, 20.f, 20, 20);
	gluDeleteQuadric(qobj);
	glPopMatrix();
    }
    glEndList();

    /* make a display list containing a sphere */
    glNewList(LIGHT, GL_COMPILE);
    {
	qobj = gluNewQuadric();
	glColor3f(1.f, 1.f, 1.f);
	glPushAttrib(GL_LIGHTING_BIT);
	glDisable(GL_LIGHTING);
	gluSphere(qobj, 3.f, 20, 20);
	glPopAttrib();
	gluDeleteQuadric(qobj);
    }
    glEndList();

    /* create a display list containing a cone */
    glNewList(CONE, GL_COMPILE);
    {
	glPushMatrix();
	glTranslatef(-60.f, -100.f, -5.f);

	qobj = gluNewQuadric();
	gluQuadricTexture(qobj, GL_TRUE);
	glRotatef(-90.f, 1.f, 0.f, 0.f);
	glColor3f(0.f, .25f, .5f);

	gluCylinder(qobj, 20., 0., 60., 20, 20);
	gluDeleteQuadric(qobj);

	qobj = gluNewQuadric();
	gluQuadricOrientation(qobj, GLU_INSIDE);
	gluDisk(qobj, 0., 20., 20, 1);
	gluDeleteQuadric(qobj);
	glPopMatrix();
    }
    glEndList();

    /* load pattern for current 2d texture */
    tex = make_texture(128, 128);

    /* makes texturing faster, and looks better than GL_LINEAR */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, 128, 128, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    {
	GLuint *surftex;
	int texwid, texht, texcomps;
	surftex = read_texture("../data/brick.rgb", &texwid, &texht, &texcomps);

	glBindTexture(GL_TEXTURE_2D, SURFMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwid, texht, 0, GL_RGBA,
		     GL_UNSIGNED_BYTE, surftex);

    }

    glBindTexture(GL_TEXTURE_2D, DEFAULT);

    tess_surface(FLOOR, 1);
    tess_surface(CEIL, 1);
    tess_surface(LEFT, 1);
    tess_surface(RIGHT, 1);
    tess_surface(BACK, 1);

    /* 1/80 intensity at edges */
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 100.f/(texdim * texdim/4));
    /* 1/2 intensity at edges */
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 1.f/texdim/2);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, one);

    make_lightmap(GL_LIGHT1, 128);

    key('?', 0, 0);

    CHECK_ERROR("end of main");

    glutMainLoop();

    return 0;
}

