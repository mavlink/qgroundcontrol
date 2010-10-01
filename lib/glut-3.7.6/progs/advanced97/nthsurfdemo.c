#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TRUE	1
#define FALSE	0

int whichSurface = 0;
int takeSnapshot = 0;


int 		winWidth, winHeight;
GLfloat 	*depthSave = NULL;
GLubyte 	*stencilSave = NULL;
GLubyte 	*colorSave = NULL;


void resizeBuffers(void)
{
    colorSave = realloc(colorSave, winWidth * winHeight * 4 * sizeof(GLubyte));
    depthSave = realloc(depthSave, winWidth * winHeight * 4 * sizeof(GLfloat));
    stencilSave = (GLubyte *)depthSave;
}


void pushOrthoView(float left, float right, float bottom, float top,
    float znear, float zfar)
{
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(left, right, bottom, top, znear, zfar);
}


void popView(void)
{
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void copyDepthToColor(GLenum whichColorBuffer)
{
    int		x, y;
    GLfloat	max, min;
    GLint	previousColorBuffer;

    glReadPixels(0, 0, winWidth, winHeight, GL_DEPTH_COMPONENT, GL_FLOAT,
        depthSave);

    /* I'm sure this could be done much better with OpenGL */
    max = 0;
    min = 1;
    for(y = 0; y < winHeight; y++)
	for(x = 0; x < winWidth; x++) {
	    if(depthSave[winWidth * y + x] < min)
		min = depthSave[winWidth * y + x];
	    if(depthSave[winWidth * y + x] > max && depthSave[winWidth * y + x] < .999)
		max = depthSave[winWidth * y + x];
	}

    for(y = 0; y < winHeight; y++)
	for(x = 0; x < winWidth; x++) {
	    if(depthSave[winWidth * y + x] <= max)
		depthSave[winWidth * y + x] = 1 -  (depthSave[winWidth * y + x] - min) / (max - min);
	    else
		depthSave[winWidth * y + x] = 0;
	}

    pushOrthoView(0, 1, 0, 1, 0, 1);
    glRasterPos3f(0, 0, -.5);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glGetIntegerv(GL_DRAW_BUFFER, &previousColorBuffer);
    glDrawBuffer(whichColorBuffer);
    glDrawPixels(winWidth, winHeight, GL_LUMINANCE , GL_FLOAT, depthSave);
    glDrawBuffer(previousColorBuffer);
    glEnable(GL_DEPTH_TEST);
    popView();
}


unsigned char colors[][3] =
{
    {255, 0, 0},	/* red */
    {255, 218, 0},	/* yellow */
    {72, 255, 0},	/* yellowish green */
    {0, 255, 145},	/* bluish cyan */
    {0, 145, 255},	/* cyanish blue */
    {72, 0, 255},	/* purplish blue */
    {255, 0, 218},	/* reddish purple */
};


void copyStencilToColor(GLenum whichColorBuffer)
{
    int		x, y;
    GLint	previousColorBuffer;

    glReadPixels(0, 0, winWidth, winHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE,
        stencilSave);

    /* I'm sure this could be done much better with OpenGL */
    for(y = 0; y < winHeight; y++)
	for(x = 0; x < winWidth; x++) {
	    int stencilValue;
	    
	    stencilValue = stencilSave[winWidth * y + x];

	    colorSave[(winWidth * y + x) * 3 + 0] = colors[stencilValue % 7][0];
	    colorSave[(winWidth * y + x) * 3 + 1] = colors[stencilValue % 7][1];
	    colorSave[(winWidth * y + x) * 3 + 2] = colors[stencilValue % 7][2];
	}

    pushOrthoView(0, 1, 0, 1, 0, 1);
    glRasterPos3f(0, 0, -.5);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glGetIntegerv(GL_DRAW_BUFFER, &previousColorBuffer);
    glDrawBuffer(whichColorBuffer);
    glDrawPixels(winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, colorSave);
    glDrawBuffer(previousColorBuffer);
    glEnable(GL_DEPTH_TEST);
    popView();
}


#if 0

GLushort rrow[1280], grow[1280], brow[1280], arow[1280];


#define iopen junk
#include <gl/image.h>
#undef iopen


IMAGE *iopen(char *file, char *mode, ...);
int putrow(IMAGE *image, unsigned short *buff, int y, int z);
int iclose(IMAGE *);


void saveSGIImage(char *name, int doAlphaToo)
{
    IMAGE *img;
    FILE *fp;
    GLubyte *pixels; 
    int x, y;
    int numComp = doAlphaToo ? 4 : 3;

    pixels = malloc(winWidth * winHeight * numComp * sizeof(GLubyte));

    glReadPixels(0, 0, winWidth, winHeight, doAlphaToo ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, pixels);

    img = iopen(name, "w", RLE(1), numComp, winWidth, winHeight, numComp);

    for(y = 0; y < winHeight; y++) {
        for(x = 0; x < winWidth; x++) {
	    rrow[x] = pixels[(y * winWidth + x) * numComp + 0];
	    grow[x] = pixels[(y * winWidth + x) * numComp + 1];
	    brow[x] = pixels[(y * winWidth + x) * numComp + 2];
	    if(doAlphaToo)
		arow[x] = pixels[(y * winWidth + x) * numComp + 3];
	}
        putrow(img, rrow, y, 0);
        putrow(img, grow, y, 1);
        putrow(img, brow, y, 2);
	if(doAlphaToo)
	    putrow(img, arow, y, 3);
    }
    iclose(img);

    free(pixels);
}

#endif


struct transformation {
    float	translation[3];
    float	rotation[4];
    float	scale[3];
};


void drawXform(struct transformation *xform, int applyScale)
{
    glTranslatef(xform->translation[0], xform->translation[1], xform->translation[2]);
    glRotatef(xform->rotation[3] / M_PI * 180, xform->rotation[0], xform->rotation[1], xform->rotation[2]);
    if(applyScale)
	glScalef(xform->scale[0], xform->scale[1], xform->scale[2]);
}


enum trackballModeEnum {
    ROTATE,
    TRANSLATEXY,
    TRANSLATEZ,
    SCALEX,
    SCALEY,
    SCALEZ
} trackballMode = ROTATE;


struct transformation xform = 
{
    0, 0, 0,
    -0.65, -0.75, -0.04, 0.89,
    .7, .7, 2,
};


void axisamountToMat(float aa[], float mat[])
{
    float c, s, t;

    c = (float)cos(aa[3]);
    s = (float)sin(aa[3]);
    t = 1.0f - c;

    mat[0] = t * aa[0] * aa[0] + c;
    mat[1] = t * aa[0] * aa[1] + s * aa[2];
    mat[2] = t * aa[0] * aa[2] - s * aa[1];
    mat[3] = t * aa[0] * aa[1] - s * aa[2];
    mat[4] = t * aa[1] * aa[1] + c;
    mat[5] = t * aa[1] * aa[2] + s * aa[0];
    mat[6] = t * aa[0] * aa[2] + s * aa[1];
    mat[7] = t * aa[1] * aa[2] - s * aa[0];
    mat[8] = t * aa[2] * aa[2] + c;
}


void matToAxisamount(float mat[], float aa[])
{
    float c;
    float s;

    c = (mat[0] + mat[4] + mat[8] - 1.0f) / 2.0f;
    aa[3] = (float)acos(c);
    s = (float)sin(aa[3]);
    if(fabs(s / M_PI - (int)(s / M_PI)) < .0000001)
    {
        aa[0] = 0.0f;
        aa[1] = 1.0f;
        aa[2] = 0.0f;
    }
    else
    {
	aa[0] = (mat[5] - mat[7]) / (2.0f * s);
	aa[1] = (mat[6] - mat[2]) / (2.0f * s);
	aa[2] = (mat[1] - mat[3]) / (2.0f * s);
    }
}


void multMat(float m1[], float m2[], float r[])
{
    float t[9];
    int i;

    t[0] = m1[0] * m2[0] + m1[1] * m2[3] + m1[2] * m2[6];
    t[1] = m1[0] * m2[1] + m1[1] * m2[4] + m1[2] * m2[7];
    t[2] = m1[0] * m2[2] + m1[1] * m2[5] + m1[2] * m2[8];
    t[3] = m1[3] * m2[0] + m1[4] * m2[3] + m1[5] * m2[6];
    t[4] = m1[3] * m2[1] + m1[4] * m2[4] + m1[5] * m2[7];
    t[5] = m1[3] * m2[2] + m1[4] * m2[5] + m1[5] * m2[8];
    t[6] = m1[6] * m2[0] + m1[7] * m2[3] + m1[8] * m2[6];
    t[7] = m1[6] * m2[1] + m1[7] * m2[4] + m1[8] * m2[7];
    t[8] = m1[6] * m2[2] + m1[7] * m2[5] + m1[8] * m2[8];
    for(i = 0; i < 9; i++)
    {
        r[i] = t[i];
    }
}


void rotateTrackball(int dx, int dy, float rotation[4])
{
    float dist;
    float oldMat[9];
    float rotMat[9];
    float newRot[4];

    dist = (float)sqrt((double)(dx * dx + dy * dy));
    if(fabs(dist) < 0.99)
        return;

    newRot[0] = (float) dy / dist;
    newRot[1] = (float) dx / dist;
    newRot[2] = 0.0f;
    newRot[3] = (float)M_PI * dist / winWidth;

    axisamountToMat(rotation, oldMat);
    axisamountToMat(newRot, rotMat);
    multMat(oldMat, rotMat, oldMat);
    matToAxisamount(oldMat, rotation);

    dist = (float)sqrt(rotation[0] * rotation[0] + rotation[1] * rotation[1] +
        rotation[2] * rotation[2]);

    rotation[0] /= dist;
    rotation[1] /= dist;
    rotation[2] /= dist;
}


int stage = 6;
typedef enum {COLOR, DEPTH, STENCIL} BufferInterest;

BufferInterest bufferInterest = COLOR;


void init(void)
{
    GLfloat defaultMat[] = {.75, .25, .25, 1};

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glMatrixMode(GL_PROJECTION);
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */

    glEnable(GL_NORMALIZE);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_INCR, GL_INCR, GL_INCR);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, defaultMat);

    glLineWidth(3);
    glShadeModel(GL_FLAT);
}


void redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glPushMatrix();

    drawXform(&xform, TRUE);

    glEnable(GL_STENCIL_TEST);
    if(whichSurface < 2)
	glStencilFunc(GL_EQUAL, whichSurface, 0xff);
    else
	glStencilFunc(GL_ALWAYS, 0, 0);

    glutSolidTorus(2, 4, 20, 20);

    /* glDisable(GL_STENCIL_TEST); */
    /* glDisable(GL_LIGHTING); */
    /* glColor3f(1, 1, 1); */
    /* glutWireTorus(2, 4, 20, 20); */
    /* glEnable(GL_LIGHTING); */

    glPopMatrix();

    switch(bufferInterest) {
        case COLOR:
	    break; /* color already in back buffer */

	case STENCIL:
	    copyStencilToColor(GL_BACK);
	    break;

	case DEPTH:
	    copyDepthToColor(GL_BACK);
	    break;
    }

    if(takeSnapshot) {
        takeSnapshot = 0;
	/* saveSGIImage("snap.rgb", FALSE); */
	/* printf("Saved RGBA image in snap.rgba\n"); */
    }

    glutSwapBuffers();
}


void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    winWidth = width;
    winHeight = height;
    resizeBuffers();
    glutPostRedisplay();
}


void changeData(int data)
{
    char *s;

    bufferInterest = (BufferInterest) data;
    glutPostRedisplay();

    switch(data) {
        case COLOR:
	    s = "color";
	    break;

        case STENCIL:
	    s = "stencil";
	    break;

        case DEPTH:
	    s = "depth";
	    break;
    }
    printf("Now displaying %s data\n", s);
}


