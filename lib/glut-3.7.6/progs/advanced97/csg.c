#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TRUE	1
#define FALSE	0

/*
 * #define the following symbol if you know your glClear honors the
 * stencil mask.  The O2 workstation stencil clear doesn't appear
 * to honor the mask, so we use a workaround.  The workaround may work
 * on any other OpenGL which has this clear bug.
 */
#undef CLEAR_HONORS_STENCIL_MASK


GLUquadricObj	*quadric;


int doCSG = 0;
int whereToStop = -1;
typedef enum {CONTINUE, STOP} progressEnum;
int whichTree = 0;
int showSurfaces = 1;
int showOnlyCurrent = 0;
int stenSize;
enum {COLOR, DEPTH, STENCILVALUES, STENCILPLANES} bufferInterest = COLOR;


void drawFace(void)
{
    glBegin(GL_QUADS);
    glNormal3i(0, 0, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glEnd();
}


void drawBox(void)
{
    glPushMatrix();

    drawFace();

    glRotatef(90, 1, 0, 0);
    drawFace();

    glRotatef(90, 1, 0, 0);
    drawFace();

    glRotatef(90, 1, 0, 0);
    drawFace();

    glPopMatrix();

    glPushMatrix();

    glRotatef(90, 0, 1, 0);
    drawFace();

    glRotatef(180, 0, 1, 0);
    drawFace();

    glPopMatrix();
}


void drawCylinder(void)
{
    glPushMatrix();

    glTranslatef(0, 0, -.5);
    gluCylinder(quadric, 1, 1, 1, 30, 5);

    glRotatef(180, 0, 1, 0);
    gluDisk(quadric, 0, 1, 30, 2);

    glPopMatrix();

    glPushMatrix();

    glTranslatef(0, 0, .5);
    gluDisk(quadric, 0, 1, 30, 2);

    glPopMatrix();
}


void drawCone(void)
{
    glPushMatrix();

    glTranslatef(0, 0, -.5);
    gluCylinder(quadric, 1, 0, 1, 30, 5);

    glRotatef(180, 0, 1, 0);
    gluDisk(quadric, 0, 1, 30, 2);

    glPopMatrix();
}


void drawSphere(void)
{
    gluSphere(quadric, 1, 30, 20);
}


struct transformation {
    float	translation[3];
    float	rotation[4];
    float	scale[3];
};


struct primitive {
    float		color[4];
    void		(*draw)(void);
    struct transformation	xform;
};


struct transformation globalXform = {
    0, 0, 0,
    1, 0, 0, 0,
    1, 1, 1
};


struct primitive prims[20] = {
    {
        {.5, .5, 1},
	drawBox,
	{
	    -3, 0, 0,
	    1, 0, 0, 0,
	    1, 1, 1
	}
    },
    {
        {1, .5, .5},
	drawCylinder,
	{
	    -1, 0, 0,
	    1, 0, 0, 0,
	    1, 1, 1
	}
    },
    {
        {.5, 1, .5},
	drawCone,
	{
	    1, 0, 0,
	    1, 0, 0, 0,
	    1, 1, 1
	}
    },
    {
        {1, .5, 1},
	drawSphere,
	{
	    3, 0, 0,
	    1, 0, 0, 0,
	    1, 1, 1
	}
    }
};


int numPrims = 4;
int curPrim = 0;


void drawXform(struct transformation *xform, int applyScale)
{
    glTranslatef(xform->translation[0], xform->translation[1], xform->translation[2]);
    glRotatef(xform->rotation[3] / M_PI * 180, xform->rotation[0], xform->rotation[1], xform->rotation[2]);
    if(applyScale)
	glScalef(xform->scale[0], xform->scale[1], xform->scale[2]);
}


int magicTranspHack = 0;


void drawPrim(int i)
{
    struct primitive *p = &prims[i];
    struct transformation *xform = &p->xform;

    if(magicTranspHack)
    {
        float c[4];
	c[0] = p->color[0];
	c[1] = p->color[1];
	c[2] = p->color[2];
	c[3] = .25;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, c);
    }
    else
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, p->color);
    glPushMatrix();
    drawXform(xform, TRUE);
    p->draw();
    glPopMatrix();
}


