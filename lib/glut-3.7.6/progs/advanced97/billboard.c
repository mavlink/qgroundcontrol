#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "texture.h"
#include <GL/glut.h>

#ifndef __sgi
/* Most math.h's do not define float versions of the trig functions. */
#define sinf sin
#define cosf cos
#define atan2f atan2
#endif

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int billboard = 1, texture = 1;
static float scale = 1.;
static float transx = 1.0, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2

void
pan(const int x, const int y) {
    transx +=  (x-ox)/500.;
    transy -= (y-oy)/500.;
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
	case GLUT_MIDDLE_BUTTON:
	    break;
	case GLUT_RIGHT_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

void afunc(void) {
    static int state;
    if (state ^= 1) {
	glAlphaFunc(GL_GREATER, .01);
	glEnable(GL_ALPHA_TEST);
    } else {
	glDisable(GL_ALPHA_TEST);
    }
}

void demofunc(void) {
    static float deltax = -.03;
    static float deltay = 2.;

    transx += deltax;
    if (transx > 2.0 || transx < -2.0) deltax = -deltax;

    rotx += deltay;
    if (rotx > 360.f || rotx < 0.f) deltay = -deltay;
    glutPostRedisplay();
}

void dfunc(void) {
    static int demo;
    if (demo ^= 1) {
	glutIdleFunc(demofunc);
    } else {
	glutIdleFunc(0);
    }
}

void bfunc(void) {
    static int state;
    if (state ^= 1) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
    } else {
	glDisable(GL_BLEND);
    }
}

void ffunc(void) {
    billboard ^= 1;
}

void tfunc(void) {
    texture ^= 1;
}

void up(void) { scale += .1; }
void down(void) { scale -= .1; }

void help(void) {
    printf("Usage: billboard [image]\n");
    printf("'h'            - help\n");
    printf("'a'            - toggle alpha test\n");
    printf("'b'            - toggle blend\n");
    printf("'d'            - toggle demo mode\n");
    printf("'f'            - toggle fixed mode\n");
    printf("'t'            - toggle texturing\n");
    printf("'UP'           - scale up\n");
    printf("'DOWN'         - scale down\n");
    printf("left mouse     - pan\n");
    printf("right mouse    - rotate\n");
}

void init(char *filename) {
    static unsigned *image;
    static int width, height, components;
    if (filename) {
	image = read_texture(filename, &width, &height, &components);
	if (image == NULL) {
	    fprintf(stderr, "Error: Can't load image file \"%s\".\n",
		    filename);
	    exit(EXIT_FAILURE);
	} else {
	    printf("%d x %d image loaded\n", width, height);
	}
	if (components != 4) {
	    printf("must be an RGBA image\n");
	    exit(EXIT_FAILURE);
	}
    } else {
	int i, j;
	unsigned char *img;
	components = 4; width = height = 256;
	image = (unsigned *) malloc(width*height*sizeof(unsigned));
	img = (unsigned char *)image;
	for (j = 0; j < height; j++)
	    for (i = 0; i < width; i++) {
		int w2 = width/2, h2 = height/2;
		if ((i & 32) ^ (j & 32)) {
		    img[4*(i+j*width)+0] = 0xff;
		    img[4*(i+j*width)+1] = 0xff;
		    img[4*(i+j*width)+2] = 0xff;
		} else {
		    img[4*(i+j*width)+0] = 0x0;
		    img[4*(i+j*width)+1] = 0x0;
		    img[4*(i+j*width)+2] = 0x0;
		}
		if ((i-w2)*(i-w2) + (j-h2)*(j-h2) > 64*64 &&
		    (i-w2)*(i-w2) + (j-h2)*(j-h2) < 300*300) img[4*(i+j*width)+3] = 0xff;
	    }

    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, image);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.,1.,.1,10.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-5.5);
    glClearColor(.25f, .25f, .25f, .25f);
    glEnable(GL_DEPTH_TEST);
}

static void calcMatrix(void);

void display(void) {
#if NATE
    float mat[16];
#endif
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
#define RAD(x) (((x)*M_PI)/180.)
    gluLookAt(-sinf(RAD(rotx))*5.5,transy,cosf(RAD(rotx))*5.5, 0.,0.,0., 0.,1.,0.);

#if NATE
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);
#endif
    /* floor */
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.2, 0.8, 0.2, 1.);
    glBegin(GL_POLYGON);
	glVertex3f(-2.0, -1.0, -2.0);
	glVertex3f( 2.0, -1.0, -2.0);
	glVertex3f( 2.0, -1.0,  2.0);
	glVertex3f(-2.0, -1.0,  2.0);
    glEnd();

    glPushMatrix();
#if 0
    glTranslatef(transx, transy, 0.f);
    glRotatef(rotx, 0., 1., 0.);
    glRotatef(roty, 1., 0., 0.);
#endif
    glTranslatef(0.f, 0.f, -transx);
    if (billboard) calcMatrix();
    glScalef(scale,scale,1.);
    if (texture) glEnable(GL_TEXTURE_2D);
    glColor4f(1.f, 1.f, 1.f, 1.f);
    glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex2f(-1.0, -1.0);
	glTexCoord2f(1.0, 0.0);
	glVertex2f(1.0, -1.0);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(1.0, 1.0);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(-1.0, 1.0);
    glEnd();
    glPopMatrix();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/*ARGSUSED*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'a': afunc(); break;
    case 'd': dfunc(); break;
    case 'b': bfunc(); break;
    case 'f': ffunc(); break;
    case 't': tfunc(); break;
    case 'h': help(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

/*ARGSUSED*/
void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:	up(); break;
    case GLUT_KEY_DOWN:	down(); break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow("billboard");
    init(argv[1]);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}

void
buildRot(float theta, float x, float y, float z, float m[16]) {
    float d = x*x + y*y + z*z;
    float ct = cosf(RAD(theta)), st = sinf(RAD(theta));

    /* normalize */
    if (d > 0) {
	d = 1/d;
	x *= d;
	y *= d;
	z *= d;
    }

    m[ 0] = 1; m[ 1] = 0; m[ 2] = 0; m[ 3] = 0;
    m[ 4] = 0; m[ 5] = 1; m[ 6] = 0; m[ 7] = 0;
    m[ 8] = 0; m[ 9] = 0; m[10] = 1; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;

    /* R = uu' + cos(theta)*(I-uu') + sin(theta)*S
     *
     * S =  0  -z   y    u' = (x, y, z)
     *	    z   0  -x
     *	   -y   x   0
     */

     m[0] = x*x + ct*(1-x*x) + st*0;
     m[4] = x*y + ct*(0-x*y) + st*-z;
     m[8] = x*z + ct*(0-x*z) + st*y;

     m[1] = y*x + ct*(0-y*x) + st*z;
     m[5] = y*y + ct*(1-y*y) + st*0;
     m[9] = y*z + ct*(0-y*z) + st*-x;

     m[2] = z*x + ct*(0-z*x) + st*-y;
     m[6] = z*y + ct*(0-z*y) + st*x;
     m[10]= z*z + ct*(1-z*z) + st*0;
}

static void
calcMatrix(void) {
    float mat[16];

    glGetFloatv(GL_MODELVIEW_MATRIX, mat);
    buildRot(-180*atan2f(mat[8], mat[10])/M_PI, 0, 1, 0, mat);
    glMultMatrixf(mat);
}
