#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "texture.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep((x*1000))
#else
#include <unistd.h>
#endif

#include <GL/glut.h>

const char defaultBaseName[] = "../data/";

GLuint *faces[6];
GLsizei faceW[6], faceH[6];

GLfloat angle1[6] = {90, 180, 270, 0, 90, -90};
GLfloat axis1[6][3] = {{0,1,0}, {0,1,0}, {0,1,0}, {0,1,0}, {1,0,0}, {1,0,0}};
GLfloat angle2[6] = {0, 0, 0, 0, 180, 180};
GLfloat axis2[6][3] = {{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,1,0}, {0,1,0}};

GLuint *load_texture(const char *fname, GLsizei *w, GLsizei *h)
{
    int comps;
    GLuint *img;
    int i;

    img = read_texture(fname, w, h, &comps);
    if (!img) {
	fprintf(stderr, "Could not open %s\n", fname);
	exit(1);
    }

    for (i = 0; i < *w * *h; i++) {
	img[i] |= 0xff;
    }

    return img;
}

void set_texture_border(GLuint val, GLuint mask, 
			GLsizei w, GLsizei h, GLuint *pix)
{
    int x, y;

    val &= mask;
    mask = ~mask;

    /* top & bottom rows */
    for (x = 0; x < w; x++) {
	pix[x] = (pix[x] & mask) | val;
	pix[x + (h-1)*w] = (pix[x + (h-1)*w] & mask) | val;
    }

    for (y = 0; y < h; y++) {
	pix[y*w] = (pix[y*w] & mask) | val;
	pix[y*w + (w-1)] = (pix[y*w + (w-1)] & mask) | val;
    }
}

void init(void)
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void reshape(GLsizei w, GLsizei h) 
{
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(-2, 2, -2, 2, 0, 5);
}

void draw_special_sphere(int tess)
{
    float r = 1.0, r1, r2, z1, z2;
    float theta, phi;
    int nlon = tess, nlat = tess;
    int i, j;

    glBegin(GL_TRIANGLE_FAN);
    theta = M_PI*1.0/nlat;
    r2 = r*sin(theta); z2 = r*cos(theta);
    glNormal3f(0.0, 0.0, 1.0);
    glVertex4f(0.0, 0.0, r*r, r);
    for (j = 0, phi = 0.0; j <= nlon; j++, phi = 2*M_PI*j/nlon) {
	glNormal3f(r2*cos(phi), r2*sin(phi), z2);
	glVertex4f(r2*cos(phi)*z2, r2*sin(phi)*z2, z2*z2, z2); /* top */
    }
    glEnd();

    for (i = 2; i < nlat; i++) {
	theta = M_PI*i/nlat;
	r1 = r*sin(M_PI*(i-1)/nlat); z1 = r*cos(M_PI*(i-1)/nlat);
	r2 = r*sin(theta); z2 = r*cos(theta);

	if (fabs(z1) < 0.01 || fabs(z2) < 0.01)
	    break;

	glBegin(GL_QUAD_STRIP);
	for (j = 0, phi = 0; j <= nlat; j++, phi = 2*M_PI*j/nlon) {
	    glNormal3f(r1*cos(phi), r1*sin(phi), z1);
	    glVertex4f(r1*cos(phi)*z1, r1*sin(phi)*z1, z1*z1, z1);
	    glNormal3f(r2*cos(phi), r2*sin(phi), z2);
	    glVertex4f(r2*cos(phi)*z2, r2*sin(phi)*z2, z2*z2, z2);
	}
	glEnd();
    }
}

void render_spheremap(int width, int height)
{
    GLfloat p[4];
    int i;

    glColor4f(1, 1, 1, 1);

    glEnable(GL_TEXTURE_2D);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    p[0] = 2.0; p[1] = p[2] = p[3] = 0.0; /* 2zx */
    glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
  
    glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    p[0] = 0.0; p[1] = 2.0; p[2] = p[3] = 0.0; /* 2zy */
    glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
  
    glTexGenf(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    p[0] = p[1] = 0.0; p[2] = 0.0; p[3] = 2.0; /* 2z */
    glTexGenfv(GL_R, GL_OBJECT_PLANE, p);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
  
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, 1.0, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 6,
	      0, 0, 0,
	      0, 1, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (i = 0; i < 6; i++) {
	glTexImage2D(GL_TEXTURE_2D, 0, 4, faceW[i], faceH[i], 0, 
		     GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)faces[i]);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(0.5, 0.5, 1.0);
	glTranslatef(1.0, 1.0, 0.0);
	glFrustum(-1.01, 1.01, -1.01, 1.01, 1.0, 100.0);
	if (angle2[i]) {
	    glRotatef(angle2[i], axis2[i][0], axis2[i][1], axis2[i][2]);
	}
	glRotatef(angle1[i], axis1[i][0], axis1[i][1], axis1[i][2]);
    
	/* XXX atul does another angle thing here... */
	/* XXX atul does a third angle thing here... */
    
	glTranslatef(0.0, 0.0, -1.00);
    
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_DEPTH_BUFFER_BIT);
	draw_special_sphere(20);

	sleep(1);
    }

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);

    glDisable(GL_TEXTURE_2D);
}

void draw(void)
{
    GLenum err;

    glClear(GL_COLOR_BUFFER_BIT);

    render_spheremap(256, 256);

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);
}

void show_usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "genspheremap -- use default files"
	    "(%s00.rgb through %s05.rgb)\n", defaultBaseName, defaultBaseName);
    fprintf(stderr, "genspheremap baseName -- use files of the form "
	    "baseName0.rgb through baseName5.rgb\n");
    fprintf(stderr, "genspheremap f0.rgb f1.rgb f2.rgb f3.rgb f4.rgb f5.rgb\n");
}

main(int argc, char *argv[])
{
    const char *baseName;
    char fname[128];
    int i;

    glutInitWindowSize(512, 512);
    glutInitWindowPosition(0, 0);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    init();

    if (argc == 1 || argc == 2) {
	if (argc == 1) baseName = defaultBaseName;
	else baseName = argv[1];
	assert(strlen(baseName) < 128 - (strlen("0.rgb") + 1));
	for (i = 0; i < 6; i++) {
	    sprintf(fname, "%s%02d.rgb", baseName, i);
	    faces[i] = load_texture(fname, &faceW[i], &faceH[i]);
	}
    } else if (argc == 7) {
	for (i = 0; i < 6; i++) {
	    faces[i] = load_texture(fname, &faceW[i], &faceH[i]);
	}
    } else {
	show_usage();
	exit(1);
    }
  
    for (i = 0; i < 6; i++) {
	set_texture_border(0x00, 0xff, faceW[i], faceH[i], faces[i]);
    }
  
    glutMainLoop();
    return 0;
}