struct product {
    int targetPrim;
    int frontFace;
    int whichSurface;
    int numTrimPrims;
    int *trimmingPrims;
    int *isComplemented;
};


int prodAtrimmingPrims1[] = {1};
int prodAisComplemented1[] = {1};
int prodBtrimmingPrims1[] = {0};
int prodBisComplemented1[] = {0};


int prodAtrimmingPrims2[] = {1};
int prodAisComplemented2[] = {0};
int prodBtrimmingPrims2[] = {0};
int prodBisComplemented2[] = {1};


int prodAtrimmingPrims3[] = {1};
int prodAisComplemented3[] = {0};
int prodBtrimmingPrims3[] = {0};
int prodBisComplemented3[] = {0};


int prodAtrimmingPrims4[] = {1, 2, 3};
int prodAisComplemented4[] = {0, 1, 1};
int prodBtrimmingPrims4[] = {1, 0, 2};
int prodBisComplemented4[] = {0, 0, 1};
int prodCtrimmingPrims4[] = {1, 0, 3};
int prodCisComplemented4[] = {0, 0, 1};
int prodDtrimmingPrims4[] = {0, 2, 3};
int prodDisComplemented4[] = {0, 1, 1};


struct product products[][4] = {
    {	/* A - B */
	{
	    0, 1, 0,
	    1,
	    prodAtrimmingPrims1,
	    prodAisComplemented1,
	},
	{
	    1, 0, 0,
	    1,
	    prodBtrimmingPrims1,
	    prodBisComplemented1,
	},
    },

    {    /* B - A */
	{
	    0, 0, 0,
	    1,
	    prodAtrimmingPrims2,
	    prodAisComplemented2,
	},
	{
	    1, 1, 0,
	    1,
	    prodBtrimmingPrims2,
	    prodBisComplemented2,
	},
    },

    {    /* A and B */
	{
	    0, 1, 0,
	    1,
	    prodAtrimmingPrims3,
	    prodAisComplemented3,
	},
	{
	    1, 1, 0,
	    1,
	    prodBtrimmingPrims3,
	    prodBisComplemented3,
	},
    },

    { /* A and B - D - C */
	{
	    0, 1, 0,
	    3,
	    prodAtrimmingPrims4,
	    prodAisComplemented4,
	},
	{
	    3, 0, 0,
	    3,
	    prodBtrimmingPrims4,
	    prodBisComplemented4,
	},
	{
	    2, 0, 0,
	    3,
	    prodCtrimmingPrims4,
	    prodCisComplemented4,
	},
	{
	    1, 1, 0,
	    3,
	    prodDtrimmingPrims4,
	    prodDisComplemented4,
	},
    },
};


int numProducts[4] = {2, 2, 2, 4};


int 		winWidth, winHeight;
GLfloat 	*depthSave = NULL;
GLubyte 	*stencilSave = NULL;
GLubyte 	*colorSave = NULL;


