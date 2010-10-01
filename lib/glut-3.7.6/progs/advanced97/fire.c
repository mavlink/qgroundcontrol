#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"
#include "sm.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
/* Most math.h's do not define float versions of the trig functions. */
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define atan2f(y, x) ((float)atan2((y), (x)))
#endif

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture	glBindTextureEXT
#endif

static void *smoke;
static int marshmallow;
static int the_texture;
static int texture_count;
static int texture = 1;
static float rot = 0;
static float opacity = 1.0;
static float intensity = 1.0;
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
    if (state ^= 1)
	glEnable(GL_ALPHA_TEST);
    else
	glDisable(GL_ALPHA_TEST);
}

void bfunc(void) {
    static int state;
    if (state ^= 1)
	glEnable(GL_BLEND);
    else
	glDisable(GL_BLEND);
}

void mfunc(void) {
    marshmallow += 1;
    if (marshmallow > 2) marshmallow = 0;
}

void sfunc(void) {
    the_texture++;
    if (the_texture >= texture_count) the_texture = 0;
}

void tfunc(void) {
    static int state;
    if (state ^= 1)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    else
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void fourfunc(void) {
    static int state;
    GLenum wrap;
    int i;

    glMatrixMode(GL_TEXTURE);
    if (state ^= 1) {
	wrap = GL_REPEAT;
	glScalef(4.f, 4.f, 1.f);
    } else {
	wrap = GL_CLAMP;
	glLoadIdentity();
    }
    glMatrixMode(GL_MODELVIEW);

    for(i = 0; i < texture_count; i++) {
	glBindTexture(GL_TEXTURE_2D, i+1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    }
}

void help(void) {
    printf("Usage: fire image0 ... imagen\n");
    printf("'h'            - help\n");
    printf("'a'            - toggle alpha test\n");
    printf("'b'            - toggle blend\n");
    printf("'s'            - single step\n");
    printf("'t'            - toggle MODULATE or REPLACE\n");
    printf("'m'            - marshmallow\n");
    printf("'x'            - toggle animation\n");
    printf("left mouse     - pan\n");
    printf("right mouse    - rotate\n");
}

void init(int argc, char *argv[]) {
    unsigned *image;
    int i, width, height, components;
    GLfloat pos[] = { 0.f, 1.f, 1.f, 0.f};

    glEnable(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for(i = 0; i < argc; i++) {
	image = read_texture(argv[i], &width, &height, &components);
	if (image == NULL) {
	    fprintf(stderr, "Error: Can't load image file \"%s\".\n",
		    argv[i]);
	    exit(EXIT_FAILURE);
	} else {
	    printf("%d x %d image loaded\n", width, height);
	}
	glBindTexture(GL_TEXTURE_2D, i+1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	texture_count++;
    }

    glBindTexture(GL_TEXTURE_2D, 1+texture_count);
    image = read_texture("../data/smoke.bw", &width, &height, &components);
    if (image == NULL) {
	fprintf(stderr, "Error: Can't load image file \"%s\".\n", "smoke.la");
	exit(EXIT_FAILURE);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
	     height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    smoke = new_smoke(0.f, 0.f, 0.f, .0f, 2.5f, 0.f, 25, .4f, 1+texture_count);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
    glClearColor(.25f, .25f, .25f, .25f);

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.,1.,.1,20.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-5.5);
    glClearColor(.25f, .25f, .75f, .25f);

    glAlphaFunc(GL_GREATER, 0.016f);
    glEnable(GL_ALPHA_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
}

void
animate(void) {
    static int cnt;
    if (cnt++ == 2) {
	the_texture++;
	if (the_texture >= texture_count) the_texture = 0;
	cnt = 0;
    }
    update_smoke(smoke, .003);
    glutPostRedisplay();
}

void
xfunc(void) {
    static int state = 1;
    glutIdleFunc((state ^= 1) ? animate : NULL);
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

void
logs(void) {
    static GLUquadricObj *quadric;

    if (!quadric) {
	quadric = gluNewQuadric();
	glNewList(100, GL_COMPILE);
	gluQuadricOrientation(quadric, GLU_OUTSIDE);
	glTranslatef(0.f, 0.f, -2.f);
	gluCylinder(quadric, .5, .5, 4., 5, 1);
	glTranslatef(0.f, 0.f, 4.f);
	gluDisk(quadric, 0., .5, 10, 1);
	glTranslatef(0.f, 0.f, -4.f);
	gluQuadricOrientation(quadric, GLU_INSIDE);
	gluDisk(quadric, 0., .5, 10, 1);
	glEndList();
    }
    glPushMatrix();
    glTranslatef(0.f, -.5f, 0.f);
    glColor3f(.55f, .14f, .14f);
    glPushMatrix();
    glRotatef(55., 0., 1., 0.);
    glCallList(100);
    glPopMatrix();
    glPushMatrix();
    glRotatef(-55., 0., 1., 0.);
    glCallList(100);
    glPopMatrix();
    if (marshmallow) {
	glPushMatrix();
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glTranslatef(0.f, 1.7f, 0.f);
	glRotatef(45.f, 0.f, 0.f, 1.f);
	glRotatef(90.f, 0.f, 1.f, 0.f);
	glScalef(1., 1., .25f);
	glCallList(100);
	glPopMatrix();

	glPushMatrix();
	glColor4f(.1f, .1f, .1f, 1.f);
	glTranslatef(1.5f, 3.4f, 0.f);
	glRotatef(45.f, 0.f, 0.f, 1.f);
	glRotatef(90.f, 0.f, 1.f, 0.f);
	glScalef(.2f, .2f, 1.f);
	glCallList(100);
	glPopMatrix();

	if (marshmallow == 2) {
	    extern void drawDinosaur(void);
	    glDisable(GL_COLOR_MATERIAL);
	    glPushMatrix();
	    glTranslatef(10.f, 0.f, 0.f);
	    glRotatef(180.f, 0.f, 1.f, 0.f);
	    glScalef(.5f, .5f, .5f);
	    drawDinosaur();
	    glPopMatrix();
	}
    }
    glPopMatrix();
}
static void calcMatrix(void);

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
#define RAD(x) (((x)*M_PI)/180.)
    gluLookAt(-sinf(RAD(rotx))*5.5,transy,cosf(RAD(rotx))*5.5, 0.,0.,0., 0.,1.,0.);

    glTranslatef(0.f, 0.f, transx*10.f);

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
    glColor3f(.1f,.1f,.1f);
    glPushMatrix();
    glTranslatef(-1.f, -1.+.2f, -1.5f);
    glScalef(.2f,.2f, .2f);
    logs();
    /*cube();*/
    glDisable(GL_LIGHTING);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.f, -1.f+.2f, -1.5f);
    calcMatrix();
    draw_smoke(smoke);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(/*(delta/2.f*/-1.f, /*delta*/-.25f, -1.5f);
    calcMatrix();
    glScalef(1.f,1.f,1.);
    if (texture) {
	glBindTexture(GL_TEXTURE_2D, the_texture+1);
	glEnable(GL_TEXTURE_2D);
    }
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
    case 'm': mfunc(); break;
    case 's': sfunc(); break;
    case 't': tfunc(); break;
    case 'x': xfunc(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow(argv[0]);
    init(argc-1, argv+1);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(animate);
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
