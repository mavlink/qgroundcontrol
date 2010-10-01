#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "texture.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
/* Most math.h's do not define float versions of the math functions. */
#define sqrtf(x) (float)sqrt((x))
#define cosf(x) (float)cos((x))
#define sinf(x) (float)sin((x))
#endif

#ifdef GL_EXT_blend_subtract
#if defined(_WIN32) && !defined(MESA)
#include <windows.h>
PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT = NULL;
#endif
static int hasBlendSubtract = 0;
#else
static const int hasBlendSubtract = 0;
#endif

int winWidth = 512;
int winHeight = 512;

#ifndef FALSE
enum {FALSE, TRUE};
#endif
enum {S, T}; /* make array indexing more intuitive */
enum {X, Y, Z, W};
enum {R, G, B, A};
enum {DEFAULT_TEX, ACCUM_TEX, ADD_TEX, SUB_TEX}; /* texture names */
enum {LIGHT_XY, LIGHT_Z, PGON}; /* what should move */

int dblbuf = TRUE;
int accum = FALSE;
int color = FALSE;
int wire = FALSE;
int textureOnly = FALSE;
int lightOnly = FALSE;
int bindtex = FALSE;
int embossed = FALSE;
int steps_xz = 20, steps_y = 20;

int texture_width;
int texture_height;

/* is bumpmap shifting on? */
int bumpEnabled = FALSE;

int move = LIGHT_XY;

/* current tangent vector */
GLfloat curTangent[3];

/* current texture coordinate */
GLfloat curTex[2];


/* current normal */
GLfloat curNormal[3];

/* current light position */
GLfloat curLight[3];
GLfloat lightpos[4] = {100.f, 100.f, 100.f, 1.f};
GLfloat angles[2]; /* x and y angle */

unsigned *bumptex; /* pointer to bumpmap texture */
GLfloat bumpscale = .39f; /* scale down bumpmap texture (a smidgen under .4) */

/* TEST PROGRAM */

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture glBindTextureEXT
#endif

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

void
bumpEnable(void)
{
    bumpEnabled = TRUE;
}

void
bumpDisable(void)
{
    bumpEnabled = FALSE;
}

void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
}



void
mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* move the light */
	    move = LIGHT_XY;
	    lightpos[X] = (x - winWidth/2) * 300.f/winWidth;
	    lightpos[Y] = (winHeight/2 - y) * 300.f/winHeight;
	    glutPostRedisplay();
	    break;
	case GLUT_MIDDLE_BUTTON:
	    move = PGON;
	    angles[X] = (x - winWidth/2) * 180.f/winWidth;
	    angles[Y] = (y - winHeight/2) * 180.f/winHeight;
	    glutPostRedisplay();
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    move = LIGHT_Z;
	    lightpos[Z] = (winHeight/2 - y) * 300.f/winWidth;
	    glutPostRedisplay();
	    break;
	}
}

void
motion(int x, int y)
{
    switch(move)
    {
    case LIGHT_XY:
	lightpos[X] = (x - winWidth/2) * 300.f/winWidth;
	lightpos[Y] = (winHeight/2 - y) * 300.f/winHeight;
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glutPostRedisplay();
	break;
    case LIGHT_Z:
	lightpos[Z] = (winHeight/2 - y) * 300.f/winWidth;
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glutPostRedisplay();
	break;
    case PGON:
	angles[X] = (x - winWidth/2) * 180.f/winWidth;
	angles[Y] = (y - winHeight/2) * 180.f/winHeight;
	glutPostRedisplay();
	break;
    }
}


/*
** Create a single component texture map
*/
GLfloat *make_texture(int maxs, int maxt)
{
    int s, t;
    GLfloat *texture;

    /* assumed format; LUMINANCE */
    texture = (GLfloat *)malloc(maxs * maxt * sizeof(GLfloat));
    for(t = 0; t < maxt; t++) {
	for(s = 0; s < maxs; s++) {
	    texture[s + maxs * t] = ((s >> 3) & 0x1) ^ ((t >> 3) & 0x1);
	}
    }
    return texture;
}

