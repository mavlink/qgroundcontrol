
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* fakeraytrace.c - two reflective objects, one has reflection of other's reflection */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>
#include <GL/glsmap.h>

#if defined(GL_EXT_texture_object) && !defined(GL_VERSION_1_1)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#endif

typedef struct {
        GLfloat *obj2draw;
        int drawObj1;
        GLuint texobj;
} Object;

enum { X, Y, Z };

enum {
	TO_NONE = 0,
	TO_FRONT, TO_TOP, TO_BOTTOM, TO_LEFT, TO_RIGHT,
	TO_BACK,
	TO_SPHERE_MAP,
        TO_FRONT2, TO_TOP2, TO_BOTTOM2, TO_LEFT2, TO_RIGHT2,
	TO_BACK2,
	TO_SPHERE_MAP2,

        TO_FRONT3, TO_TOP3, TO_BOTTOM3, TO_LEFT3, TO_RIGHT3,
	TO_BACK3,
	TO_SPHERE_MAP3
};

static GLuint texobjs[6] = {
	TO_FRONT, TO_TOP, TO_BOTTOM,
	TO_LEFT, TO_RIGHT, TO_BACK
};

static GLuint texobjs2[6] = {
	TO_FRONT2, TO_TOP2, TO_BOTTOM2,
	TO_LEFT2, TO_RIGHT2, TO_BACK2
};

static GLuint texobjs3[6] = {
	TO_FRONT3, TO_TOP3, TO_BOTTOM3,
	TO_LEFT3, TO_RIGHT3, TO_BACK3
};

int moving = 0;
int multibounce = 1;
SphereMap *smap, *smap2, *smap3;
int win;

GLfloat up[3] = { 0, 1, 0 };
GLfloat up2[3] = { 0, 0, 1 };
GLfloat eye[3] = { 0, 0, -15 };
GLfloat obj[3] = { 0, 4, 0 };
GLfloat obj2[3] = { 0, -4, 0 };

Object sphere[] = {
        { obj, 0, TO_SPHERE_MAP },
        { obj2, 1, TO_SPHERE_MAP2 },
        { obj, 0, TO_SPHERE_MAP3 } };

GLfloat timeCount;
int W, H;

int showSphereMap = 0;
int object = 0;
int texdim = 64;
int doubleBuffer = 1;
int dynamicSmap = 1;
int cullBackFaces = 1;
int doSphereMap = 1;
int multipass = 0;
int animation = 0;
int sphereTess = 20;

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

extern void drawObject(Object *it);

void
drawView(int view, void *context)
{
        Object *it = context;

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

        if (it && multibounce) {
                if (it->drawObj1) {
                        glEnable(GL_TEXTURE_2D);
                        drawObject(&sphere[2]);
                        glDisable(GL_TEXTURE_2D);
                }
        }
}

