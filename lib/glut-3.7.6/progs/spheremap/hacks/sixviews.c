
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* cubeview.c - shows 6 cube face views and eye view */

#include <math.h>
#include <stdio.h>
#include <GL/glut.h>

GLfloat up[3] = { 0, 1, 0 };
GLfloat eye[3] = { 0, 0, -20 };
GLfloat obj[3] = { 0, 0, 0 };

static int W, H, w3, h3;

int angle = 0;
int downx, downy;

void
reshape(int w, int h)
{
	W = w;
	H = h;

	w3 = W/3;
	h3 = H/3;
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
setViewArea(GLint x, GLint y, GLsizei w, GLsizei h)
{
    glViewport(x, y, w, h);
	glScissor(x, y, w, h);
}

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
	/* initial clear */
	glViewport(0, 0, W, H);
	glClearColor(0.5, 0.5, 0.5, 0.5);  /* clear to gray */
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* eye view (bottom right) */
	setViewArea(2*w3+4, 4, w3 - 8, h3 - 8);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1.0, 0.5, 40.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(eye[0], eye[1], eye[2],  /* eye at eye */
		      obj[0], obj[1], obj[2],  /* looking at object */
			  up[0], up[1], up[2]);

	positionLights();
	drawView(1);

	/* six views from object... */

	/* Projection has 90 degree field of view (6 make a cube). */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1.0, 0.5, 40.0);

	/* front view (center) */
	setViewArea(w3, h3, w3, h3);
	configFace(0);
	drawView(0);

	/* top view */
	setViewArea(w3, 2*h3, w3, h3);
	configFace(1);
	drawView(0);

	/* bottom view */
	setViewArea(w3, 0, w3, h3);
	configFace(2);
	drawView(0);

	/* left view */
	setViewArea(0, h3, w3, h3);
	configFace(3);
	drawView(0);

	/* right view */
	setViewArea(2*w3, h3, w3, h3);
	configFace(4);
	drawView(0);

	/* back view (top left) */
	setViewArea(4, 2*h3 + 4, w3 - 8, h3 - 8);
	configFace(5);
	drawView(0);

	glutSwapBuffers();
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
	}
	glutPostRedisplay();
}

void
motion(int x, int y)
{
	/* Pretty weak viewing model.  Eye gets located */
	/* on cylinder on the out skirts of the scene. */
	angle += (x - downx);
	angle = angle % 360;
	eye[0] = sin(angle * 3.14159/180.0) * -20;
	eye[2] = cos(angle * 3.14159/180.0) * -20;
	eye[1] = eye[1] + 0.1 * (y - downy);
	if (eye[1] > +15) eye[1] = +15;
	if (eye[1] < -15) eye[1] = -15;
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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("sixviews");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	initGraphicsState();

	glutCreateMenu(menu);
	glutAddMenuEntry("light on", 1);
	glutAddMenuEntry("light off", 2);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();
	return 0;
}
