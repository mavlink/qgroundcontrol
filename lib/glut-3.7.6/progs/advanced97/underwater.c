#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
/* Most math.h's do not define float versions of the math functions. */
#define expf(x) ((float)exp((x)))
#define sinf(x) ((float)sin((x)))
#endif

static float transx = 1.0, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2

void
pan(const int x, const int y) {
    transx +=  (x-ox)/5.;
    transy -= (y-oy)/5.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
rotate(const int x, const int y) {
    rotx += x-ox;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += y-oy;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y) {
    if (mot == PAN) pan(x, y);
    else if (mot == ROT) rotate(x,y);
}

void
mouse(int button, int state, int x, int y) {
    if(state == GLUT_DOWN) {
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    mot = PAN;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

static GLfloat s_plane[4] = { 1.0, 0., 0., 0.};
static GLfloat t_plane[4] = { 0., 0., 1.0, 0.};
static GLfloat fog_params[5] = {.015, .1, .2, .2, .1};

void init(void) {
    int width, height, components;
    GLubyte *image;

    glClearColor (0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    if (!(image = (GLubyte *)read_texture("../data/sea.rgb", &width, &height, &components))) {
	perror("sea.rgb");
	exit(EXIT_FAILURE);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glMaterialf (GL_FRONT, GL_SHININESS, 64.0);

    glClearColor(.09f,.18f,.18f,1.f);

    glFogi(GL_FOG_MODE, GL_EXP);
    glFogf(GL_FOG_DENSITY, fog_params[0]);
    glFogfv(GL_FOG_COLOR, fog_params+1);
    glEnable(GL_FOG);
    {
    GLfloat pos[] = {0.,150.,1.,1.};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    }
}

void display(void) {
    GLfloat s, t;
    static float phase;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    s = sinf(phase); t = s; phase += M_PI/25.f;
    if (phase > 2*M_PI) phase -= 2*M_PI;
    glTranslatef(.5f, -0.5f, 0.f);
    glScalef(.1f, .1f, 1.f);
    glTranslatef(s, t, 0.f);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    glColor4f(.09, .18, .18, 1.f);
    glDisable(GL_TEXTURE_2D);
    glTranslatef(0., 0., -160.);
    glScalef(200., 200., 1.);
    glBegin(GL_POLYGON);
	glVertex3f(-1.,-1.,0.);
	glVertex3f( 1.,-1.,0.);
	glVertex3f( 1., 1.,0.);
	glVertex3f(-1., 1.,0.);
    glEnd();
    glPopMatrix();
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glTranslatef(0., 0., -100.+transx);
    glRotatef(rotx, 0.0, 1.0, 0.0);
    glScalef(10.f, 10.f, 10.f);
    glutSolidTeapot(2.0);
    glPopMatrix ();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40., 1.0, 10.0, 200000.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void ffunc(void) {
    static int state = 1;
    if (state ^= 1)
	glEnable(GL_FOG);
    else
	glDisable(GL_FOG);
}

void help(void) {
    printf("Usage: smoke [image]\n");
    printf("'h'           - help\n");
    printf("'f'           - toggle fog\n");
    printf("left mouse    - pan\n");
    printf("right mouse   - rotate\n");
}

/*ARGSUSED1*/
void key (unsigned char key, int x, int y) {
   switch (key) {
      case 'f': ffunc(); break;
      case 'h': help(); break;
      case '\033': exit(EXIT_SUCCESS); break;
      default: break;
   }
}

void animate(void) {
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInitWindowSize(256, 256);
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutCreateWindow (argv[0]);
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    glutIdleFunc(animate);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}
