#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


/* taken from skyfly */
static float paper_plane_vertexes[] = {
/*Nx  Ny  Nz   Vx     Vy    Vz */
/* ----------------------------    Top view of plane, middle stretched open  */
 0.2, 0., .98, -.10,    0,  .02,/* vertex #'s      4 (.48,0,-.06)            */
 0., 0., 1.,   -.36,  .20, -.04,/*                 .                         */
 0., 0., 1.,    .36,  .01,    0,/*                ...                        */
 0., 0.,-1.,   -.32,  .02,    0,/*                 .             +X          */
 0., 1., 0.,    .48,    0, -.06,/*               2 . 6,8          ^          */
 0., 1., 0.,   -.30,    0, -.12,/*               . . .            |          */
 0.,-1., 0.,    .36, -.01,    0,/*              .. . ..           |          */
 0.,-1., 0.,   -.32, -.02,    0,/*               . . .            |          */
 0., 0.,-1.,    .36, -.01,    0,/*             . . . . .  +Y<-----*          */
 0., 0.,-1.,   -.36, -.20, -.04,/*               . . .     for this picture  */
 -0.2, 0., .98,  -.10,  0,  .02,/*            .  . . .  .  coord system rot. */
 -0.2, 0., -.98, -.10,  0,  .02,/*               . . .     90 degrees        */
 0., 0., -1.,  -.36,  .20, -.04,/*           .   . . .   .                   */
 0., 0., -1.,   .36,  .01,    0,/*               . # .           # marks     */
 0., 0., 1.,   -.32,  .02,    0,/*          .    . . .    .   (0,0) origin   */
 0., -1., 0.,   .48,    0, -.06,/*               . . .         (z=0 at top   */
 0., -1., 0.,  -.30,    0, -.12,/*         .     0 . 10    .    of plane)    */
 0.,1., 0.,     .36, -.01,    0,/*             . . . . .                     */
 0.,1., 0.,    -.32, -.02,    0,/*        .  .   . . .   .  .                */
 0., 0.,1.,     .36, -.01,    0,/*         .     . . .     .                 */
 0., 0.,1.,    -.36, -.20, -.04,/*       1.......3.5.7.......9               */
 0.2, 0., -.98,  -.10,  0,  .02,/* (-.36,.2,-.04)                            */
};

#define MAX_TIME	(2*196)
#define MAX_VAPOR	1024
int vapors = 0;
static struct vapor {
    float x, y, z;
    int time;
    float size;
} vapor[MAX_VAPOR];

static float ttrans[2];
static float scale = 1.;
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
static int show_t = 0;
static int mot;
#define PAN	1
#define ROT	2

static int _time = 1;
static float _x = -1.;
static float _y = .75;

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

void wire(void) {
    static int w;
    if (w ^= 1)
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void light(void) {
    static int l;
    if (l ^= 1)
	glEnable(GL_LIGHTING);
    else
	glDisable(GL_LIGHTING);
}

void up(void) { scale += .1; }
void down(void) { scale -= .1; }

void
draw_plane(void) {
    glEnable(GL_LIGHTING);
    glShadeModel(GL_FLAT);
    glRotatef(-120.f, 1.f, 0.f, 0.f);
#define nv(p) glNormal3fv(paper_plane_vertexes+6*p); glVertex3fv(paper_plane_vertexes+6*p+3)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glBegin(GL_TRIANGLE_STRIP);
    nv(0); nv(1); nv(2); nv(3); nv(4); nv(5); nv(6); nv(7); nv(8); nv(9); nv(10);
    glEnd();
    glCullFace(GL_BACK);
    glBegin(GL_TRIANGLE_STRIP);
    nv(11); nv(12); nv(13); nv(14); nv(15); nv(16); nv(17); nv(18); nv(19); nv(20); nv(21);
    glEnd();
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_LIGHTING);
#undef nv
}

