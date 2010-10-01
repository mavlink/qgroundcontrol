
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* rtsmap.c - real-time generation and use of a sphere map with libglsmap */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>
#include <GL/glsmap.h>

#if defined(GL_EXT_texture_object) && !defined(GL_VERSION_1_1)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#endif

enum {
	TO_NONE = 0,
	TO_FRONT, TO_TOP, TO_BOTTOM, TO_LEFT, TO_RIGHT,
	TO_BACK,
	TO_SPHERE_MAP
};

static GLuint texobjs[6] = {
	TO_FRONT, TO_TOP, TO_BOTTOM,
	TO_LEFT, TO_RIGHT, TO_BACK
};

SphereMap *smap;

GLfloat up[3] = { 0, 1, 0 };
GLfloat eye[3] = { 0, 0, -20 };
GLfloat obj[3] = { 0, 0, 0 };

int W, H;

int showSphereMap = 0;
int object = 0;
int texdim = 64;
int doubleBuffer = 1;
int dynamicSmap = 1;
int cullBackFaces = 1;
int doSphereMap = 1;
int multipass = 0;

static char *pattern[] = {
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
  "xxxxxxxxxxxxxxxx",
  "xxxxxxxxxxxxxxxx",
  "xxxxxxxxxxxxxxxx",
  "xxxxxxxxxxxxxxxx",
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
  "......xxxx......",
};

static void
makePatternTexture(void)
{
  GLubyte patternTexture[16][16][4];
  GLubyte *loc;
  int s, t;

  /* Setup RGB image for the texture. */
  loc = (GLubyte*) patternTexture;
  for (t = 0; t < 16; t++) {
    for (s = 0; s < 16; s++) {
      if (pattern[t][s] == 'x') {
	/* Nice green. */
        loc[0] = 0x1f;
        loc[1] = 0x8f;
        loc[2] = 0x1f;
        loc[3] = 0x7f;  /* opaque */
      } else {
	/* Light gray. */
        loc[0] = 0x00;
        loc[1] = 0x00;
        loc[2] = 0x00;
        loc[3] = 0x00;  /* transparent */
      }
      loc += 4;
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);
  gluBuild2DMipmaps(GL_TEXTURE_2D, 4, 16, 16,
    GL_RGBA, GL_UNSIGNED_BYTE, patternTexture);
}

void
reshape(int w, int h)
{
	W = w;
	H = h;
}

void
positionLights(int view, void *context)
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
drawView(int view, void *context)
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
	/* bottom magenta sphere (0,-7,0) */
	glPushMatrix();
	  glTranslatef(0.0, -7.0, 0.0);
	  glColor3f(1.0, 0.0, 1.0);
	  glutSolidSphere(3.0, 8, 8);
	glPopMatrix();
}

void
drawObject(void *context)
{
        static GLfloat xplane[4] = { 1, 0, 0, 0 };
        static GLfloat zplane[4] = { 0, 1, 0, 0 };

        if (!cullBackFaces) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
        }

	glPushMatrix();
                glTranslatef(obj[0], obj[1], obj[2]);
                if (doSphereMap) {
		        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		        glEnable(GL_TEXTURE_GEN_S);
		        glEnable(GL_TEXTURE_GEN_T);
		        glEnable(GL_TEXTURE_2D);
		        glBindTexture(GL_TEXTURE_2D, 7);
		        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
			GL_REPLACE);
                } else {
                        glTexGenfv(GL_S, GL_OBJECT_PLANE, xplane);
                        glTexGenfv(GL_T, GL_OBJECT_PLANE, zplane);
                        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                        glEnable(GL_TEXTURE_GEN_S);
		        glEnable(GL_TEXTURE_GEN_T);
		        glEnable(GL_TEXTURE_2D);
                        glBindTexture(GL_TEXTURE_2D, 100);
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
			GL_REPLACE);
                }

		glColor3f(1.0, 1.0, 1.0);
		switch (object) {
		case 0:
			glutSolidSphere(3.0, 20, 20);
			break;
		case 1:
			glScalef(3.0, 3.0, 3.0);
			glRotatef(30.0, 1.0, 1.0, 0.0);
			glutSolidIcosahedron();
			break;
		case 2:
			glFrontFace(GL_CW);
			glutSolidTeapot(3.0);
			glFrontFace(GL_CCW);
			break;
		}

	        glDisable(GL_TEXTURE_GEN_S);
       		glDisable(GL_TEXTURE_GEN_T);
	glPopMatrix();

        if (!cullBackFaces) {
                glCullFace(GL_BACK);
        }
}