/* get current light position in object space */
void
bumpLightPos(GLfloat *x, GLfloat *y, GLfloat *z)
{
    GLdouble mvmatrix[16];
    GLint viewport[4];
    static GLdouble projmatrix[16]; /* to make them zero */
    GLfloat light[4];
    GLdouble Ex, Ey, Ez;
    GLdouble Ox, Oy, Oz;

    CHECK_ERROR("bumpLightPos");
    glGetLightfv(GL_LIGHT0, GL_POSITION, light);
    Ex = light[X]; Ey = light[Y]; Ez = (light[Z] + 1)/2.;

    CHECK_ERROR("bumpLightPos");
    glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
    
    /* identity projection matrix */
    projmatrix[0] = projmatrix[5] = projmatrix[10] = projmatrix[15] = 1.;

    /* identity viewport */
    viewport[0] = viewport[1] = -1; 
    viewport[2] = viewport[3] =  2;

    /*
    ** use the inverse of the modelview matrix to
    ** transform light from eye to object space
    */
    gluUnProject(Ex, Ey, Ez, mvmatrix, projmatrix, viewport, &Ox, &Oy, &Oz);

    *x = Ox; *y = Oy; *z = Oz;
    CHECK_ERROR("bumpLightPos");
}


/* compute binormal from current normal and tangent vector, using cross prod */
/* assuming normal and tangent are already normalized */
void
biNormal(GLfloat *x, GLfloat *y, GLfloat *z)
{
    /* normal <cross> tangent = binormal */

    *x = curNormal[Y] * curTangent[Z] - curNormal[Z] * curTangent[Y];
    *y = curNormal[Z] * curTangent[X] - curNormal[X] * curTangent[Z];
    *z = curNormal[X] * curTangent[Y] - curNormal[Y] * curTangent[X];
}

void
Normalize2f(GLfloat *x, GLfloat *y)
{
    GLfloat len;
    len = *x * *x + *y * *y;
    len = sqrtf(len);

    *x /= len;
    *y /= len;
}

/* rotate point by supplied rotatation matrix */
void
Rotate(GLfloat *rot, GLfloat *light, GLfloat *s, GLfloat *t)
{
    GLfloat r;
    r = rot[3] * light[X] + rot[4] * light[Y] + rot[5] * light[Z];
    if(r < 0.f) /* light below surface */
    {
	*s = 0.f;
	*t = 0.f;
	return;
    }
    *s = rot[0] * light[X] + rot[1] * light[Y] + rot[2] * light[Z];
    *t = rot[6] * light[X] + rot[7] * light[Y] + rot[8] * light[Z];
    Normalize2f(s, t);
}


/* find out where the current normal is */
void
bumpNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
    curNormal[X] = x;
    curNormal[Y] = y;
    curNormal[Z] = z;
    glNormal3f(x, y, z);
}

void
bumpTangent3f(GLfloat x, GLfloat y, GLfloat z)
{
    curTangent[X] = x;
    curTangent[Y] = y;
    curTangent[Z] = z;
}

/* save the texture coordinate call; will shift to do bumpmapping */
void
bumpTexCoord2f(GLfloat s, GLfloat t)
{
    curTex[S] = s;
    curTex[T] = t;
}

