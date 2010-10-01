#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GLUquadricObj	*quadric;
GLfloat 	black[] = {0, 0, 0, 1};
GLfloat 	white[] = {1, 1, 1, 1};


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


int useStencil = 0;
int stage = 6;
typedef enum {COLOR, DEPTH, STENCIL} DataChoice;
DataChoice dataChoice = COLOR;

void init(void)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    quadric = gluNewQuadric();

    glMatrixMode(GL_PROJECTION);
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(-4, 10, 6, 0, 0, 0, 0, 1, 0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_NORMALIZE);

    /*
     * only need this to clear stencil and only need to clear stencil
     * when you're looking at it; the algorithm works without it.
     */
    glClearStencil(5);
}


void setupLight(void)
{
    static GLfloat 	lightpos[] = {0, 1, 0, 0};

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
}


void setupNormalDrawingState(void)
{
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
}

void setupBasePolygonState(int maxDecal)
{
    glEnable(GL_DEPTH_TEST);
    if(useStencil) {
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, maxDecal + 1, 0xff);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_ZERO);
    }
}


void setupDecalState(int decalNum)
{
    if(useStencil) {
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_GREATER, decalNum, 0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }
}


void drawAirplane(void)
{
    GLfloat airplaneColor[4] = {.75, .75, .75, 1};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, airplaneColor);

    glDisable(GL_CULL_FACE);

    glPushMatrix();
    glTranslatef(0, 0, -2.5);

    gluCylinder(quadric, .5, .5, 5, 10, 10);

    glPushMatrix();
    glTranslatef(0, 0, 5);
    gluCylinder(quadric, .5, 0, 1, 10, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 0, 3);
    glScalef(3, .1, 1);
    gluSphere(quadric, 1, 10, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 0, .5);
    glScalef(2, .1, .5);
    gluSphere(quadric, 1, 10, 10);
    glPopMatrix();

    glEnable(GL_CULL_FACE);

    glBegin(GL_TRIANGLES);
    glNormal3f(1, 0, 0);
    glVertex3f(0, 1.5, 0);
    glVertex3f(0, .5, 1);
    glVertex3f(0, .5, 0);
    glNormal3f(-1, 0, 0);
    glVertex3f(0, 1.5, 0);
    glVertex3f(0, .5, 0);
    glVertex3f(0, .5, 1);
    glEnd();

    glDisable(GL_CULL_FACE);

    glRotatef(180, 0, 1, 0);
    gluDisk(quadric, 0, .5, 10, 10);

    glPopMatrix();

    glEnable(GL_CULL_FACE);
}


void drawGround(void)
{
    GLfloat groundColor[4] = {.647, .165, .165, 1};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, groundColor);

    glBegin(GL_QUADS);
    glNormal3i(0, 1, 0);
    glVertex3f(10, 0, 10);
    glVertex3f(10, 0, -10);
    glVertex3f(-10, 0, -10);
    glVertex3f(-10, 0, 10);
    glEnd();
}


void drawAsphalt(void)
{
    GLfloat asphaltColor[4] = {.25, .25, .25, 1};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, asphaltColor);

    glBegin(GL_QUADS);
    glNormal3i(0, 1, 0);
    glVertex3f(5, 0, 9.5);
    glVertex3f(5, 0, -9.5);
    glVertex3f(-5, 0, -9.5);
    glVertex3f(-5, 0, 9.5);
    glEnd();
}


int	numStripes = 5;
float	stripeGap = .66;


void drawStripes(void)
{
    GLfloat	stripeColor[4] = {1, 1, 0, 1};
    int		i;
    float	stripeLength;

    stripeLength = (16 - stripeGap * (numStripes - 1)) / numStripes;

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, stripeColor);

    glBegin(GL_QUADS);

    glNormal3i(0, 1, 0);

    glVertex3f(4.5, 0, 8.5);
    glVertex3f(4.5, 0, -8.5);
    glVertex3f(3.5, 0, -8.5);
    glVertex3f(3.5, 0, 8.5);

    glVertex3f(-3.5, 0, 8.5);
    glVertex3f(-3.5, 0, -8.5);
    glVertex3f(-4.5, 0, -8.5);
    glVertex3f(-4.5, 0, 8.5);

    for(i = 0; i < numStripes; i++) {
        glVertex3f(.5, 0, 8 - i * (stripeLength + stripeGap));
        glVertex3f(.5, 0, 8 - i * (stripeLength + stripeGap) - stripeLength);
        glVertex3f(-.5, 0, 8 - i * (stripeLength + stripeGap) - stripeLength);
        glVertex3f(-.5, 0, 8 - i * (stripeLength + stripeGap));
    }
    glEnd();
}


void redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    /* Only need this if you care to look at the stencil buffer */
    if(dataChoice == STENCIL)
	glClear(GL_STENCIL_BUFFER_BIT);

    glPushMatrix();
    glScalef(.5, .5, .5);

    if(stage == 1)
       goto doneWithFrame;

    setupLight();

    setupNormalDrawingState();
    glPushMatrix();
    glTranslatef(0, 1, 4);
    glRotatef(135, 0, 1, 0);
    drawAirplane();
    glPopMatrix();

    if(stage == 2)
       goto doneWithFrame;

    setupBasePolygonState(3);	/* 2 decals */
    drawGround();

    if(stage == 3)
       goto doneWithFrame;

    setupDecalState(1);		/* decal # 1 = the runway asphalt */
    drawAsphalt();

    if(stage == 4)
       goto doneWithFrame;

    setupDecalState(2);		/* decal # 2 = yellow paint on the runway */
    drawStripes();

    if(stage == 5)
       goto doneWithFrame;

    setupDecalState(3);		/* decal # 3 = the plane's shadow */
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glPushMatrix();
    glColor4f(0, 0, 0, .5);
    glTranslatef(0, 0, 4);
    glRotatef(135, 0, 1, 0);
    glScalef(1, 0, 1);
    drawAirplane();
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

doneWithFrame:

    setupNormalDrawingState();

    glPopMatrix();

    switch(dataChoice) {
        case COLOR:
	    break; /* color already in back buffer */

	case STENCIL:
	    copyStencilToColor(GL_BACK);
	    break;

	case DEPTH:
	    copyDepthToColor(GL_BACK);
	    break;
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


static int ox, oy;
static int mode;


/* ARGSUSED */
void button(int b, int state, int x, int y)
{
    ox = x;
    oy = y;
    mode = b;
}


void motion(int x, int y)
{
    static float ang = 0;
    static float height = 10;
    float eyex, eyez;

    if(mode == GLUT_LEFT_BUTTON)
    {
        ang += (x - ox) / 512.0 * M_PI;
        height += (y - oy) / 512.0 * 10;
	eyex = cos(ang) * 7;
	eyez = sin(ang) * 7;
	glLoadIdentity();
	gluLookAt(eyex, height, eyez, 0, 0, 0, 0, 1, 0);
        glutPostRedisplay();
	ox = x;
	oy = y;
    }
}


void changeData(int data)
{
    char *s;

    dataChoice = data;
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


void changeStage(int data)
{
    char *s;

    stage = data;
    glutPostRedisplay();
    switch(data) {
        case 1:
	    s = "clearing";
	    break;

        case 2:
	    s = "drawing airplane";
	    break;

        case 3:
	    s = "drawing ground";
	    break;

        case 4:
	    s = "drawing asphalt";
	    break;

        case 5:
	    s = "drawing paint";
	    break;

        case 6:
	    s = "drawing shadow";
	    break;
    }
    printf("Now displaying frame after %s\n", s);
}


int mainMenu;

void stencilDecalEnable(int enabled)
{
    glutSetMenu(mainMenu);
    if(enabled){
	glutChangeToMenuEntry(1, "Turn off stencil decal", 0);
	printf("Stencil decaling turned on\n");
	glutPostRedisplay();
    } else {
	glutChangeToMenuEntry(1, "Turn on stencil decal", 1);
	printf("Stencil decaling turned off\n");
	glutPostRedisplay();
    }
    useStencil = enabled;
}


/*
 * Basically shortcuts for menu items
 */
/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
    switch(key) {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
	    changeStage(key - '0');
	    break;

	case 's': case 'S':
	    stencilDecalEnable(! useStencil);
	    break;

	case 'q': case 'Q': case '\033':
	    exit(0);
	    break;

	default:
	    fprintf(stderr, "Push right mouse button for menu\n");
	    break;
    }
}


int main(int argc, char **argv)
{
    int bufferMenu;
    int finishMenu;
    int stenSize;

    glutInitWindowSize(winWidth = 512, winHeight = 512);
    glutInit(&argc, argv);
    glutInitDisplayString("samples stencil>=3 rgb double depth");
    /* glutInitDisplayMode(GLUT_DOUBLE|GLUT_STENCIL|GLUT_DEPTH|GLUT_ALPHA); */
    (void)glutCreateWindow("decaling using stencil");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(motion);
    glutMouseFunc(button);
    glutReshapeFunc(reshape);

    resizeBuffers();
    glGetIntegerv(GL_STENCIL_BITS, &stenSize);
    fprintf(stderr, "(%d bits of stencil available in this visual)\n", stenSize);

    fprintf(stderr, "Hit 'h' for help message\n");

    finishMenu = glutCreateMenu(changeStage);
    glutAddMenuEntry("Clearing screen", 1);
    glutAddMenuEntry("Drawing airplane", 2);
    glutAddMenuEntry("Drawing ground ", 3);
    glutAddMenuEntry("Drawing asphalt", 4);
    glutAddMenuEntry("Drawing paint", 5);
    glutAddMenuEntry("Drawing shadow", 6);

    bufferMenu = glutCreateMenu(changeData);
    glutAddMenuEntry("Color data", COLOR);
    glutAddMenuEntry("Stencil data", STENCIL);
    glutAddMenuEntry("Depth data", DEPTH);

    mainMenu = glutCreateMenu(stencilDecalEnable);
    glutAddMenuEntry("Turn on stencil decal", 1);
    glutAddSubMenu("Visible buffer", bufferMenu);
    glutAddSubMenu("Finish frame after...", finishMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    init();
    glutMainLoop();

    return 0;
}
