#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

#ifndef __sgi
/* Most math.h's do not define float versions of the math functions. */
#define expf(x) ((float)exp((x)))
#endif

static float ttrans[2];
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

void
animate(void) {
    ttrans[0] += .010f;
    if (ttrans[0] == 1.0f) ttrans[0] = 0.0f;
    ttrans[1] -= .020f;
    if (ttrans[1] <= 0.0f) ttrans[1] = 1.0f;
    glutPostRedisplay();
}

void xfunc(void) {
    static state = 1;
    glutIdleFunc((state ^= 1) ? animate : NULL);
}

void help(void) {
    printf("Usage: cloudl [image]\n");
    printf("'h'            - help\n");
    printf("'x'            - toggle cloud motion\n");
    printf("left mouse     - pan\n");
    printf("middle mouse   - rotate\n");
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

    fog_color[0] = .23*.2 + density *.57*.2;
    fog_color[1] = .35*.2 + density *.45*.2;
    fog_color[2] = .78*.5 + density *.22*.2;

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

void draw_lights(void) {
    int i;

    glEnable(GL_BLEND);
    glColor4f(.7f, .7f, .1f, 1.0);
    glPushMatrix();
    glScalef(.1f, 1.f, 1.f);
    glEnable(GL_POINT_SMOOTH);
    for(i = 0; i <= 20; i++) {
	glPointSize((float)i/2.);
	glBegin(GL_POINTS);
	glVertex3f(-1.f, 0.f, -1.f+2.f/20*i);
	glVertex3f( 1.f, 0.f, -1.f+2.f/20*i);
	glEnd();
    }
    glPopMatrix();
    glDisable(GL_BLEND);
}

void
draw_clouds(void) {
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(ttrans[0], ttrans[1], 0.);
    glColor4f(1.f, 1.f, 1.f, 1.0);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1., .2, -1.);
    glTexCoord2f(0, 5); glVertex3f(-1., .2,  1.);
    glTexCoord2f(5, 5); glVertex3f( 1., .2,  1.);
    glTexCoord2f(5, 0); glVertex3f( 1., .2, -1.);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(transx, transy, 0.f);
    glRotatef(rotx, 0., 1., 0.);
    glRotatef(roty, 1., 0., 0.);
    glScalef(10,1,10);
    glTranslatef(0.f,-1.f,0.f);
    draw_base();
    draw_runway();
    draw_lights();
    draw_clouds();
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
    case 'x': xfunc(); break;
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
    init(argc == 1 ? "../data/clouds.bw" : argv[1]);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(animate);
    glutMainLoop();
    return 0;
}