/*
** use current tangent vector to compute texture coordinate shift
** then apply it by passing through vertex call
*/
void
bumpVertex3f(GLfloat x, GLfloat y, GLfloat z)
{

    GLfloat Light[3]; /* light in tangent space */
    GLfloat length;
    GLfloat s, t; /* tranformed light, used to shift */
    GLfloat rot[9]; /* rotation matrix (just enought for x and y */
    GLfloat Bx, By, Bz; /* binormal axis */

    if(bumpEnabled)
    {

	/* get current light position */
	Light[X] = curLight[X];
	Light[Y] = curLight[Y];
	Light[Z] = curLight[Z];

	/* find light vector from vertex */
	Light[X] -= x; Light[Y] -= y; Light[Z] -= z;

	length = 1.f/ sqrtf(Light[X] * Light[X] + 
			    Light[Y] * Light[Y] +
			    Light[Z] * Light[Z]);

	Light[X] *= length; Light[Y] *= length; Light[Z] *= length;

	/* create rotation matrix (rotate into tangent space) */
    
	biNormal(&Bx, &By, &Bz); /* find binormal axis */

	rot[0] = curTangent[X]; rot[1] = curTangent[Y]; rot[2] = curTangent[Z];
	rot[3] = curNormal[X]; rot[4] = curNormal[Y]; rot[5] = curNormal[Z];	
	rot[6] = Bx; rot[7] = By; rot[8] = Bz;

	Rotate(rot, Light, &s, &t);

	/* shift coordinates in opposite direction of desired texture shift */
	glTexCoord2f(curTex[S] - s/texture_width, curTex[T] - t/texture_height);
    }
    else
	glTexCoord2f(curTex[S], curTex[T]);

    glVertex3f(x, y, z); /* pass on the vertex call */
}

void
bumpBegin(GLenum prim)
{
    if(bumpEnabled)
    {
	/* get light position; map back from eye to object space */
	bumpLightPos(&curLight[X], &curLight[Y], &curLight[Z]);
    }
    glBegin(prim);
}

void draw(void)
{
    int i, j;
    GLfloat Vx, Vy, Vz; /* vertex */
    GLfloat Tx, Ty, Tz; /* tangent */
    GLfloat Nx, Ny, Nz; /* normal */
    GLfloat c, s; /* cos, sin */

    CHECK_ERROR("start of draw()");


    Ny = 0.f;
    Ty = 0.f;
    /* v(i, j) v(i+1, j), v(i+1, j+1), v(i, j+1) */
    bumpBegin(GL_QUADS);
    for(j = 0; j < steps_y; j ++)
	for(i = 0; i < steps_xz; i++) /* 180 -> 0 degrees */
	{
	    /* v(i, j) */
	    c = cosf(M_PI * (1.f - (GLfloat)i/(steps_xz - 1)));
	    s = sinf(M_PI * (1.f - (GLfloat)i/(steps_xz - 1)));
	    Vx = 100 * c;
	    Vy = j * 200.f/steps_y - 100.f;
	    Vz = 100 * s - 100.f;
	    Nx = c;
	    Nz = s;
	    Tx = s;
	    Ty = -c;
		Tz = 0;
	    bumpNormal3f(Nx, Ny, Nz);
	    bumpTangent3f(Tx, Ty, Tz);
	    bumpTexCoord2f(i/(GLfloat)steps_xz, j/(GLfloat)steps_y);
	    bumpVertex3f(Vx, Vy, Vz);

	    /* v(i+1, j) */
	    c = cosf(M_PI * (1.f - (GLfloat)(i + 1)/(steps_xz - 1)));
	    s = sinf(M_PI * (1.f - (GLfloat)(i + 1)/(steps_xz - 1)));
	    Vx = 100 * c;
	    Vz = 100 * s - 100.f;
	    Nx = c;
	    Nz = s;
	    Tx = s;
	    Ty = -c;
	    bumpNormal3f(Nx, Ny, Nz);
	    bumpTangent3f(Tx, Ty, Tz);
	    bumpTexCoord2f((i + 1)/(GLfloat)steps_xz, j/(GLfloat)steps_y);
	    bumpVertex3f(Vx, Vy, Vz);

	    /* v(i+1, j+1) */
	    Vy = (j + 1) * 200.f/steps_y - 100.f;
	    bumpNormal3f(Nx, Ny, Nz);
	    bumpTangent3f(Tx, Ty, Tz);
	    bumpTexCoord2f((i + 1)/(GLfloat)steps_xz, (j + 1)/(GLfloat)steps_y);
	    bumpVertex3f(Vx, Vy, Vz);

	    /* v(i, j+1) */
	    c = cosf(M_PI * (1.f - (GLfloat)i/(steps_xz - 1)));
	    s = sinf(M_PI * (1.f - (GLfloat)i/(steps_xz - 1)));
	    Vx = 100 * c;
	    Vz = 100 * s - 100.f;
	    Nx = c;
	    Nz = s;
	    Tx = s;
	    Ty = -c;
	    bumpNormal3f(Nx, Ny, Nz);
	    bumpTangent3f(Tx, Ty, Tz);
	    bumpTexCoord2f(i/(GLfloat)steps_xz, (j + 1)/(GLfloat)steps_y);
	    bumpVertex3f(Vx, Vy, Vz);
	}
    glEnd();

    CHECK_ERROR("end of draw()");
}



