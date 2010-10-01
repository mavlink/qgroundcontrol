#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "texture.h"
#include <GL/glut.h>

#ifndef __sgi
/* Most math.h's do not define float versions of the math functions. */
#define expf(x) ((float)exp((x)))
#define fabsf(x) ((float)fabs((x)))
#endif

static int pstyle = 3;
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot;
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

void pfunc(void) { pstyle = (pstyle+1) % 4; }

void help(void) {
    printf("Usage: lightp [image]\n");
    printf("'h'            - help\n");
    printf("'p'            - toggle point mode\n");
    printf("left mouse     - pan\n");
    printf("right mouse    - rotate\n");
}

void init(char *filename) {
    GLfloat fog_color[4], fog_density = 0.05, density, far_cull;
    unsigned *image;
    int width, height, components;
    if (filename) {
	image = read_texture(filename, &width, &height, &components);
	if (image == NULL) {
	    fprintf(stderr, "Error: Can't load image file \"%s\".\n",
		    filename);
	    exit(EXIT_FAILURE);
	} else {
	    printf("%d x %d image loaded\n", width, height);
	}
	if (components != 1 && components != 2) {
	    printf("must be a l or la image\n");
	    exit(EXIT_FAILURE);
	}
	if (components == 1) {
	    /* hack for RE */
	    int i;
	    GLubyte *p = (GLubyte *)image;
	    for(i = 0; i < width*height; i++) {
		p[i*4+3] = p[i*4+0];
	    }
	    components = 2;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
    /*glEnable(GL_TEXTURE_2D);*/
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.,1.,.1,far_cull = 10.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-5.5);

    density = 1.- expf(-5.5 * fog_density * fog_density *
			      far_cull * far_cull);

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
    density = MAX(MIN(density, 1.), 0.);

    fog_color[0] = .23*.19 + density *.57*.19;
    fog_color[1] = .35*.19 + density *.45*.19;
    fog_color[2] = .78*.19 + density *.22*.19;

    glClearColor(fog_color[0], fog_color[1], fog_color[2], 1.f);

    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, fog_density);
    glFogfv(GL_FOG_COLOR, fog_color);
    if (fog_density > 0)
	glEnable(GL_FOG);
    glLineWidth(2.0f);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(10.f);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

void draw_base(void) {
    glColor4f(.1, .3, .1, 1.0);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1.f, 0.f, -1.f);
    glTexCoord2f(0, 1); glVertex3f(-1.f, 0.f,  1.f);
    glTexCoord2f(1, 1); glVertex3f( 1.f, 0.f,  1.f);
    glTexCoord2f(1, 0); glVertex3f( 1.f, 0.f, -1.f);
    glEnd();
}

void draw_runway(void) {
    glColor4f(.1, .1, .1, 1.0);
    glPushMatrix();
    glScalef(.1f, 1.f, 1.f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1.f, 0.f, -1.f);
    glTexCoord2f(0, 1); glVertex3f(-1.f, 0.f,  1.f);
    glTexCoord2f(1, 1); glVertex3f( 1.f, 0.f,  1.f);
    glTexCoord2f(1, 0); glVertex3f( 1.f, 0.f, -1.f);
    glEnd();
    glPopMatrix();
}

/* v0 = v1*mat */
void xform(float *v0, float *mat, float *v1) {
    v0[0] = v1[0]*mat[0] + v1[1]*mat[4] + v1[2]*mat[8] + v1[3]*mat[12];
    v0[1] = v1[1]*mat[1] + v1[1]*mat[5] + v1[2]*mat[9] + v1[3]*mat[13];
    v0[2] = v1[2]*mat[2] + v1[1]*mat[6] + v1[2]*mat[10] + v1[3]*mat[14];
    v0[3] = v1[3]*mat[3] + v1[1]*mat[7] + v1[2]*mat[11] + v1[3]*mat[15];
}

/* m0 = m1*m2 */
void xformm(float *m0, float *m1, float *m2) {
    int i, j;
    for(i = 0; i < 4; i++) {
	for(j = 0; j < 4; j++) {
	    m0[4*i+j] = m1[4*i+0]*m2[4*0+j] + m1[4*i+1]*m2[4*1+j] + m1[4*i+2]*m2[4*2+j] + m1[4*i+3]*m2[4*3+j];
	}
    }
}

void draw_quad(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90.f, 1.f, 0.f, 0.f);
    glScalef(.03f, .03f, .03f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1.f, 0.f, -1.f);
    glTexCoord2f(0, 1); glVertex3f(-1.f, 0.f,  1.f);
    glTexCoord2f(1, 1); glVertex3f( 1.f, 0.f,  1.f);
    glTexCoord2f(1, 0); glVertex3f( 1.f, 0.f, -1.f);
    glEnd();
    glPopMatrix();
}

void draw_lights(void) {
    int i;
    GLfloat mat[16], matv[16], matp[16];
    float v0[4], v1[4], v[4];

    glEnable(GL_BLEND);
    glColor4f(1.f, 1.f, 1.f, 1.0);
    glPushMatrix();
    if (pstyle != 3) {
	glScalef(.1f, 1.f, 1.f);
	if (pstyle == 0) glDisable(GL_POINT_SMOOTH);
	else glEnable(GL_POINT_SMOOTH);
	if (pstyle == 1) glPointSize(13.f);
	glGetFloatv(GL_MODELVIEW_MATRIX, matv);
	glGetFloatv(GL_PROJECTION_MATRIX, matp);
	xformm(mat, matv, matp);
	v[0] = -1.f;
	v[1] = 0.0f;
	v[3] = 1.0f;
	for(i = 0; i <= 20; i++) {
	    if (pstyle == 2) {
		float s;
		v[2] = -1.f+2.f/20*i;
		v[0] -= 1.;
		xform(v0, mat, v);
		v[0] += 2.;
		xform(v1, mat, v);
		v[0] -= 1.;
		s = fabsf(v0[0]/v0[3] - v1[0]/v1[3]);
		glPointSize(10.f*s);
	    }
	    glBegin(GL_POINTS);
	    glVertex3f(-1.f, 0.f, -1.f+2.f/20*i);
	    glVertex3f( 1.f, 0.f, -1.f+2.f/20*i);
	    glEnd();
	}
    } else {
	glEnable(GL_TEXTURE_2D);
	glScalef(1.f, 10.f, 1.f);
	for(i = 0; i <= 20; i++) {
	    float v = -1.f+2.f/20*i;
	    draw_quad(-.1f, 0.f, v);
	    draw_quad( .1f, 0.f, v);
	}
	glDisable(GL_TEXTURE_2D);
    }
    glPopMatrix();
    glDisable(GL_BLEND);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(transx, transy, 0.f);
    glRotatef(rotx, 0., 1., 0.);
    glRotatef(roty, 1., 0., 0.);
    glScalef(10.f,1.f,10.f);
    glTranslatef(0.f,-.4f,0.f);
    draw_base();
    draw_runway();
    draw_lights();
    glPopMatrix();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'p': pfunc(); break;
    case 'h': help(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    (void)glutCreateWindow(argv[0]);
    init(argc == 1 ? "../data/light.bw" : argv[1]);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}