void resizeBuffers(void)
{
    if(colorSave != NULL)
        free(colorSave);
    colorSave = malloc(winWidth * winHeight * 4 * sizeof(GLubyte));
    if(depthSave != NULL)
        free(depthSave);
    depthSave = malloc(winWidth * winHeight * sizeof(GLfloat));
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


unsigned char stencilValueToColorMap[][3] =
{
    {255, 0, 0},	/* red */
    {255, 218, 0},	/* yellow */
    {72, 255, 0},	/* yellowish green */
    {0, 255, 145},	/* bluish cyan */
    {0, 145, 255},	/* cyanish blue */
    {72, 0, 255},	/* purplish blue */
    {255, 0, 218},	/* reddish purple */
};


void copyStencilValuesToColor(GLenum whichColorBuffer)
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

	    colorSave[(winWidth * y + x) * 3 + 0] =
	        stencilValueToColorMap[stencilValue % 7][0];
	    colorSave[(winWidth * y + x) * 3 + 1] =
	        stencilValueToColorMap[stencilValue % 7][1];
	    colorSave[(winWidth * y + x) * 3 + 2] =
	        stencilValueToColorMap[stencilValue % 7][2];
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


/*
 * XXX This function should make colors that identify individual bits in
 * the stencil buffer, but it seems like too hard a problem to solve
 * in five minutes.  Fix later.
 */
void copyStencilPlanesToColor(GLenum whichColorBuffer)
{
    int		x, y;
    GLint	previousColorBuffer;

    glReadPixels(0, 0, winWidth, winHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE,
        stencilSave);

    /* I'm sure this could be done much better with OpenGL */
    for(y = 0; y < winHeight; y++)
	for(x = 0; x < winWidth; x++) {
	    /* Have to think of something clever to do here */
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


void copyInterest(void)
{
    switch(bufferInterest) {
        case COLOR:
	    break; /* Well, that's already there. */

	case STENCILVALUES:
	    copyStencilValuesToColor(GL_BACK);
	    break;

	case STENCILPLANES:
	    copyStencilPlanesToColor(GL_BACK);
	    break;

	case DEPTH:
	    copyDepthToColor(GL_BACK);
	    break;
    }
}


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


struct transformation *curXform;


void init(void)
{
    glMatrixMode(GL_PROJECTION);
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0, 0, 7, 0, 0, 0, 0, 1, 0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_NORMALIZE);

    quadric = gluNewQuadric();

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    curXform = &prims[0].xform;
}


void setupLight(void)
{
    static GLfloat lightpos[] = {0, 1, 0, 0};

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
}


void redrawNoCSG(void)
{
    int i;

    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    drawXform(&globalXform, FALSE);

    for(i = 0; i < numPrims; i++)
    {
        if(i == curPrim || !showOnlyCurrent)
	    drawPrim(i);
    }

    glPopMatrix();

    copyInterest();

    glutSwapBuffers();
}


int whereSoFar;


GLfloat *depthResults = NULL;


void saveDepth(void)
{
    depthResults = realloc(depthResults, winWidth * winHeight *
        sizeof(GLfloat));
    glReadPixels(0, 0, winWidth, winHeight, GL_DEPTH_COMPONENT, GL_FLOAT,
	depthResults);
}


void restoreDepth(void)
{
    glStencilMask(0);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_STENCIL_TEST);

    pushOrthoView(0, 1, 0, 1, 0, 1);
    glRasterPos3f(0, 0, -.5);

    glDrawPixels(winWidth, winHeight, GL_DEPTH_COMPONENT, GL_FLOAT,
        depthResults);

    popView();
}


#define COPY_AND_RETURN_IF_DONE(s) \
    { \
	if(whereSoFar++ == whereToStop) { \
	    printf("%s\n", s); \
	    return(STOP);  \
	} \
    }


#define COPY_AND_GOTO_IF_DONE(s) \
    { \
	if(whereSoFar++ == whereToStop) { \
	    printf("%s\n", s); \
	    goto doneWithFrame;  \
	} \
    }


int sCountMask = 0x01;	/* 1-convexity maximum */
int sCountShift = 0;
int sPMask = 1;
int sPShift = 0;


void drawFarRect(void)
{
    pushOrthoView(0, 1, 0, 1, 0, 1);

    /* Can I just draw & let be clipped? */
    glBegin(GL_QUADS);
    glVertex3f(0, 0, -1);
    glVertex3f(1, 0, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(0, 1, -1);
    glEnd();

    popView();
}


progressEnum renderPrimDepths(int targetPrim, int frontFace, int whichSurface)
{
    glClear(GL_DEPTH_BUFFER_BIT);


    glDepthFunc(GL_ALWAYS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilMask(sCountMask);

#ifndef CLEAR_HONORS_STENCIL_MASK /* see comment at beginning of source */

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    drawFarRect();

    COPY_AND_RETURN_IF_DONE("After clearing p bit in stencil");

#else

    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);

#endif

    glEnable(GL_CULL_FACE);
    if(frontFace)
        glCullFace(GL_BACK);
    else
        glCullFace(GL_FRONT);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glStencilFunc(GL_EQUAL, whichSurface, sCountMask);
    glStencilOp(GL_INCR, GL_INCR, GL_INCR);

    drawPrim(targetPrim);

    return(CONTINUE);
}


progressEnum trimWithPrimitive(int trimPrim, int isComplemented)
{
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilMask(sPMask);
    glDisable(GL_CULL_FACE);

#ifndef CLEAR_HONORS_STENCIL_MASK /* see comment at beginning of source */

    glDisable(GL_DEPTH_TEST);

    glStencilFunc(GL_ALWAYS, isComplemented ? (1 << sPShift) : 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    drawFarRect();

#else

    glClearStencil(isComplemented ? (1 << sPShift) : 0);
    glClear(GL_STENCIL_BUFFER_BIT);

#endif

    glEnable(GL_DEPTH_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
    drawPrim(trimPrim);

    COPY_AND_RETURN_IF_DONE("After setting stencil to mark depths "
        "inside trimming primitive");

    /* stencil == 0 where pixels were not inside */
    /* so now set Z to far where stencil == 0, everywhere pixels trimmed */

    glStencilFunc(GL_EQUAL, 0, sPMask);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDepthMask(1);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_LIGHTING);
    drawFarRect();
    glEnable(GL_LIGHTING);

    COPY_AND_RETURN_IF_DONE("After clearing depths where target outside "
        "trimming primitive");

    return(CONTINUE);
}


progressEnum markProductPixels(int product, int accumBit)
{
    int i;

    struct product *p;

    p = &products[whichTree][product];

    if(renderPrimDepths(p->targetPrim, p->frontFace, p->whichSurface) == STOP)
        return(STOP);

    COPY_AND_RETURN_IF_DONE("After rendering target depths");

    for(i = 0; i < p->numTrimPrims; i++)
    {
        if(trimWithPrimitive(p->trimmingPrims[i], p->isComplemented[i]) == STOP)
	    return(STOP);
    }

    COPY_AND_RETURN_IF_DONE("After target has been trimmed by all "
        "trimming primitives");

    /* set accumulator stencil bit for this primitive everywhere depth != far */
    glStencilFunc(GL_ALWAYS, 1 << accumBit, 0);
    glStencilOp(GL_KEEP, GL_ZERO, GL_REPLACE);
    glStencilMask(1 << accumBit);
    glDepthMask(0);
    glDepthFunc(GL_GREATER);
    glDisable(GL_LIGHTING);
    drawFarRect();
    glEnable(GL_LIGHTING);

    COPY_AND_RETURN_IF_DONE("After setting accumulator where depths != far");

    return(CONTINUE);
}


progressEnum drawProduct(int product, int accumBit)
{
    struct product *p;
    p = &products[whichTree][product];

    glEnable(GL_CULL_FACE);
    if(p->frontFace)
        glCullFace(GL_BACK);
    else
        glCullFace(GL_FRONT);

    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1 << accumBit, 1 << accumBit);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    drawPrim(p->targetPrim);

    COPY_AND_RETURN_IF_DONE("After drawing target color and depth");

    return(CONTINUE);
}


void redrawCSG(void)
{
    int i;
    int accumBit;
    int firstProduct;
    int lastProduct;

    whereSoFar = 0;
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 
    glClear(GL_COLOR_BUFFER_BIT);

    /* Only have to do this if you're going to look at the stencil buffer */
    glClearStencil(0);
    glStencilMask((1 << stenSize) - 1);
    glClear(GL_STENCIL_BUFFER_BIT);

    glPushMatrix();

    drawXform(&globalXform, FALSE);

    firstProduct = 0;

    while(firstProduct != numProducts[whichTree]) {
    
 	/*
	 * set lastProduct so that accum bits for first to last fit in
	 * stencil buffer minus bits needed for surface counting bits
	 */
	lastProduct = firstProduct + (stenSize - 1) - 1;
	if(lastProduct >= numProducts[whichTree])
	    lastProduct = numProducts[whichTree] - 1;

	if(firstProduct > 0)	/* know depth is clear before 1st group */
	    saveDepth();	

	accumBit = 1;		/* first available after counting bits */

	for(i = firstProduct; i <= lastProduct; i++)
	    if(markProductPixels(i, accumBit++) == STOP)
		goto doneWithFrame;

	COPY_AND_GOTO_IF_DONE("After marking \"inside\" target accumulators");

	if(firstProduct > 0)	/* know depth was clear before first group */
	    restoreDepth();
	else {
	    glDepthMask(GL_TRUE);
	    glClear(GL_DEPTH_BUFFER_BIT);
	}

	accumBit = 1;		/* first available after counting bits */
	
	for(i = firstProduct; i <= lastProduct; i++)
	    if(drawProduct(i, accumBit++) == STOP)
		goto doneWithFrame;

	COPY_AND_GOTO_IF_DONE("After drawing all target colors and depths");

	firstProduct = lastProduct + 1;
    }

    if(showSurfaces) {
	glDisable(GL_STENCIL_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	magicTranspHack = 1;
	for(i = 0; i < numProducts[whichTree]; i++)
	{
	    struct product *p;
	    p = &products[whichTree][i];

	    if(i == curPrim || !showOnlyCurrent)
		drawPrim(p->targetPrim);
	}
	magicTranspHack = 0;
	glDisable(GL_BLEND);
    }

doneWithFrame:

    glPopMatrix();

    copyInterest();
    glutSwapBuffers();
}


enum trackballModeEnum {
    ROTATE,
    TRANSLATEXY,
    TRANSLATEZ,
    SCALEX,
    SCALEY,
    SCALEZ
} trackballMode = ROTATE;


/* ARGSUSED1 */
void special(int thing, int x, int y)
{
     switch(thing) {
         case GLUT_KEY_F1:
	     prims[curPrim].draw = drawBox;
	     glutPostRedisplay();
	     break;

         case GLUT_KEY_F2:
	     prims[curPrim].draw = drawCylinder;
	     glutPostRedisplay();
	     break;

         case GLUT_KEY_F3:
	     prims[curPrim].draw = drawCone;
	     glutPostRedisplay();
	     break;

         case GLUT_KEY_F4:
	     prims[curPrim].draw = drawSphere;
	     glutPostRedisplay();
	     break;
     }
}

/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case '1':
        case '2':
        case '3':
        case '4':
	    whichTree = key - '1';
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

	case '<': case ',':
	    if(curXform != &globalXform)
	    {
		curPrim = (curPrim - 1 + numPrims) % numPrims;
		curXform = &prims[curPrim].xform;
		printf("Manipulating transformation for object %d\n", curPrim);
	    }
	    else
	    {
	        printf("Have to toggle out of global mode first\n");
	    }
	    glutPostRedisplay();
	    break;

	case '>': case '.':
	    if(curXform != &globalXform)
	    {
		curPrim = (curPrim + 1) % numPrims;
		curXform = &prims[curPrim].xform;
		printf("Manipulating transformation for object %d\n", curPrim);
	    }
	    else
	    {
	        printf("Have to toggle out of global mode first\n");
	    }
	    break;

	case 'g':
	    if(curXform == &globalXform){
		curXform = &prims[curPrim].xform;
		printf("Manipulating transformation for object %d\n", curPrim);
	    }else{
		curXform = &globalXform;
		printf("Manipulating global transformation\n");
	    }
	    break;

	case '+': case '=':
	    whereToStop++;
	    glutPostRedisplay();
	    break;

	case '-': case '_':
	    whereToStop--;
	    glutPostRedisplay();
	    break;

	case 's':
	    glutPostRedisplay();
	    showSurfaces = !showSurfaces;
	    break;

	case 'c':
	    doCSG = ! doCSG;
	    if(doCSG)
		glutDisplayFunc(redrawCSG);
	    else
		glutDisplayFunc(redrawNoCSG);
	    glutPostRedisplay();
	    break;

	case ' ':
	    showOnlyCurrent = !showOnlyCurrent;
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
	    rotateTrackball(dx, dy, curXform->rotation);
	    break;

	case SCALEX:
	    curXform->scale[0] += (dx + dy) / 40.0f;
	    if(curXform->scale[0] < 1/40.0)
	        curXform->scale[0] = 1/40.0;
	    break;

	case SCALEY:
	    curXform->scale[1] += (dx + dy) / 40.0f;
	    if(curXform->scale[1] < 1/40.0)
	        curXform->scale[1] = 1/40.0;
	    break;

	case SCALEZ:
	    curXform->scale[2] += (dx + dy) / 40.0f;
	    if(curXform->scale[2] < 1/40.0)
	        curXform->scale[2] = 1/40.0;
	    break;

	case TRANSLATEXY:
	    curXform->translation[0] += dx / 40.0f;
	    curXform->translation[1] -= dy / 40.0f;
	    break;

	case TRANSLATEZ:
	    curXform->translation[2] += (dx + dy) / 40.0f;
	    break;
    }
    glutPostRedisplay();
}


void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    winWidth = width;
    winHeight = height;
    resizeBuffers();
    glutPostRedisplay();
}