void
display(void)
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
       	glEnable(GL_CULL_FACE);

        if (dynamicSmap) {
	  smapGenSphereMap(smap);
        }

	/* smapGenSphereMap leaves scissor enabled, disable it. */
	glDisable(GL_SCISSOR_TEST);

	glViewport(0, 0, W, H);

	if (showSphereMap) {
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, 1, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 7);
		glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2f(0,0);
			glTexCoord2f(1,0);
			glVertex2f(1,0);
			glTexCoord2f(1,1);
			glVertex2f(1,1);
			glTexCoord2f(0,1);
			glVertex2f(0,1);
		glEnd();
	} else {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60.0, 1.0, 0.5, 40.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(eye[0], eye[1], eye[2],  /* eye at eye */
				  obj[0], obj[1], obj[2],  /* looking at object */
				  up[0], up[1], up[2]);

		positionLights(-1, NULL);
		drawView(-1, NULL);
                if (multipass) {
                  doSphereMap = 1;
                }
		drawObject(NULL);
                if (multipass) {
                  doSphereMap = 0;
                  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                  glDepthFunc(GL_EQUAL);
                  glEnable(GL_BLEND);
		  drawObject(NULL);
                  glDisable(GL_BLEND);
                  glDepthFunc(GL_LESS);
                }
	}

	if (doubleBuffer) {
		glutSwapBuffers();
	}
}

int angle = 0;
int downx, downy;

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
	smapSetEyeVector(smap, eye);
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
menu(int value)
{
	switch (value) {
	case 0:
		showSphereMap = 0;
		break;
	case 1:
		showSphereMap = 1;
		break;
	case 4:
		object = (object + 1) % 3;
		break;
	case 5:
		smapSetSphereMapTexDim(smap, 64);
		smapSetViewTexDim(smap, 64);
		break;
	case 6:
		smapSetSphereMapTexDim(smap, 128);
		smapSetViewTexDim(smap, 128);
		break;
	case 7:
		smapSetSphereMapTexDim(smap, 256);
		smapSetViewTexDim(smap, 256);
		break;
	case 8:
		glDrawBuffer(GL_FRONT);
		glReadBuffer(GL_FRONT);
		doubleBuffer = 0;
		break;
	case 9:
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);
		doubleBuffer = 1;
		break;
        case 10:
                dynamicSmap = 1;
                break;
        case 11:
                dynamicSmap = 0;
                break;
        case 12:
                cullBackFaces = 1;
                break;
        case 13:
                cullBackFaces = 0;
                break;
        case 14:
                doSphereMap = 1;
                multipass = 0;
                break;
        case 15:
                doSphereMap = 0;
                multipass = 0;
                break;
        case 16:
                multipass = 1;
                break;
	case 666:
		exit(0);
		break;
	}
	glutPostRedisplay();
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
	glutInitWindowSize(400, 400);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("real-time dynamic sphere mapping");
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);

	initGraphicsState();

	glutCreateMenu(menu);
	glutAddMenuEntry("eye view", 0);
	glutAddMenuEntry("show sphere map", 1);
	glutAddMenuEntry("outline", 2);
	glutAddMenuEntry("switch object", 4);
	glutAddMenuEntry("texdim = 64", 5);
	glutAddMenuEntry("texdim = 128", 6);
	glutAddMenuEntry("texdim = 256", 7);
	glutAddMenuEntry("single buffer", 8);
	glutAddMenuEntry("double buffer", 9);
        glutAddMenuEntry("dynamic smap", 10);
        glutAddMenuEntry("stop smap building", 11);
        glutAddMenuEntry("cull back faces", 12);
        glutAddMenuEntry("cull front faces", 13);
        glutAddMenuEntry("sphere map", 14);
        glutAddMenuEntry("texture", 15);
        glutAddMenuEntry("multipass", 16);
	glutAddMenuEntry("quit", 666);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	smap = smapCreateSphereMap(NULL);
	smapSetSphereMapTexObj    (smap, TO_SPHERE_MAP);
	smapSetViewTexObjs        (smap, texobjs);
	smapSetEyeVector          (smap, eye);
	smapSetUpVector           (smap, up);
	smapSetObjectVector       (smap, obj);
	smapSetNearFar            (smap, 0.5, 40.0);
	smapSetPositionLightsFunc (smap, positionLights);
	smapSetDrawViewFunc       (smap, drawView);
	smapSetViewTexDim         (smap, 64);
	smapSetSphereMapTexDim    (smap, 64);

        glBindTexture(GL_TEXTURE_2D, 100);
        makePatternTexture();

	glutMainLoop();
	return 0;
}
