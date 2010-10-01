/*
 * Main.c
 *
 * This file is part of the openGL-logo demo.
 * (c) Henk Kok (kok@wins.uva.nl)
 *
 * Copying, redistributing, etc is permitted as long as this copyright
 * notice and the Dutch variable names :) stay in tact.
 */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

GLfloat lightpos[4] = { 1.0, 1.0, 1.0, 0.0 };
GLfloat lightamb[4] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat lightdif[4] = { 0.8, 0.8, 0.8, 1.0 };
float speed=0, progress = 1;
void SetCamera(void);

extern void randomize(void);
extern void def_logo(void);
extern void draw_logo(void);

void do_display (void)
{
	SetCamera();
	draw_logo();
	glFlush();
	glutSwapBuffers();
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	do_display();
}

void myinit (void)
{
	glShadeModel (GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdif);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glColor3f(1.0, 1.0, 1.0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_NORMALIZE);
	def_logo();
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
}

/* ARGSUSED1 */
void parsekey(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: exit(0); break;
	case 13: break;
	case ' ': progress = 1; randomize(); break;
	}
}

/* ARGSUSED1 */
void parsekey_special(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:		break;
	case GLUT_KEY_DOWN:		break;
	case GLUT_KEY_RIGHT:	break;
	case GLUT_KEY_LEFT:		break;
	}
}

void Animate(void)
{
	speed = -0.95*speed + progress*0.05;
	if (progress > 0.0 && speed < 0.0003)
		speed = 0.0003;
	if (speed > 0.01)
		speed = 0.01;
	progress = progress - speed;
	if (progress < 0.0)
	{
		progress = 0.0;
		speed = 0;
	}
	glutPostRedisplay();
}

void myReshape(int w, int h)
{
	glMatrixMode (GL_MODELVIEW);
	glViewport (0, 0, w, h);
	glLoadIdentity();
	SetCamera();
}

void SetCamera(void)
{
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glFrustum (-0.1333, 0.1333, -0.1, 0.1, 0.2, 150.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0,1.5,2, 0,1.5,0, 0,1,0);
	glTranslatef(0.0, -8.0, -45.0);
	glRotatef(-progress*720, 0.0, 1.0, 0.0);
}

int main(int argc, char *argv[])
{
	glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE | GLUT_MULTISAMPLE);
	glutInitWindowPosition(200, 0);
	glutInitWindowSize(640, 480);
	glutCreateWindow("Rotating OpenGL Logo");
	glutDisplayFunc(display);
	glutKeyboardFunc(parsekey);
	glutSpecialFunc(parsekey_special);
	glutReshapeFunc(myReshape);
	glutIdleFunc(Animate);
	randomize();
	myinit();
	glutSwapBuffers();
	glutMainLoop();
	return 0;             /* ANSI C requires main to return int. */
}