int mainMenu;


void mainMenuFunc(int choice)
{
    switch(choice)
    {
        case 1:
	    doCSG = !doCSG;
	    if(doCSG) {
	    	printf("CSG expression on\n");
		glutDisplayFunc(redrawCSG);
		glutChangeToMenuEntry(1, "Turn off CSG", 0);
	    }
	    else {
	    	printf("CSG expression off\n");
		glutDisplayFunc(redrawNoCSG);
		glutChangeToMenuEntry(1, "Turn on CSG", 0);
	    }
		glutPostRedisplay();
	    break;
        case 2:
	    showSurfaces = !showSurfaces;
	    if(showSurfaces) {
	    	printf("Transparent surfaces on\n");
		glutChangeToMenuEntry(2, "(s) Don't show objects transparent in CSG", 2);
	    }
	    else {
	    	printf("Transparent surfaces off\n");
		glutChangeToMenuEntry(2, "(s) Show objects transparent in CSG", 2);
	    }
		glutPostRedisplay();
	    break;
    }
}


void interestBufferFunc(int data)
{
    char *s;

    bufferInterest = data;

    switch(data) {
        case COLOR:
	    s = "color";
	    break;

        case STENCILVALUES:
	    s = "stencil values";
	    break;

        case STENCILPLANES:
	    s = "individual stencil plane converage";
	    break;

        case DEPTH:
	    s = "depth";
	    break;
    }
    printf("Now displaying %s data\n", s);
    glutPostRedisplay();
}


