
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* cview2smap.c - warp 6 cube views into sphere map */

#include <math.h>
#include <stdio.h>
#include <GL/glut.h>

#include "smapmesh.h"

#if defined(GL_EXT_texture_object) && !defined(GL_VERSION_1_1)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#endif
#if defined(GL_EXT_copy_texture) && !defined(GL_VERSION_1_1)
#define glCopyTexImage2D(A, B, C, D, E, F, G, H)    glCopyTexImage2DEXT(A, B, C, D, E, F, G, H)
#endif

GLfloat up[3] = { 0, 1, 0 };
GLfloat eye[3] = { 0, 0, -20 };
GLfloat obj[3] = { 0, 0, 0 };
int outline = 0;
int bufswap = 1;
int texdim = 128;

static int W, H;

int angle = 0;
int downx, downy;

void
reshape(int w, int h)
{
	W = w;
	H = h;
}

void
drawView(int drawCenterObject)
{
	/* right green small cube (+5,0,-8) */
	glPushMatrix();
	  glTranslatef(5.0, 0.0, -8.0);
	  glRotatef(-45, 1.0, 0.0, 1.0);
	  glColor3f(0.0, 1.0, 0.0);
	  glutSolidCube(2.0);
	glPopMatrix();
	/* left red cube (-5,0,-8) */
	glPushMatrix();
	  glTranslatef(-5.0, 0.0, -8.0);
	  glRotatef(45, 1.0, 0.0, 1.0);
	  glColor3f(1.0, 0.0, 0.0);
	  glutSolidCube(6.0);
	glPopMatrix();
	/* left blue cube (-7,0,0); */
	glPushMatrix();
	  glTranslatef(-7.0, 0.0, 0.0);
	  glColor3f(0.0, 0.0, 1.0);
	  glutSolidCube(5.0);
	glPopMatrix();
	/* right cyan big cube (+7,0,0) */
	glPushMatrix();
	  glTranslatef(7.0, 0.0, 0.0);
	  glRotatef(30, 1.0, 1.0, 0.0);
	  glColor3f(0.0, 1.0, 1.0);
	  glutSolidCube(5.0);
	glPopMatrix();
	/* distant yellow sphere (0,0,+10) */
	glPushMatrix();
	  glTranslatef(0.0, 0.0, 10.0);
	  glColor3f(1.0, 1.0, 0.0);
	  glutSolidSphere(7.0, 8, 8);
	glPopMatrix();
	/* top white sphere (0,+5,0) */
	glPushMatrix();
	  glTranslatef(0.0, 5.0, 0.0);
	  glColor3f(1.0, 1.0, 1.0);
	  glutSolidSphere(2.0, 8, 8);
	glPopMatrix();
	/* bottom magenta big sphere (0,-6,0) */
	glPushMatrix();
	  glTranslatef(0.0, -6.0, 0.0);
	  glColor3f(1.0, 0.0, 1.0);
	  glutSolidSphere(4.0, 8, 8);
	glPopMatrix();

	if (drawCenterObject) {
		glPushMatrix();
			glScalef(3.0, 3.0, 3.0);
			glColor3f(1.0, 1.0, 1.0);
			glutSolidIcosahedron();
		glPopMatrix();
	}
}

