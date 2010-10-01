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
/* Most math.h's do not define float versions of the trig functions. */
#define sinf sin
#define cosf cos
#define atan2f atan2
#endif

static int texture = 1;
static float rot = 0;
static float opacity = 1.0;
static float intensity = 1.0;
static float size = .001, delta = 0;
static float scale = 1.;
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2

void
pan(int x, int y) {
    transx +=  (x-ox)/500.;
    transy -= (y-oy)/500.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
rotate(int x, int y) {
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

void afunc(void) {
    static int state;
    if (state ^= 1) {
	glAlphaFunc(GL_GREATER, .01);
	glEnable(GL_ALPHA_TEST);
    } else {
	glDisable(GL_ALPHA_TEST);
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

void tfunc(void) {
    texture ^= 1;
}

void up(void) { scale += .1; }
void down(void) { scale -= .1; }
void left(void) { intensity -= .05f; if (intensity < 0.f) intensity = 0.0f; }
void right(void) { intensity += .05f; if (intensity > 1.f) intensity = 1.0f; }

void help(void) {
    printf("Usage: smoke [image]\n");
    printf("'h'            - help\n");
    printf("'a'            - toggle alpha test\n");
    printf("'b'            - toggle blend\n");
    printf("'t'            - toggle texturing\n");
    printf("'UP'           - scale up\n");
    printf("'DOWN'         - scale down\n");
    printf("'LEFT'	   - darken\n");
    printf("'RIGHT'	   - brighten\n");
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
#if 0
	if (components == 1) {
	    GLubyte *p = (GLubyte *)image;
	    int i;
	    for (i = 0; i < width*height; i++) {
		p[i*4+3] = p[i*4+0];
	    }
	    components = 2;
	}
#endif
	if (components != 2 && components != 4) {
	    printf("must be an RGBA or LA image\n");
	    exit(EXIT_FAILURE);
	}
    } else {
	int i, j;
	unsigned char *img;
	components = 4; width = height = 512;
	image = (unsigned *) malloc(width*height*sizeof(unsigned));
	img = (unsigned char *)image;
	for (j = 0; j < height; j++)
	    for (i = 0; i < width; i++) {
		int w2 = width/2, h2 = height/2;
		if (i & 32)
		    img[4*(i+j*width)+0] = 0xff;
		else
		    img[4*(i+j*width)+1] = 0xff;
		if (j&32)
		    img[4*(i+j*width)+2] = 0xff;
		if ((i-w2)*(i-w2) + (j-h2)*(j-h2) > 64*64 &&
		    (i-w2)*(i-w2) + (j-h2)*(j-h2) < 300*300) img[4*(i+j*width)+3] = 0xff;
	    }

    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.,1.,.1,20.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-5.5);
    glClearColor(.25f, .25f, .75f, .25f);

    glAlphaFunc(GL_GREATER, 0.016);
    glEnable(GL_ALPHA_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
}

void
animate(void) {
    if (delta > 8) {
	delta = 0.f;
	size = 0.f;
        opacity = 1.f;
	rot = 0.f;
    }
    size += .02f;
    delta += .03f;
    rot += .9f;
    opacity -= .005f;

    glutPostRedisplay();
}

void
cube(void) {
    glBegin(GL_QUADS);
	glNormal3f(0.f, 0.f, -1.f);
	glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0, -1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( 1.0, -1.0, -1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( 1.0,  1.0, -1.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-1.0,  1.0, -1.0);

	glNormal3f(0.f, 0.f, 1.f);
	glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0,  1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( 1.0, -1.0,  1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( 1.0,  1.0,  1.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-1.0,  1.0,  1.0);

	glNormal3f(0.f, 1.f, 0.f);
	glTexCoord2f(0.0, 0.0); glVertex3f(-1.0,  1.0, -1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( 1.0,  1.0, -1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( 1.0,  1.0,  1.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-1.0,  1.0,  1.0);

	glNormal3f(0.f, -1.f, 0.f);
	glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0, -1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( 1.0, -1.0, -1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( 1.0, -1.0,  1.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0,  1.0);

	glNormal3f( 1.f, 0.f, 0.f);
	glTexCoord2f(0.0, 0.0); glVertex3f( 1.0, -1.0, -1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( 1.0,  1.0, -1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( 1.0,  1.0,  1.0);
	glTexCoord2f(0.0, 1.0); glVertex3f( 1.0, -1.0,  1.0);

	glNormal3f(-1.f, 0.f, 0.f);
	glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0, -1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(-1.0,  1.0, -1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(-1.0,  1.0,  1.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0,  1.0);
    glEnd();
}
static void calcMatrix(void);

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
#define RAD(x) (((x)*M_PI)/180.)
    gluLookAt(-sinf(RAD(rotx))*5.5,transy,cosf(RAD(rotx))*5.5, 0.,0.,0., 0.,1.,0.);

    /* floor */
    glColor4f(0.f,.2f,0.f,1.f);
    glBegin(GL_POLYGON);
	glVertex3f(-4.0, -1.0, -4.0);
	glVertex3f( 4.0, -1.0, -4.0);
	glVertex3f( 4.0, -1.0,  4.0);
	glVertex3f(-4.0, -1.0,  4.0);
    glEnd();

    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glColor3f(.3f,.3f,.3f);
    glPushMatrix();
    glTranslatef(-1.f, -1.+.2f, -1.5f);
    glScalef(.2f,.2f, .2f);
    cube();
    glDisable(GL_LIGHTING);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(delta/2.f-1.f, delta-1.f, -1.5f);
    calcMatrix();
    glScalef(size,size,1.);
    if (texture) glEnable(GL_TEXTURE_2D);
    glColor4f(intensity, intensity, intensity, opacity);
    glRotatef(rot, 0., 0., 1.);
    glDepthMask(0);
    glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, -1.0);
	glTexCoord2f(1.0, 0.0); glVertex2f(1.0, -1.0);
	glTexCoord2f(1.0, 1.0); glVertex2f(1.0, 1.0);
	glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, 1.0);
    glEnd();
    glDepthMask(1);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'a': afunc(); break;
    case 'b': bfunc(); break;
    case 'h': help(); break;
    case 't': tfunc(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

/* ARGSUSED1 */
void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:	up(); break;
    case GLUT_KEY_DOWN:	down(); break;
    case GLUT_KEY_LEFT:	left(); break;
    case GLUT_KEY_RIGHT:right(); break;
    }
}

int main(int argc, char** argv) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow("smoke");
    init(argc == 1 ? "../data/smoke.bw" : argv[1]);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(animate);
    glutMainLoop();
    return 0;
}

void
printmat(float *m) {
    int i;
    for(i = 0; i < 4; i++) {
	printf("%f %f %f %f\n", m[4*i+0], m[4*i+1], m[4*i+2], m[4*i+3]);
    }
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