void csgMenuFunc(int data)
{
    whichTree = data;
    glutPostRedisplay();
}


void whichObjectFunc(int data)
{
    switch(data)
    {
        case 0:
        case 1:
        case 2:
        case 3:
	    curPrim = data;
	    curXform = &prims[curPrim].xform;
	    printf("Editing object %d\n", data);
	    break;

	case 4:
	    curXform = &globalXform;
	    printf("Editing global transformation %d\n", data);
	    break;
    }
}


void morphMenuFunc(int data)
{
    switch(data) {
        case 0:
	    prims[curPrim].draw = drawBox;
	    glutPostRedisplay();
	    break;

        case 1:
	    prims[curPrim].draw = drawCylinder;
	    glutPostRedisplay();
	    break;

        case 2:
	    prims[curPrim].draw = drawCone;
	    glutPostRedisplay();
	    break;

        case 3:
	    prims[curPrim].draw = drawSphere;
	    glutPostRedisplay();
	    break;
     }
}


void helpFakeFunc(int d)
{
}


int main(int argc, char **argv)
{
    int bufferMenu;
    int csgMenu;
    int whichObjectMenu;
    int morphMenu;
    int helpFakeMenu;

    glutInitWindowSize(512,512);
    glutInit(&argc, argv);
    glutInitDisplayString("samples stencil>=3 rgb double depth");
    /* glutInitDisplayMode(GLUT_DOUBLE|GLUT_STENCIL|GLUT_DEPTH|GLUT_RGBA); */
    (void)glutCreateWindow("csg using stencil");
    glutDisplayFunc(redrawNoCSG);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMotionFunc(motion);
    glutMouseFunc(button);
    glutReshapeFunc(reshape);

    glGetIntegerv(GL_STENCIL_BITS, &stenSize);
    printf("%d bits of stencil available in this visual\n", stenSize);

    printf("Hit 'S' to turn stencil on/off\n");

    bufferMenu = glutCreateMenu(interestBufferFunc);
    glutAddMenuEntry("Color data", COLOR);
    glutAddMenuEntry("Stencil values", STENCILVALUES);
    glutAddMenuEntry("Coverage in each stencil plane", STENCILPLANES);
    glutAddMenuEntry("Depth data", DEPTH);

    csgMenu = glutCreateMenu(csgMenuFunc);
    glutAddMenuEntry("(1) obj 1 MINUS obj 2", 0);
    glutAddMenuEntry("(2) obj 2 MINUS obj 1", 1);
    glutAddMenuEntry("(3) obj 1 AND obj 2", 2);
    glutAddMenuEntry("(4) (obj 1 AND obj 2) MINUS (obj 3 or obj 4)", 3);

    whichObjectMenu = glutCreateMenu(whichObjectFunc);
    glutAddMenuEntry("Object 1 (blue)", 0);
    glutAddMenuEntry("Object 2 (peach)", 1);
    glutAddMenuEntry("Object 3 (green)", 2);
    glutAddMenuEntry("Object 4 (purple)", 3);
    glutAddMenuEntry("(g) Toggle between global and local", 4);

    morphMenu = glutCreateMenu(morphMenuFunc);
    glutAddMenuEntry("(F1) Box", 0);
    glutAddMenuEntry("(F2) Cylinder", 1);
    glutAddMenuEntry("(F3) Cone", 2);
    glutAddMenuEntry("(F4) Sphere", 3);

    helpFakeMenu = glutCreateMenu(helpFakeFunc);
    glutAddMenuEntry("Click the left mouse button and drag to manipulate\n", 0);
    glutAddMenuEntry("Push 'r' to rotate like a trackball\n", 0);
    glutAddMenuEntry("Push 't' to translate in the X-Y plane\n", 0);
    glutAddMenuEntry("Push 'T' to translate in the Z axis\n", 0);
    glutAddMenuEntry("Push 'X' to scale in X\n", 0);
    glutAddMenuEntry("Push 'X' to scale in Y\n", 0);
    glutAddMenuEntry("Push 'Z' to scale in Z\n", 0);

    mainMenu = glutCreateMenu(mainMenuFunc);
    glutAddMenuEntry("(c) Turn on CSG", 1);
    glutAddMenuEntry("(s) Show objects transparent in CSG", 2);
    glutAddSubMenu("Buffer of interest", bufferMenu);
    glutAddSubMenu("Which CSG expression", csgMenu);
    glutAddSubMenu("Object for manipulation", whichObjectMenu);
    glutAddSubMenu("Turn current object into...", morphMenu);
    glutAddSubMenu("Help with moving and rotating:", helpFakeMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    init();
    glutMainLoop();

    return 0;
}