void
setAreaAndClear(GLint x, GLint y, GLsizei w, GLsizei h)
{
    glViewport(x, y, w, h);
	glScissor(x, y, w, h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* You need to reposition the lights everytime view */
/* transformation changes. */
void
positionLights(void)
{
	static GLfloat light1Pos[4] = { -41.0, 41.0, -41.0, 1.0 };
	static GLfloat light2Pos[4] = { +41.0, 0.0, -41.0, 1.0 };
	static GLfloat light3Pos[4] = { -41.0, 0.0, +41.0, 1.0 };
	static GLfloat light4Pos[4] = { +41.0, 0.0, +41.0, 1.0 };

	glLightfv(GL_LIGHT0, GL_POSITION, light1Pos);
	glLightfv(GL_LIGHT1, GL_POSITION, light2Pos);
	glLightfv(GL_LIGHT2, GL_POSITION, light3Pos);
	glLightfv(GL_LIGHT3, GL_POSITION, light4Pos);
}

void
snagImageAsTexture(GLuint texobj)
{
	glBindTexture(GL_TEXTURE_2D, texobj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	/* Clamp to avoid artifacts from wrap around in texture. */
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
		texdim, texdim, 0);
}

struct {
	GLfloat angle;
	GLfloat x, y, z;
} faceInfo[6] = {
	{   0.0, +1.0,  0.0,  0.0 },  /* front */
	{  90.0, -1.0,  0.0,  0.0 },  /* top */
	{  90.0, +1.0,  0.0,  0.0 },  /* bottom */
	{  90.0,  0.0, -1.0,  0.0 },  /* left */
	{  90.0,  0.0, +1.0,  0.0 },  /* right */
	{ 180.0, -1.0,  0.0,  0.0 }   /* back */
};

void
configFace(int i)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(faceInfo[i].angle,
		faceInfo[i].x, faceInfo[i].y, faceInfo[i].z);
	gluLookAt(obj[0], obj[1], obj[2],  /* "eye" at object */
		      eye[0], eye[1], eye[2],  /* looking at eye */
			  up[0], up[1], up[2]);
	positionLights();
}

void
display(void)
{
	static GLuint texobjs[6] = { 1, 2 ,3, 4, 5, 6 };
	int i;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1.0, 0.5, 40.0);
    glViewport(0, 0, texdim, texdim);
	glScissor(0, 0, texdim, texdim);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	/* front view (center) */
	for (i=0; i<6; i++) {
		configFace(i);
		drawView(0);
		snagImageAsTexture(1+i);
	}

	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, W, H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	drawSphereMapMesh(texobjs);
	glDisable(GL_TEXTURE_2D);
	if (outline) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_LIGHTING);
		glColor3f(1.0, 1.0, 1.0);
		drawSphereMapMesh(texobjs);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_LIGHTING);
	}

	glDisable(GL_SCISSOR_TEST);

	/* initial clear */
	glViewport(0, 0, W, H);

	if (bufswap) {
		glutSwapBuffers();
	}
}

void
menu(int value)
{
	switch (value) {
	case 1:
		glEnable(GL_LIGHTING);
		printf("enable\n");
		break;
	case 2:
		glDisable(GL_LIGHTING);
		printf("disable\n");
		break;
	case 3:
		outline = 1;
		break;
	case 4:
		outline = 0;
		break;
	case 5:
		glDrawBuffer(GL_FRONT);
		glReadBuffer(GL_FRONT);
		bufswap = 0;
		break;
	case 6:
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);
		bufswap = 1;
		break;
	case 7:
		texdim = 64;
		break;
	case 8:
		texdim = 128;
		break;
	case 9:
		texdim = 256;
		break;
	}
	glutPostRedisplay();
}

void
motion(int x, int y)
{
	angle += (x - downx);
	angle = angle % 360;
	eye[0] = sin(angle * 3.14159/180.0) * -20;
	eye[2] = cos(angle * 3.14159/180.0) * -20;
	eye[1] = eye[1] + 0.1 * (y - downy);
	if (eye[1] > +25) eye[1] = +25;
	if (eye[1] < -25) eye[1] = -25;
	glutPostRedisplay();
	downx = x;
	downy = y;
}

void
mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			downx = x;
			downy = y;
		}
	}
}

void
initGraphicsState(void)
{
    GLfloat lightColor[4] = { 0.6, 0.6, 0.6, 1.0 };  /* white */

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT3, GL_DIFFUSE, lightColor);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glEnable(GL_LIGHT3);
	glEnable(GL_DEPTH_TEST);
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);
}

int
main(int argc, char **argv)
{
	makeSphereMapMesh();

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("cview2smap");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	initGraphicsState();

	glutCreateMenu(menu);
	glutAddMenuEntry("light on", 1);
	glutAddMenuEntry("light off", 2);
	glutAddMenuEntry("outline", 3);
	glutAddMenuEntry("no outline", 4);
	glutAddMenuEntry("single buffer", 5);
	glutAddMenuEntry("double buffer", 6);
	glutAddMenuEntry("face tex dim = 64", 7);
	glutAddMenuEntry("face tex dim = 128", 8);
	glutAddMenuEntry("face tex dim = 256", 9);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();
	return 0;
}
