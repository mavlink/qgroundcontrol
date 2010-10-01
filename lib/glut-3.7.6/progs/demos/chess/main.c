/*
 * main.c - part of the chess demo in the glut distribution.
 *
 * (C) Henk Kok (kok@wins.uva.nl)
 *
 * This file can be freely copied, changed, redistributed, etc. as long as
 * this copyright notice stays intact.
 */

#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#include "chess.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define M_TEXTURING	1
#define M_REFLECTION	2
#define M_CHAOS		3
#define M_ANIMATION	4

extern int reflection, texturing, animating, chaos;

GLfloat lightpos[4] = { 2.0, 1.0, 1.0, 0.0 };
GLfloat lightamb[4] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat lightdif[4] = { 1.0, 1.0, 1.0, 1.0 };

float angle = 0.0, a2 =  45.0;
int speed = 0;
GLfloat px = -3.5, py = -16.5, pz = 9.5;

void SetCamera(void)
{
    gluLookAt(0.0,2.0,2.0, 0.0,2.0,0.0, 0.0,1.0,0.0);
    glRotatef(a2, 1.0, 0.0, 0.0);
    glRotatef(angle, 0.0, 1.0, 0.0);
    glTranslatef(px, -pz, py);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
}

void display(void)
{
    glLoadIdentity();
    SetCamera();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    do_display();
    glutSwapBuffers();
}

void myinit(void)
{
    glShadeModel (GL_SMOOTH);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    glLoadIdentity();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdif);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    init_lists();
}

void Animate(void)
{
    px -= speed * 0.02 * sin(angle*M_PI/180);
    py += speed * 0.02 * cos(angle*M_PI/180);
    if (animating)
	proceed();
    glutPostRedisplay();
}

extern int chaosPieces;

/* ARGSUSED1 */
void parsekey(unsigned char key, int x, int y)
{
    switch (key)
    {
	case 27: exit(0);
	case 13: speed = 0; break;
	case 'a': a2 += 5; break;
	case 'z': a2 -= 5; break;
	case 'A': pz += 0.5; break;
	case 'Z': pz -= 0.5; break;
        default:
	    return;
    }
    glutPostRedisplay();
    if (animating || (chaosPieces > 0) || (speed != 0)) 
          glutIdleFunc(Animate);
    else
          glutIdleFunc(NULL);
}

/* ARGSUSED1 */
void parsekey_special(int key, int x, int y)
{
    switch (key)
    {
	case GLUT_KEY_UP:	speed += 1; break;
	case GLUT_KEY_DOWN:	speed -= 1; break;
	case GLUT_KEY_RIGHT:	angle += 5; break;
	case GLUT_KEY_LEFT:	angle -= 5; break;
	case GLUT_KEY_HOME:
            angle = 0.0, a2 =  45.0;
            speed = 0;
            px = -3.5, py = -16.5, pz = 9.5;
            break;
        default:
	    return;
    }
    glutPostRedisplay();
    if (animating || (chaosPieces > 0) || (speed != 0)) 
          glutIdleFunc(Animate);
    else
          glutIdleFunc(NULL);
}

void handle_main_menu(int item)
{
    switch(item) {
    case M_REFLECTION:
	reflection = !reflection;
        glutPostRedisplay();
	break;
    case M_TEXTURING:
	texturing = !texturing;
        glutPostRedisplay();
	break;
    case M_ANIMATION:
	animating = !animating;
	if (animating || (chaosPieces > 0) || (speed != 0)) 
          glutIdleFunc(Animate);
	else
          glutIdleFunc(NULL);
	break;
    case M_CHAOS:
	chaos = !chaos;
	if (animating || chaos || (speed != 0)) 
          glutIdleFunc(Animate);
	break;
    }
}

void
Visible(int visible)
{
  if (visible == GLUT_VISIBLE) {
    if (animating || (chaosPieces > 0) || (speed != 0))
      glutIdleFunc(Animate);
  } else {
    glutIdleFunc(NULL);
  }
}

void myReshape(int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum (-0.1, 0.1, -0.1, 0.1, 0.3, 200.0);
    glMatrixMode (GL_MODELVIEW);
    glViewport(0, 0, w, h);
    glLoadIdentity();
    SetCamera();
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE | GLUT_MULTISAMPLE | GLUT_STENCIL);
    glutCreateWindow("Chess");
    glutDisplayFunc(display);
    glutInitWindowPosition(200, 0);
    glutInitWindowSize(300, 300);
    glutKeyboardFunc(parsekey);
    glutSpecialFunc(parsekey_special);
    glutReshapeFunc(myReshape);
    glutVisibilityFunc(Visible);
    myinit();

    glutCreateMenu(handle_main_menu);
    glutAddMenuEntry("Toggle texturing", M_TEXTURING);
    glutAddMenuEntry("Toggle reflection", M_REFLECTION);
    glutAddMenuEntry("-----------------", -1);
    glutAddMenuEntry("Toggle animation", M_ANIMATION);
    glutAddMenuEntry("Toggle CHAOS!", M_CHAOS);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
 
    glutSwapBuffers();
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