/* Called when window needs to be redrawn */
void redraw_blendext(void)
{
    GLUquadricObj *obj;
    void draw(void);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


    glLoadIdentity();

    gluLookAt(0., 0., 650., 0., 0., 0., 0., 1., 0.);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    obj = gluNewQuadric();
    gluSphere(obj, 7., 10, 10);
    gluDeleteQuadric(obj);
    glPopMatrix();

    glRotatef(angles[X], 0.f, 1.f, 0.f);
    glRotatef(angles[Y], 1.f, 0.f, 0.f);

    if(textureOnly)
    {
	glEnable(GL_TEXTURE_2D);
	draw();
	glDisable(GL_TEXTURE_2D);
    }
    else if(lightOnly)
    {
	/* draw "z" diffuse component */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	draw();
	glDisable(GL_LIGHTING);
    }
    else /* bumpmapping */
    {

	/* find N dot L */

	/* draw "z" diffuse component */
	/* also do ambient here */
	if(!embossed)
	{
	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
	    draw();
	    glDisable(GL_LIGHTING);
	}
	/* add in shifted values */
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	bumpEnable(); /* shift texture coords */
	draw();
	bumpDisable();

#ifdef GL_EXT_blend_subtract
        if (hasBlendSubtract) {
  	    /* subtract unshifted */
	    glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
	    draw();
	    glBlendEquationEXT(GL_FUNC_ADD_EXT);
	}
#endif
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	if(color)
	{
	    /* Modulate the color texture with N dot L */
	    glEnable(GL_TEXTURE_2D);
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_DST_COLOR, GL_ZERO);
	    glBindTexture(GL_TEXTURE_2D, 1); /* use color texture */
	    draw();
	    glBindTexture(GL_TEXTURE_2D, 0);
	    glDisable(GL_BLEND);
	    glDisable(GL_TEXTURE_2D);
	}

    }

    /* scale up the image */

    glPixelTransferf(GL_RED_SCALE, 2.f);
    glPixelTransferf(GL_GREEN_SCALE, 2.f);
    glPixelTransferf(GL_BLUE_SCALE, 2.f);
    glCopyPixels(0, 0, winWidth, winHeight, GL_COLOR);

    CHECK_ERROR("OpenGL Error in redraw()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}


/* Called when window needs to be redrawn */
void redraw_accum(void)
{
    GLUquadricObj *obj;
    void draw(void);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


    glLoadIdentity();

    gluLookAt(0., 0., 650., 0., 0., 0., 0., 1., 0.);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glRotatef(angles[X], 0.f, 1.f, 0.f);
    glRotatef(angles[Y], 1.f, 0.f, 0.f);

    if(textureOnly)
    {
	glEnable(GL_TEXTURE_2D);
	draw();
	glDisable(GL_TEXTURE_2D);
    }
    else if(lightOnly)
    {
	/* draw "z" diffuse component */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	draw();
	glDisable(GL_LIGHTING);
    }
    else /* bumpmapping */
    {

	CHECK_ERROR("start");
	/* draw "z" diffuse component */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	draw();
	glAccum(GL_LOAD, .5f);
	CHECK_ERROR("load");
	glDisable(GL_LIGHTING);

	/* draw shifted */
	glEnable(GL_TEXTURE_2D);
	bumpEnable();
	draw();
	glAccum(GL_ACCUM, .5f);
	bumpDisable();

	/* subtract unshifted */
	draw();
	glAccum(GL_ACCUM, -.5f);
	
	glDisable(GL_TEXTURE_2D);
	glAccum(GL_RETURN, 2.f);

	if(color)
	{
	    /* Modulate the color texture with N dot L */
	    glEnable(GL_TEXTURE_2D);
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_DST_COLOR, GL_ZERO);
	    glBindTexture(GL_TEXTURE_2D, 1); /* use color texture */
	    draw();
	    glBindTexture(GL_TEXTURE_2D, 0);
	    glDisable(GL_BLEND);
	    glDisable(GL_TEXTURE_2D);
	}

    }

    

    CHECK_ERROR("OpenGL Error in redraw()");

    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0., 0., 650., 0., 0., 0., 0., 1., 0.);
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    obj = gluNewQuadric();
    gluSphere(obj, 7., 10, 10);
    gluDeleteQuadric(obj);
    glPopMatrix();


    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 't':
	if(textureOnly)
	    textureOnly = FALSE;
	else
	    textureOnly = TRUE;
	glutPostRedisplay();
	break;
    case 'l':
	if(lightOnly)
	    lightOnly = FALSE;
	else
	    lightOnly = TRUE;
	glutPostRedisplay();
	break;
    case 'e':
	if(embossed)
	    embossed = FALSE;
	else
	    embossed = TRUE;
	glutPostRedisplay();
	break;
    case 'B': /* make bumps taller */
	bumpscale += .01f;
	printf("bump map scale = %.2f\n", bumpscale);
	glPixelTransferf(GL_RED_SCALE, bumpscale);
	glPixelTransferf(GL_GREEN_SCALE, bumpscale);
	glPixelTransferf(GL_BLUE_SCALE, bumpscale);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
		     texture_width, texture_height, 0, GL_RGBA,
		     GL_UNSIGNED_BYTE, bumptex);
	glutPostRedisplay();
	break;
    case 'b': /* make bumps flatter */
	bumpscale -= .01f;
	if(bumpscale < 0.f)
	    bumpscale = 0.f;
	printf("bump map scale = %.2f\n", bumpscale);
	glPixelTransferf(GL_RED_SCALE, bumpscale);
	glPixelTransferf(GL_GREEN_SCALE, bumpscale);
	glPixelTransferf(GL_BLUE_SCALE, bumpscale);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
		     texture_width, texture_height, 0, GL_RGBA,
		     GL_UNSIGNED_BYTE, bumptex);
	glutPostRedisplay();
	break;
    case 'c':
	if(color == TRUE)
	    color = FALSE;
	else
	    color = TRUE;
	glutPostRedisplay();
	break;
    case 'w':
	if(wire == TRUE)
	{
	    glPolygonMode(GL_FRONT, GL_FILL);
	    wire = FALSE;
	}
	else
	{
	    glPolygonMode(GL_FRONT, GL_LINE);
	    wire = TRUE;
	}
	glutPostRedisplay();
	break;
    case 'x':
	steps_xz -= 2;
	if(steps_xz < 1)
	    steps_xz = 1;
	glutPostRedisplay();
	break;
    case 'X':
	steps_xz += 2;
	glutPostRedisplay();
	break;
    case 'y':
	steps_y -= 2;
	if(steps_y < 1)
	    steps_y = 1;
	glutPostRedisplay();
	break;
    case 'Y':
	steps_y += 2;
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
	break;
    case 'h':
    case '?':
    default:
	fprintf(stderr, 
		"Keyboard commands:\n"
		"t-texture only\n"
		"l-light only\n"
		"c-color texture\n"
		"e-embossed (horizontal part)\n"
		"w-toggle wireframe\n"
		"B-increase bumps b-decrease bumps\n");
	break;
    }

}