void
drawObject(Object *it)
{
        static GLfloat xplane[4] = { 1, 0, 0, 0 };
        static GLfloat zplane[4] = { 0, 1, 0, 0 };
        GLfloat *obj2draw = it->obj2draw;

        if (!cullBackFaces) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
        }

	glPushMatrix();

                glTranslatef(obj2draw[X], obj2draw[Y], obj2draw[Z]);

                if (doSphereMap) {
		        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		        glEnable(GL_TEXTURE_GEN_S);
		        glEnable(GL_TEXTURE_GEN_T);
		        glEnable(GL_TEXTURE_2D);
		        glBindTexture(GL_TEXTURE_2D, it->texobj);
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
                        glutSolidSphere(3.5, 20, 20);
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
vsub(const GLfloat *src1, const GLfloat *src2, GLfloat *dst)
{
    dst[0] = src1[0] - src2[0];
    dst[1] = src1[1] - src2[1];
    dst[2] = src1[2] - src2[2];
}

void
vcopy(const GLfloat *v1, GLfloat *v2)
{
    register int i;
    for (i = 0 ; i < 3 ; i++)
        v2[i] = v1[i];
}

void
vcross(const GLfloat *v1, const GLfloat *v2, GLfloat *cross)
{
    GLfloat temp[3];

    temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
    temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
    temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
    vcopy(temp, cross);
}

void
display(void)
{
        GLfloat tmp1[3], tmp2[3];

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
       	glEnable(GL_CULL_FACE);

        if (dynamicSmap) {
                if (multibounce) {
                glDisable(GL_TEXTURE_2D);
	        glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);

                vsub(eye,obj,tmp1);
                vsub(obj2,obj,tmp2);
                vcross(tmp1,tmp2,up2);
                vcross(up2,tmp2,up2);

                smapSetUpVector(smap3, up2);
	        smapGenSphereMap(smap3);
                }
        
                glDisable(GL_TEXTURE_2D);
	        glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);
	        smapGenSphereMap(smap);

                glDisable(GL_TEXTURE_2D);
	        glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);
	        smapGenSphereMap(smap2);
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
		glBindTexture(GL_TEXTURE_2D, showSphereMap);
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
		gluPerspective(60.0, (GLfloat) W / (GLfloat) H, 0.5, 40.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(eye[0], eye[1], eye[2],  /* eye at eye */
				  0, 0, 0,  /* looking at object */
				  up[0], up[1], up[2]);

		positionLights(-1, NULL);
		drawView(-1, NULL);
                if (multipass) {
                  doSphereMap = 1;
                }
		drawObject(&sphere[0]);
                if (multipass) {
                  doSphereMap = 0;
                  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                  glDepthFunc(GL_EQUAL);
                  glEnable(GL_BLEND);
		  drawObject(&sphere[0]);
                  glDisable(GL_BLEND);
                  glDepthFunc(GL_LESS);
                }
                if (multipass) {
                  doSphereMap = 1;
                }
		drawObject(&sphere[1]);
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
        if (moving) {
	angle += (x - downx);
	angle = angle % 360;
	eye[0] = sin(angle * 3.14159/180.0) * -15;
	eye[2] = cos(angle * 3.14159/180.0) * -15;
	eye[1] = eye[1] + 0.1 * (y - downy);
	if (eye[1] > +25) eye[1] = +25;
	if (eye[1] < -25) eye[1] = -25;
	smapSetEyeVector(smap, eye);
	smapSetEyeVector(smap2, eye);
	glutPostRedisplay();
	downx = x;
	downy = y;
        }
}

void
mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
                        moving = 1;
			downx = x;
			downy = y;
                } else {
                        moving = 0;
                }
	}
}

void
idle(void)
{
   timeCount = timeCount + 0.1;
   obj2[Y] = -5 + cos(timeCount) * 1.0;
   smapSetObjectVector(smap, obj);
   smapSetObjectVector(smap2, obj2);
   smapSetObjectVector(smap3, obj);
   glutPostRedisplay();
}

/* When not visible, stop animating.  Restart when visible again. */
static void 
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (animation)
      glutIdleFunc(idle);
  } else {
    if (!animation)
      glutIdleFunc(NULL);
  }
}

void initGraphicsState(void);
void initCallbacks(int ifFullscreen);

