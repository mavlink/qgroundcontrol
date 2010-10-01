#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

static char defaultFile[] = "../data/mandrill.rgb";
GLuint *img;
GLsizei w, h;
GLint comp;

GLfloat kernScale;

/* a few filters... */
GLfloat horizontalEdge3x3[] = {
    -1, -1, -1,
    2,  2,  2,
    -1, -1, -1,
};

GLfloat verticalEdge3x3[] = {
    -1,  2, -1,
    -1,  2, -1,
    -1,  2, -1,  
};

GLfloat allEdge3x3[] = {
    -2, 1, -2,
    1, 4,  1,
    -2, 1, -2,
};

GLfloat smooth3x3[] = {
    1, 2, 1,
    2, 4, 2,
    1, 2, 1,
};

GLfloat highpass3x3[] = {
    -1, -1, -1, 
    -1,  9, -1,
    -1, -1, -1,
};

GLfloat laplacian5x5[] = {
    1,  1,  1,  1,  1,  
    1,  1,  1,  1,  1,  
    1,  1, 24,  1,  1,  
    1,  1,  1,  1,  1,  
    1,  1,  1,  1,  1,  
};

GLfloat box3x3[] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
};

GLfloat box5x5[] = {
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 
};

struct Kernel {
    const char *name;
    GLfloat *kern;
    GLsizei w, h;
} filters[] = {
    {"horizontal edge detect", horizontalEdge3x3, 3, 3},
    {"vertical edge detect", verticalEdge3x3, 3, 3},
    {"all edge detect", allEdge3x3, 3, 3},
    {"3x3 smooth", smooth3x3, 3, 3},
    {"3x3 highpass", highpass3x3, 3, 3},
    {"5x5 laplacian", laplacian5x5, 5, 5},
    {"3x3 box", box3x3, 3, 3},
    {"5x5 box", box5x5, 5, 5},
};
struct Kernel *kern;

void kern_normalize(void)
{
    GLfloat total, scale, *k;
    int i;

    k = kern->kern;
    total = 0;
    for (i = 0; i < kern->w*kern->h; i++) {
	total += *k++;
    }

    if (!total) {
	/* kernel sums to 0... */
	return;
    } else {
	scale = 1. / total;
	k = kern->kern;
	for (i = 0; i < kern->w * kern->h; i++) {
	    *k *= scale;
	    k++;
	}
    }
}

GLfloat acc_kern_scale(void)
{
    GLfloat minPossible = 0, maxPossible = 1;
    GLfloat *k;
    int i;
  
    k = kern->kern;
    for (i = 0; i < kern->w*kern->h; i++) {
	if (*k < 0) {
	    minPossible += *k;
	} else {
	    maxPossible += *k;
	}
	k++;
    }

    return(1. / ((-minPossible > maxPossible) ? -minPossible : maxPossible));
}

void init(void)
{
    kern = &filters[4];
    kern_normalize();
    kernScale = acc_kern_scale();
}

void load_img(const char *fname)
{
    img = read_texture(fname, &w, &h, &comp);
    if (!img) {
	fprintf(stderr, "Could not open %s\n", fname);
	exit(1);
    }
}

void reshape(GLsizei winW, GLsizei winH) 
{
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(0, winW, 0, winH, 0, 5);
}

void acc_convolve(void)
{
    int x, y;

    for (y = 0; y < kern->h; y++) {
	for (x = 0; x < kern->w; x++) {
	    glRasterPos2i(0, 0); 
	    glBitmap(0, 0, 0, 0, -x, -y, 0);
	    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);
	    glAccum(GL_ACCUM, kern->kern[y*kern->w + x]*kernScale);
	}
    }
    glAccum(GL_RETURN, 1./kernScale);
}

void draw(void)
{
    GLenum err;

    glutSetCursor(GLUT_CURSOR_WAIT);

    glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);

    acc_convolve();

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    glutSetCursor(GLUT_CURSOR_INHERIT);
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);
}

void menu(int val)
{
    kern = &filters[val];
    kern_normalize();
    kernScale= acc_kern_scale();
    draw();
}

main(int argc, char *argv[])
{
    int i;

    glutInit(&argc, argv);
    if (argc > 1) {
	load_img(argv[1]);
    } else {
	load_img(defaultFile);
    }
    glutInitWindowSize(w, h);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_ACCUM);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
  
    glutCreateMenu(menu);
    for (i = 0; i < sizeof(filters) / sizeof(filters[0]); i++) {
	glutAddMenuEntry(filters[i].name, i);
    }
    glutAttachMenu(GLUT_RIGHT_BUTTON);
  
    init();
  
    glutMainLoop();
    return 0;
}
