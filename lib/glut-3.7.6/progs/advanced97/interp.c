#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include "texture.h"

static char defaultFile[] = "../data/mandrill.rgb";
GLuint *img, *black, *avgLum, *lum;
GLsizei w, h;
GLint comp;
GLboolean isIR;

#define RW 0.3086
#define GW 0.6094
#define BW 0.0820

GLuint *in0, *in1;
GLfloat x = 1.;

#define BRIGHTNESS	0
#define CONTRAST	1
#define SATURATION	2
int nOperations = 3;
int operation = 0;


void set_img_pointers(void)
{
    switch(operation) {
    case CONTRAST:
	in0 = avgLum;
	printf("modifying contrast\n");
	break;
    case SATURATION:
	in0 = lum;
	printf("modifying saturation\n");
	break;
    case BRIGHTNESS:
	in0 = black;
	printf("modifying brightness\n");
	break;
    default:
	assert(0);
    }
}

void init(void)
{
    const char *renderer;

    set_img_pointers();

    renderer = (char*) glGetString(GL_RENDERER);
    isIR = (renderer[0] == 'I' && renderer[1] == 'R');
}

GLuint *alloc_image(void)
{
    GLuint *ptr;

    ptr = (GLuint *)malloc(w * h * sizeof(GLuint));
    if (!ptr) {
	fprintf(stderr, "malloc of %d bytes failed.\n", w * h * sizeof(GLuint));
    }
    return ptr;
}

void load_img(const char *fname)
{
    int i;
    GLubyte *src, *dst;
    GLfloat pix, avg;

    img = read_texture(fname, &w, &h, &comp);
    if (!img) {
	fprintf(stderr, "Could not open %s\n", fname);
	exit(1);
    }

    black = alloc_image();
    memset(black, 0, w * h * sizeof(GLuint));

    lum = alloc_image();
    src = (GLubyte *)img;
    dst = (GLubyte *)lum;
    avg = 0.;
    /* compute average luminance at same time that we set luminance image.
     * note that little care is taken to avoid mathematical error when
     * computing overall average... */
    for (i = 0; i < w * h; i++) {
	pix = (float)src[0]*RW + (float)src[1]*GW + (float)src[2]*BW;
	if (pix > 255) pix = 255;
	dst[0] = dst[1] = dst[2] = pix;
	avg += pix / 255.;
	src += 4;
	dst += 4;
    }

    avgLum = alloc_image();
    pix = avg * 255. / (float)(w*h);
    dst = (GLubyte *)avgLum;
    for (i = 0; i < w * h; i++) {
	dst[0] = dst[1] = dst[2] = pix;
	dst += 4;
    }
}

void reshape(GLsizei winW, GLsizei winH) 
{
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(0, winW, 0, winH, 0, 5);
}

void draw(void)
{
    GLenum err;
    GLfloat s, absx, abs1minusx;

    glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
    glRasterPos2i(0, 0);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, in0);

    absx = fabs(x);
    abs1minusx = fabs(1.-x);
    s = absx > abs1minusx ? absx : abs1minusx;

    if (!isIR) {
	glAccum(GL_ACCUM, (1. - x) / s);
	glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, in1);
	glAccum(GL_ACCUM, x/s);
	glAccum(GL_RETURN, s);
    } else {
	if (fabs(x) < 1. && fabs(1. - x) < 1) {
	    glAccum(GL_ACCUM, 1. - x);
	    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, in1);
	    glAccum(GL_ACCUM, x);
	    glAccum(GL_RETURN, 1);	
	} else {
	    absx = fabs(x);
	    abs1minusx = fabs(1.-x);
	    s = absx > abs1minusx ? absx : abs1minusx;

	    glAccum(GL_ACCUM, .8 * ((1. - x) / s));
	    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, in1);
	    glAccum(GL_ACCUM, .8 * x/s);
	    glAccum(GL_RETURN, 1.2 * s);
	}
    }

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

/* ARGSUSED1 */
void key(unsigned char key, int xpos, int ypos)
{
    if (key == 27) exit(0);

    operation = (operation + 1) % nOperations;
    set_img_pointers();
    draw();
}

int mouseOriginX;
int moveCalled = 0;
float deltaSinceDraw = 0.;

void idle(void);

void clamp_x(void)
{
    if (x > 3.99 && isIR) x = 3.99;
    else if (x < -4. && isIR) x = -4.;
}

/* ARGSUSED */
void button(int button, int state, int xpos, int ypos)
{
    if (state == GLUT_DOWN) {
	mouseOriginX = xpos;
	deltaSinceDraw = 0;
	glutIdleFunc(idle);
	return;
    }

    if (!moveCalled) {
	if (button == GLUT_MIDDLE_BUTTON) x = 1.;
	else x = (float)xpos / ((float)w / 2.);
	clamp_x();
	deltaSinceDraw = 100000.;
    } 
    moveCalled = 0;
    if (deltaSinceDraw) draw();
    printf("x = %f\n", x);
    glutIdleFunc(NULL);
}

void menu(int val) 
{
    operation = val;
    set_img_pointers();
    draw();
}

void idle(void)
{
    if (deltaSinceDraw) {
	x += deltaSinceDraw;
	clamp_x();
	draw();
	deltaSinceDraw = 0;
    }
}

/* ARGSUSED */
void motion(int xpos, int ypos)
{
    float delta = xpos - mouseOriginX;
    mouseOriginX = xpos;
    moveCalled = 1;
    delta /= w/2.;
    deltaSinceDraw += delta;
}

main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    if (argc > 1) {
	load_img(argv[1]);
    } else {
	load_img(defaultFile);
    }
    in0 = black;
    in1 = img;
    operation = BRIGHTNESS;
    glutInitWindowSize(w, h);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_ACCUM | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutMotionFunc(motion);
    glutMouseFunc(button);
    glutReshapeFunc(reshape);

    glutCreateMenu(menu);
    glutAddMenuEntry("Change brightness", 0);
    glutAddMenuEntry("Change contrast", 1);
    glutAddMenuEntry("Change saturation", 2);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();

    glutMainLoop();
    return 0;
}