int mainMenu;


/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case '1':
        case '2':
        case '3':
	    whichSurface = key - '1';
	    glutPostRedisplay();
	    break;

        case 'r':
	    trackballMode = ROTATE;
	    break;

	case 't':
	    trackballMode = TRANSLATEXY;
	    break;

	case 'T':
	    trackballMode = TRANSLATEZ;
	    break;

	case 'x':
	    trackballMode = SCALEX;
	    break;

	case 'y':
	    trackballMode = SCALEY;
	    break;

	case 'z':
	    trackballMode = SCALEZ;
	    break;

	case 'q': case 'Q': case '\033':
	    exit(0);
	    break;

	case '+': case '=':
	    stage++;
	    glutPostRedisplay();
	    break;

	case '-': case '_':
	    stage--;
	    glutPostRedisplay();
	    break;

	case 's':
	    printf("%f %f %f %f\n", xform.rotation[0], xform.rotation[1],
	        xform.rotation[2], xform.rotation[3]);
	    glutPostRedisplay();
	    break;
    }
}


static int ox, oy;


/* ARGSUSED */
void button(int b, int state, int x, int y)
{
    ox = x;
    oy = y;
}


void motion(int x, int y)
{
    int dx, dy;

    dx = x - ox;
    dy = y - oy;

    ox = x;
    oy = y;

    switch(trackballMode) {
	case ROTATE:
	    rotateTrackball(dx, dy, xform.rotation);
	    break;

	case SCALEX:
	    xform.scale[0] += (dx + dy) / 20.0f;
	    break;

	case SCALEY:
	    xform.scale[1] += (dx + dy) / 20.0f;
	    break;

	case SCALEZ:
	    xform.scale[2] += (dx + dy) / 20.0f;
	    break;

	case TRANSLATEXY:
	    xform.translation[0] += dx / 20.0f;
	    xform.translation[1] -= dy / 20.0f;
	    break;

	case TRANSLATEZ:
	    xform.translation[2] += (dx + dy) / 20.0f;
	    break;
    }
    glutPostRedisplay();
}

/* ARGSUSED */
void mainMenuFunc(int menu)
{
    /* */
}


int main(int argc, char **argv)
{
    int bufferMenu;
    int stenSize;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth = 256, winHeight = 256);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_STENCIL|GLUT_DEPTH|GLUT_ALPHA);
    (void)glutCreateWindow("torus depth");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(motion);
    glutMouseFunc(button);
    glutReshapeFunc(reshape);

    resizeBuffers();
    glGetIntegerv(GL_STENCIL_BITS, &stenSize);
    fprintf(stderr, "(%d bits of stencil available in this visual)\n", stenSize);

    fprintf(stderr, "Hit 'h' for help message\n");

    bufferMenu = glutCreateMenu(changeData);
    glutAddMenuEntry("Color data", COLOR);
    glutAddMenuEntry("Stencil data", STENCIL);
    glutAddMenuEntry("Depth data", DEPTH);

    mainMenu = glutCreateMenu(mainMenuFunc);
    glutAddSubMenu("Visible buffer", bufferMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    init();
    glutMainLoop();

    return 0;
}
