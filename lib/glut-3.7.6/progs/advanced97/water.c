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

static int rgb;
static int mesh = 1;
static float ttrans[2];
static float transx, transy, rotx, roty;
static float amplitude = 0.03;
static float freq = 5.0f;
static float phase = .00003;
static int ox = -1, oy = -1;
static int show_t = 1;
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
	case GLUT_MIDDLE_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

void toggle_t(void) {
    show_t ^= 1;
}

void ffunc(void) { freq *= 2.f; }
void Ffunc(void) { freq /= 2.f; }
void mfunc(void) { mesh ^= 1; }

void wire(void) {
    static int w;
    if (w ^= 1) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_BLEND);
    } else {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_BLEND);
    }
}

void light(void) {
    static int l;
    if (l ^= 1)
	glEnable(GL_LIGHTING);
    else
	glDisable(GL_LIGHTING);
}

void up(void) { amplitude += .01; }
void down(void) { amplitude -= .01; }
void left(void) { phase -= .00001; }
void right(void) { phase += .00001; }

void
animate(void) {
    ttrans[0] += .005f;
    if (ttrans[0] == 1.0f) ttrans[0] = 0.0f;
    ttrans[1] -= .0025f;
    if (ttrans[1] <= 0.0f) ttrans[1] = 1.0f;
    glutPostRedisplay();
}

void xfunc(void) {
    static state = 1;
    glutIdleFunc((state ^= 1) ? animate : NULL);
}

void help(void) {
    printf("Usage: water [image]\n");
    printf("'h'            - help\n");
    printf("'l'            - toggle lighting\n");
    printf("'f'            - increase frequency\n");
    printf("'F'            - decrease frequency\n");
    printf("'m'            - toggle mesh\n");
    printf("'t'            - toggle wireframe\n");
    printf("'x'            - toggle water motion\n");
    printf("'UP'           - increase amplitude\n");
    printf("'DOWN'         - decrease amplitude\n");
    printf("'RIGHT'        - increase phase change\n");
    printf("'LEFT'         - decreae phase change\n");
    printf("left mouse     - pan\n");
    printf("middle mouse   - rotate\n");
}

void init(char *filename) {
    GLfloat cloud_color[4] = { 1., 1., 1., 0., };
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
	if (components < 3) rgb = 0;
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
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, cloud_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
    glEnable(GL_TEXTURE_2D);
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

    fog_color[0] = .23 + density *.57;
    fog_color[1] = .35 + density *.45;
    fog_color[2] = .78 + density *.22;

    glClearColor(fog_color[0], fog_color[1], fog_color[2], 1.f);

    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, fog_density);
    glFogfv(GL_FOG_COLOR, fog_color);
    if (fog_density > 0)
	glEnable(GL_FOG);
    glLineWidth(2.0f);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void draw_mesh(void) {
    if (mesh) {
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-1.f, 0.f, -1.f);
	glTexCoord2f(0, 1); glVertex3f(-1.f, 0.f,  1.f);
	glTexCoord2f(1, 1); glVertex3f( 1.f, 0.f,  1.f);
	glTexCoord2f(1, 0); glVertex3f( 1.f, 0.f, -1.f);
	glEnd();
    } else {
#define MESH 32
	int i, j;
	static float off;
	float d = 1.f/MESH;
	for(i = 0; i < MESH; i++) {
	    glBegin(GL_TRIANGLE_STRIP);
	    for(j = 0; j < MESH; j++) {
		float s = (float)j*d;
		float t = (float)i*d;
		float x = -1.0 + 2.f*s;
		float z = -1.0 + 2.f*t;
		float y = amplitude*sinf(freq*2.f*M_PI*t+off);
		glTexCoord2f(s, t); glVertex3f(x, y, z);
		s += d; t += d;
		x = -1.0 + 2.f*s;
		z = -1.0 + 2.f*t;
		y = amplitude*sinf(freq*2.f*M_PI*t+off);
		glTexCoord2f(s, t); glVertex3f(x, y, z);
		off += phase;
	    }
	    glEnd();
	}
    }
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(transx, transy, 0.f);
    glRotatef(rotx, 0., 1., 0.);
    glRotatef(roty, 1., 0., 0.);
    glScalef(10,1,10);
    if (!rgb)
	glColor3f(.31, .41, .97);
    else
	glColor3f(1.f,1.f,1.f);
    glTranslatef(0.f,-1.f,0.f);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(ttrans[0], ttrans[1], 0.);
    glScalef(10.f, 10.f,1.f);
    draw_mesh();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
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
    case 'l': light(); break;
    case 'f': ffunc(); break;
    case 'F': Ffunc(); break;
    case 't': toggle_t(); break;
    case 'm': mfunc(); break;
    case 'w': wire(); break;
    case 'x': xfunc(); break;
    case 'h': help(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

/*ARGSUSED1*/
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
    glutInitWindowSize(256, 256);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    (void)glutCreateWindow(argv[0]);
    init(argc == 1 ? "../data/water.bw" : argv[1]);
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