void
menu(int value)
{
	switch (value) {
	case 0:
		showSphereMap = 0;
		break;
	case 1:
		showSphereMap = TO_SPHERE_MAP;
		break;
	case -1:
		showSphereMap = TO_SPHERE_MAP2;
		break;
	case -2:
		showSphereMap = TO_SPHERE_MAP3;
		break;
	case 4:
		object = (object + 1) % 3;
		break;
	case 5:
		smapSetSphereMapTexDim(smap, 64);
		smapSetViewTexDim(smap, 64);
		smapSetSphereMapTexDim(smap2, 64);
		smapSetViewTexDim(smap2, 64);
		smapSetSphereMapTexDim(smap3, 64);
		smapSetViewTexDim(smap3, 64);
		break;
	case 6:
		smapSetSphereMapTexDim(smap, 128);
		smapSetViewTexDim(smap, 128);
		smapSetSphereMapTexDim(smap2, 128);
		smapSetViewTexDim(smap2, 128);
		smapSetSphereMapTexDim(smap3, 128);
		smapSetViewTexDim(smap3, 128);
	        break;  
	case 7:
		smapSetSphereMapTexDim(smap, 256);
		smapSetViewTexDim(smap, 256);
		smapSetSphereMapTexDim(smap2, 256);
		smapSetViewTexDim(smap2, 256);
		smapSetSphereMapTexDim(smap3, 256);
		smapSetViewTexDim(smap3, 256);
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
        case 17:
                animation = !animation;
                if (animation) {
                        glutIdleFunc(idle);
                } else {
                        glutIdleFunc(NULL);
                }
                break;
	case 666:
		exit(0);
		break;
	}
	glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
        switch (c) {
        case 27:
                exit(0);
                break;
        case '1':
                menu(5);
                break;
        case '2':
                menu(6);
                break;
        case '3':
                menu(7);
                break;
        case 's':
                menu(4);
                break;
        case 'f':
                glutGameModeString("400x300:16@60");
                glutEnterGameMode();
                initGraphicsState();
                initCallbacks(0);
                break;
        case 'l':
                glutLeaveGameMode();
                glutSetWindow(win);
                break;
        case 'a':
                sphereTess += 5;
                glutPostRedisplay();
                break;
        case 'z':
                sphereTess -= 5;
                glutPostRedisplay();
                break;
        case 'b':
        case 'B':
                multibounce = !multibounce;
                printf("multibounce = %d\n", multibounce);
                break;
        case ' ':
                menu(17);
                break;
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

        glBindTexture(GL_TEXTURE_2D, 100);
        makePatternTexture();
}

void
initCallbacks(int ifFullscreen)
{
        glutReshapeFunc(reshape);
	glutDisplayFunc(display);
        glutVisibilityFunc(visible);
	glutMouseFunc(mouse);
        glutKeyboardFunc(keyboard);
	glutMotionFunc(motion);

        if (ifFullscreen) {

        glutCreateMenu(menu);
	glutAddMenuEntry("eye view", 0);
	glutAddMenuEntry("eye to obj 1 smap", 1);
	glutAddMenuEntry("eye to obj 2 smap", -1);
	glutAddMenuEntry("obj 1 to obj 2 smap", -2);
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
        glutAddMenuEntry("toggle animate", 17);
	glutAddMenuEntry("quit", 666);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

        }
}

int
main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	win = glutCreateWindow("fakeraytrace");

	initGraphicsState();
        initCallbacks(1);

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
        smapSetContextData(smap, &sphere[0]);

        smap2 = smapCreateSphereMap(NULL);
	smapSetSphereMapTexObj    (smap2, TO_SPHERE_MAP2);
	smapSetViewTexObjs        (smap2, texobjs2);
	smapSetEyeVector          (smap2, eye);
	smapSetUpVector           (smap2, up);
	smapSetObjectVector       (smap2, obj2);
	smapSetNearFar            (smap2, 0.5, 40.0);
	smapSetPositionLightsFunc (smap2, positionLights);
	smapSetDrawViewFunc       (smap2, drawView);
	smapSetViewTexDim         (smap2, 64);
	smapSetSphereMapTexDim    (smap2, 64);
        smapSetContextData(smap2, &sphere[1]);

        smap3 = smapCreateSphereMap(NULL);
	smapSetSphereMapTexObj    (smap3, TO_SPHERE_MAP3);
	smapSetViewTexObjs        (smap3, texobjs3);
	smapSetEyeVector          (smap3, obj2);
	smapSetUpVector           (smap3, up2);
	smapSetObjectVector       (smap3, obj);
	smapSetNearFar            (smap3, 0.1, 40.0);
	smapSetPositionLightsFunc (smap3, positionLights);
	smapSetDrawViewFunc       (smap3, drawView);
	smapSetViewTexDim         (smap3, 64);
	smapSetSphereMapTexDim    (smap3, 64);
        smapSetContextData(smap3, &sphere[2]);

	glutMainLoop();
	return 0;
}