main(int argc, char *argv[])
{
    unsigned *tex;
    GLfloat lightpos[4];
    GLfloat diffuse[4];
    GLboolean valid;
    int texcomps, texwid, texht;
    const char *version;
    char varray[32];

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	int done = FALSE;
	while(!done)
	{
	    switch(*args)
	    {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = FALSE;
		break;
	    case 'a': /* use accumulation buffer */
		printf("Use accumulation buffer\n");
		accum = TRUE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	if(accum)
	    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_ACCUM);
        else
	    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    else
	if(accum)
	    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_ACCUM);
        else
	    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);

    (void)glutCreateWindow("bump mapping example program");
    if(accum)
	glutDisplayFunc(redraw_accum);
    else
	glutDisplayFunc(redraw_blendext);

    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);


    
    version = (char *) glGetString(GL_VERSION);
    strncpy(varray, version, strcspn(version, " "));
    printf("%s\n", version);
    if(atof(varray) > 1.f)
	bindtex = TRUE;
    else
	bindtex = FALSE;

    glRasterPos3i(-1, -1, -1);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
    if(!valid)
	printf("invalid raster position!\n");
    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-150., 150., -150., 150., 500., 800.);
    glMatrixMode(GL_MODELVIEW);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);


    diffuse[R] = diffuse[G] = diffuse[B] = .4f;
    diffuse[A] = 1.f;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    bumptex = read_texture("../data/opengl.bw", &texture_width, 
		       &texture_height, &texcomps);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    if(!accum)
    {
	/* scale to match maximum z (default Kd = .8, default Ld = 1. */
	/* divide by 2 to stay in range of 0 to 1 */
	glPixelTransferf(GL_RED_SCALE, bumpscale);
	glPixelTransferf(GL_GREEN_SCALE, bumpscale);
	glPixelTransferf(GL_BLUE_SCALE, bumpscale);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
		 texture_width, texture_height, 0, GL_RGBA,
		 GL_UNSIGNED_BYTE, bumptex);

    tex = read_texture("../data/plank.rgb", &texwid, &texht, &texcomps);

    glBindTexture(GL_TEXTURE_2D, 1); /* for color */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glPixelTransferf(GL_RED_SCALE, 1.f);
    glPixelTransferf(GL_GREEN_SCALE, 1.f);
    glPixelTransferf(GL_BLUE_SCALE, 1.f);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		 texwid, texht, 0, GL_RGBA,
		 GL_UNSIGNED_BYTE, tex);

    glBindTexture(GL_TEXTURE_2D, 0);

    free(tex);

    lightpos[X] = lightpos[Y] = 0.f;
    lightpos[Z] = -90.f;
    lightpos[W] = 1.f;
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glDisable(GL_DITHER);

    CHECK_ERROR("end of main");

#ifdef GL_EXT_blend_subtract
    if(!glutExtensionSupported("GL_EXT_blend_subtract")) {
      fprintf(stderr,
        "bump: requires OpenGL blend subtract extension to operate correctly.\n");
      hasBlendSubtract = 0;
    } else {
      hasBlendSubtract = 1;
#if defined(_WIN32) && !defined(MESA)
      glBlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC) wglGetProcAddress("glBlendEquationEXT");
      if (glBlendEquationEXT == NULL) {
        hasBlendSubtract = 0;
      }
#endif
    }
#endif

    key('?', 0, 0); /* startup message */
    glutMainLoop();
    return(0);
}