void
add_vapor(void) {
    int i;
    /* garbage collect */
    for(i = 0; i < vapors; i++)
	if (_time - vapor[i].time > MAX_TIME) {
	    memcpy(vapor+i, vapor+i+1, (vapors-i-1)*sizeof vapor[0]);
	    vapors--;
	}
    if (vapors >= MAX_VAPOR) {
	printf("max_vapors\n");
	return;
    }
    vapor[vapors].time = _time;
    vapor[vapors].x = -.6f + _x;
    vapor[vapors].y = _y;
    vapor[vapors].z = 0.f;
    vapor[vapors].size = 1.f;
    vapors++;
    if (_x > 8.5) {
	_y = .75f;
	_x = -2.f;
	vapors = 0;
    }
}

void
animate(void) {
    static int cnt = 0;
    ttrans[0] += .01;
    if (ttrans[0] == 1.0) ttrans[0] = 0;
    ttrans[1] += .005;
    if (ttrans[1] == 1.0) ttrans[1] = 0;
    _y -= .01f;
    _x +=.025f;
    if (cnt++ & 1) add_vapor();
    _time++;
    glutPostRedisplay();
}

void help(void) {
    printf("Usage: vapor [image]\n");
    printf("'h'            - help\n");
    printf("'l'            - toggle lighting\n");
    printf("'t'            - toggle wireframe\n");
    printf("'UP'           - scale up\n");
    printf("'DOWN'         - scale down\n");
    printf("left mouse     - pan\n");
    printf("middle mouse   - rotate\n");
}

void init(char *filename) {
    static GLfloat plane_mat[] = { 1.f, 1.f, .2f, 1.f };
    static unsigned *image;
    static int width, height, components, i;
    if (filename) {
	image = read_texture(filename, &width, &height, &components);
	if (image == NULL) {
	    fprintf(stderr, "Error: Can't load image file \"%s\".\n",
		    filename);
	    exit(EXIT_FAILURE);
	} else {
	    printf("%d x %d image loaded\n", width, height);
	}
	if (components != 2 && components != 4) {
	    printf("must be an rgba or la image\n");
	    exit(EXIT_FAILURE);
	}
	for(i = 0; i < width*height; i++)
		image[i] = image[i] | 0xffffff00;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.,1.,.1,10.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-5.5);

    glEnable(GL_LIGHT0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, plane_mat);

    glClearColor(0.1, 0.1, 0.6, 1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0./255.);

    glEnable(GL_DEPTH_TEST);
}

void
draw_vapor(void) {
    int i;
    float intensity = .9f;
    float t;
    struct vapor *v;

    glDepthMask(0);
    glEnable(GL_TEXTURE_2D);

    for(i = 0; i < vapors; i++) {
	v = vapor+i;
	t = _time - v->time;

	glPushMatrix();
	glTranslatef(v->x, v->y, v->z);
	glScalef(.5*(t+40)/MAX_TIME, .5*(t+40)/MAX_TIME, 1.);
	glColor4f(intensity,intensity,intensity,10./(t+0.f));
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-1., -1., -0.);
	glTexCoord2f(0, 1); glVertex3f(-1., 1.,  0.);
	glTexCoord2f(1, 1); glVertex3f( 1., 1.,  0.);
	glTexCoord2f(1, 0); glVertex3f( 1., -1., -0.);
	glEnd();
	if (show_t) {
	    glDisable(GL_TEXTURE_2D);
	    glColor4f(0.,0.,0.,1.0);
	    glBegin(GL_LINE_LOOP);
	    glVertex3f(-1., -1., -0.);
	    glVertex3f(-1., 1.,  0.);
	    glVertex3f( 1., 1.,  0.);
	    glVertex3f( 1., -1., -0.);
	    glEnd();
	    glEnable(GL_TEXTURE_2D);
	}
	glPopMatrix();
    }
    glDisable(GL_TEXTURE_2D);
    glDepthMask(1);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glPushMatrix();
    glColor4f(1.f,1.f,.2f,1.f);
    glTranslatef(_x, _y+.05f, 0.f);
    glScalef(2.f,2.f,2.f);
    glRotatef(-10.f, 0.f, 0.f, 1.f);
    draw_plane();
    glPopMatrix();
    draw_vapor();
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
    case 't': toggle_t(); break;
    case 'w': wire(); break;
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
    }
}

int main(int argc, char** argv) {
    glutInitWindowSize(256, 256);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow(argv[0]);
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
