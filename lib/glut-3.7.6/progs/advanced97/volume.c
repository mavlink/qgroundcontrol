#include <GL/glut.h>
#include <math.h>
#include "texture.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* nonzero if not power of 2 */ 
#define NOTPOW2(num) ((num) & (num - 1))

int
makepow2(int val)
{
    int power = 0;
    if(!val)
	return 0;

    while(val >>= 1)
	power++;

    return(1 << power);
}

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

enum {X, Y, Z, W};
enum {R, G, B, A};
enum {OVER, ATTENUATE, NONE, LASTOP}; /* blend modes */
/* mouse modes */
enum {OBJ_ANGLE, SLICES, CUTTING, GEOMXY, GEOMZ, MINBOOST, BOOSTWID, BOOST}; 
enum {NOLIST, SPHERE}; /* display list */

/* window dimensions */
int winWidth = 512;
int winHeight = 512;
int active;
int operator = OVER;
GLboolean texture = GL_TRUE;
GLboolean dblbuf = GL_TRUE;
GLboolean cut = GL_FALSE;
GLboolean geom = GL_FALSE;
GLboolean map = GL_FALSE;
GLint cutbias = 50;
int hasBlendColor = 0;
#if defined(_WIN32) && !defined(MESA)
#include <windows.h>
PFNGLBLENDCOLOREXTPROC glBlendColorEXT;
#endif

GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[3] = {0.f, 0.f, 0.f};


GLfloat minboost = 0.f, boostwid = .03f, boost = 3.f; /* transfer function */

/* 3d texture data that's read in */
/* XXX TODO; make command line arguments */
int Texwid = 128; /* dimensions of each 2D texture */
int Texht = 128;
int Texdepth = 69; /* number of 2D textures */

/* Actual dimensions of the texture (restricted to max 3d texture size) */
int texwid, texht, texdepth;
int slices;
GLubyte *tex3ddata; /* pointer to 3D texture data */


GLfloat *lighttex = 0;
GLfloat lightpos[4] = {0.f, 0.f, 1.f, 0.f};
GLboolean lightchanged[2] = {GL_TRUE, GL_TRUE};


void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
}


void
motion(int x, int y)
{
    switch(active)
    {
    case OBJ_ANGLE:
	objangle[X] = (x - winWidth/2) * 360./winWidth;
	objangle[Y] = (y - winHeight/2) * 360./winHeight;
	glutPostRedisplay();
	break;
    case SLICES:
	slices = x * texwid/winWidth;
	glutPostRedisplay();
	break;
    case CUTTING:
	cutbias = (x - winWidth/2) * 300/winWidth;
	glutPostRedisplay();
	break;
    case GEOMXY:
	objpos[X] = (x - winWidth/2) * 300/winWidth;
	objpos[Y] = (winHeight/2 - y) * 300/winHeight;
	glutPostRedisplay();
	break;
    case GEOMZ:
	objpos[Z] = (x - winWidth/2) * 300/winWidth;
	glutPostRedisplay();
	break;
    case MINBOOST:
	minboost = x * .25f/winWidth;
	glutPostRedisplay();
	break;
    case BOOSTWID:
	boostwid = x * .5f/winWidth;
	glutPostRedisplay();
	break;
    case BOOST:
	boost = x * 20.f/winWidth;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* rotate the data volume */
	    if(map)
		active = MINBOOST;
	    else
		active = OBJ_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    if(map)
		active = BOOSTWID;
	    else
		if(cut)
		    active = CUTTING; /* move cutting plane */
		else
		    active = GEOMXY; /* move geometry */
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    if(map)
		active = BOOST;
	    else
		if(geom)
		    active = GEOMZ;
		else
		    active = SLICES;
	    motion(x, y);
	    break;
	}
}

/* use pixel path to remap 3D texture data */
void
remaptex(void)
{
    int i, size;
    GLfloat *map;

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

    glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &size);

    map = (GLfloat *)malloc(sizeof(GLfloat) * size);
    for(i = 0; i < size;i++)
    {
	map[i] = (GLfloat)i/(size - 1);
	if(((GLfloat)i/size > minboost) &&
	   ((GLfloat)i/size < minboost + boostwid))
	{
	    map[i] *= boost;
	}
	else
	    map[i] /= boost;
    }

    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, size, map);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, size, map);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, size, map);
    glPixelMapfv(GL_PIXEL_MAP_A_TO_A, size, map);

#ifdef GL_EXT_texture3D
    glTexImage3DEXT(GL_TEXTURE_3D_EXT, 0, GL_LUMINANCE_ALPHA,
		    texwid, texht, texdepth,
		    0,
		    GL_RGBA, GL_UNSIGNED_BYTE, tex3ddata);
#endif

    glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
    free(map);

    CHECK_ERROR("OpenGL Error in remaptex()");
}


GLdouble clipplane0[] = {-1.,  0.,  0., 100.}; /* x < 100 out */
GLdouble clipplane1[] = { 1.,  0.,  0., 100.}; /* x > 100 out */
GLdouble clipplane2[] = { 0., -1.,  0., 100.}; /* y < 100 out */
GLdouble clipplane3[] = { 0.,  1.,  0., 100.}; /* y > 100 out */
GLdouble clipplane4[] = { 0.,  0., -1., 100.}; /* z < 100 out */
GLdouble clipplane5[] = { 0.,  0.,  1., 100.}; /* z > 100 out */

/* define a cutting plane */
GLdouble cutplane[] = {0.f, -.5f, -2.f, 50.f};

/* draw the object unlit without surface texture */
void redraw(void)
{
    int i;
    GLfloat offS, offT, offR; /* mapping texture to planes */

    offS = 200.f/texwid;
    offT = 200.f/texht;
    offR = 200.f/texdepth;
    
    clipplane0[W] = 100.f - offS;
    clipplane1[W] = 100.f - offS;
    clipplane2[W] = 100.f - offT;
    clipplane3[W] = 100.f - offT;
    clipplane4[W] = 100.f - offR;
    clipplane5[W] = 100.f - offR;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if(map)
	remaptex();

    /* GL_MODELVIEW */
    if(cut)
    {
	cutplane[W] = cutbias;
	glClipPlane(GL_CLIP_PLANE5, cutplane);
    }

    glPushMatrix(); /* identity */
    glRotatef(objangle[X], 0.f, 1.f, 0.f);
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    glClipPlane(GL_CLIP_PLANE0, clipplane0);
    glClipPlane(GL_CLIP_PLANE1, clipplane1);
    glClipPlane(GL_CLIP_PLANE2, clipplane2);
    glClipPlane(GL_CLIP_PLANE3, clipplane3);
    glClipPlane(GL_CLIP_PLANE4, clipplane4);
    if(!cut)
	glClipPlane(GL_CLIP_PLANE5, clipplane5);
    glPopMatrix(); /* back to identity */

    /* draw opaque geometry here */
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    if(geom)
    {
	if(!cut)
	    glDisable(GL_CLIP_PLANE5);
	glPushMatrix();
	glTranslatef(objpos[X], objpos[Y], objpos[Z]);
	glCallList(SPHERE);
	glPopMatrix();
    }
    glMatrixMode(GL_TEXTURE);
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);
    glEnable(GL_CLIP_PLANE4);
    glEnable(GL_CLIP_PLANE5);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix(); /* identity */
    glTranslatef( .5f,  .5f, .5f);
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    glRotatef(objangle[X], 0.f, 0.f, 1.f);
    glTranslatef( -.5f,  -.5f, -.5f);

    switch(operator)
    {
    case OVER:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	break;
    case ATTENUATE:
#ifdef GL_EXT_blend_color
        if (hasBlendColor){
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE);
	    glBlendColorEXT(1.f, 1.f, 1.f, 1.f/slices);
        } else
#endif
        {
            fprintf(stderr, "volume: attenuate not supported!\n");
        }
        break;
    case NONE:
	/* don't blend */
	break;
    }

    if(texture) {
#ifdef GL_EXT_texture3D
       glEnable(GL_TEXTURE_3D_EXT);
#endif
    } else {
#ifdef GL_EXT_texture3D
       glDisable(GL_TEXTURE_3D_EXT);
#endif
       glEnable(GL_LIGHTING);
       glEnable(GL_LIGHT0);
    }

    

    for(i = 0; i < slices; i++)
    {
	glBegin(GL_QUADS);
	glVertex3f(-100.f, -100.f, 
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glVertex3f( 100.f, -100.f,
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glVertex3f( 100.f,  100.f,
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glVertex3f(-100.f,  100.f,
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glEnd();
    }
#ifdef GL_EXT_texture3D
    glDisable(GL_TEXTURE_3D_EXT);
#endif
    if(!texture)
    {
       glDisable(GL_LIGHTING);
    }
    glDisable(GL_BLEND);

    glPopMatrix(); /* back to identity */
    glMatrixMode(GL_MODELVIEW);

    if(operator == ATTENUATE)
    {
	glPixelTransferf(GL_RED_SCALE, 3.f); /* brighten image */
	glPixelTransferf(GL_GREEN_SCALE, 3.f);
	glPixelTransferf(GL_BLUE_SCALE, 3.f);
	glCopyPixels(0, 0, winWidth, winHeight, GL_COLOR);
    }
    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'm': /* remap texture values */
	if(map)
	{
	    fprintf(stderr, "remapping off\n");
	    map = GL_FALSE;
	}
	else
	{
	    fprintf(stderr, "remapping on:\n"
		            "left mouse moves emphasize value\n"
		            "middle mouse moves emphasize width\n"
		            "right mouse adjusts gain\n");
	    map = GL_TRUE;
	}

	remaptex();
	glutPostRedisplay();
	break;
    case 'o':
	operator++;
	if(operator == LASTOP)
	    operator = OVER;
	glutPostRedisplay();
	break;
    case 't':
	if(texture)
	    texture = GL_FALSE;
	else
	    texture = GL_TRUE;
	glutPostRedisplay();
	break;
    case 'c':
	if(cut)
	{
	    fprintf(stderr, "cutting plane off\n");
	    cut = GL_FALSE;
	}
	else
	{
	    fprintf(stderr, 
		    "Cutting plane on: "
		    "middle mouse (horizontal) moves cutting plane\n");
	    cut = GL_TRUE;
	}
	glutPostRedisplay();
	break;
    case 'g': /* toggle geometry */
	if(geom)
	    geom = GL_FALSE;
	else
	    geom = GL_TRUE;
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
	break;
    case '?':
    case 'h':
    default:
	fprintf(stderr, 
		"Keyboard Commands\n"
		"m - toggle transfer function (remapping)\n"
		"o - toggle operator\n"
		"t - toggle 3D texturing\n"
		"c - toggle cutting plane\n"
		"g - toggle geometry\n");
	break;
    }
}

GLubyte *
loadtex3d(int *texwid, int *texht, int *texdepth, int *texcomps)
{
    char *filename;
    GLubyte *tex3ddata;
    GLuint *texslice; /* 2D slice of 3D texture */
    GLint max3dtexdims; /* maximum allowed 3d texture dimension */
    GLint newval;
    int i;

    /* load 3D texture data */
    filename = (char*)malloc(sizeof(char) * strlen("../data/skull/skullXX.la"));

    tex3ddata = (GLubyte *)malloc(Texwid * Texht * Texdepth * 
				  4 * sizeof(GLubyte));
    for(i = 0; i < Texdepth; i++)
    {
	sprintf(filename, "../data/skull/skull%d.la", i);
	/* read_texture reads as RGBA */
	texslice = read_texture(filename, texwid, texht, texcomps);
	memcpy(&tex3ddata[i * Texwid * Texht * 4],  /* copy in a slice */
	       texslice, 
	       Texwid * Texht * 4 * sizeof(GLubyte));
	free(texslice);
    }
    free(filename);

    *texdepth = Texdepth;

    max3dtexdims = 0;
#ifdef GL_EXT_texture3D
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &max3dtexdims);
#endif

    /* adjust width */
    newval = *texwid;
    if(*texwid > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texwid))
        newval = makepow2(*texwid);
    if(newval != *texwid)
    {
	glPixelStorei(GL_UNPACK_ROW_LENGTH, *texwid);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, (*texwid - newval)/2);
	*texwid = newval;
    }

    /* adjust height */
    newval = *texht;
    if(*texht > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texht))
        newval = makepow2(*texht);
    if(*texht > newval)
    {
#ifdef GL_EXT_texture3D
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT_EXT, *texht);
#endif
	glPixelStorei(GL_UNPACK_SKIP_ROWS, (*texht - newval)/2);
	*texht = newval;
    }

    /* adjust depth */
    newval = *texdepth;
    if(*texdepth > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texdepth))
        newval = makepow2(*texdepth);
    if(*texdepth > newval)
    {
	*texdepth = newval;
    }
    return tex3ddata;
}



main(int argc, char *argv[])
{
    int texcomps;
    static GLfloat splane[4] = {1.f/200.f, 0.f, 0.f, .5f};
    static GLfloat rplane[4] = {0, 1.f/200.f, 0, .5f};
    static GLfloat tplane[4] = {0, 0, 1.f/200.f, .5f};
    static GLfloat lightpos[4] = {150., 150., 150., 1.f};


    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done)
	{
	    switch(*args)
	    {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = GL_FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = GL_TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);

    (void)glutCreateWindow("volume rendering demo");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);

    /* Initialize OpenGL State */

    /* draw a perspective scene */
#if 0
    glMatrixMode(GL_PROJECTION);
    /* cube, 300 on a side */
    glFrustum(-150., 150., -150., 150., 300., 600.);
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 450) */
    gluLookAt(0., 0., 450., 0., 0., 0., 0., 1., 0.);
#else
    glMatrixMode(GL_PROJECTION);
    /* cube, 300 on a side */
    glOrtho(-150., 150., -150., 150., -150., 150.);
    glMatrixMode(GL_MODELVIEW);
#endif

    glEnable(GL_DEPTH_TEST);
#ifdef GL_EXT_texture3D
    glEnable(GL_TEXTURE_3D_EXT);
#endif

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    glTexGenfv(GL_S, GL_OBJECT_PLANE, splane);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tplane);
    glTexGenfv(GL_R, GL_OBJECT_PLANE, rplane);

#ifdef GL_EXT_texture3D
    /* to avoid boundary problems */
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP);
#endif

    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);
    glEnable(GL_CLIP_PLANE4);
    glEnable(GL_CLIP_PLANE5);

    glDisable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);



    tex3ddata = loadtex3d(&texwid, &texht, &texdepth, &texcomps);

    slices = texht;

#ifdef GL_EXT_texture3D
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage3DEXT(GL_TEXTURE_3D_EXT, 0, GL_LUMINANCE_ALPHA,
		    texwid, texht, texdepth,
		    0,
		    GL_RGBA, GL_UNSIGNED_BYTE, tex3ddata);
#endif

    /* make a display list containing a sphere */
    glNewList(SPHERE, GL_COMPILE);
    {
	static GLfloat lightpos[] = {150.f, 150.f, 150.f, 1.f};
	static GLfloat material[] = {1.f, .5f, 1.f, 1.f};
	GLUquadricObj *qobj = gluNewQuadric();
	glPushAttrib(GL_LIGHTING_BIT);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material);
	gluSphere(qobj, 20.f, 20, 20);
	gluDeleteQuadric(qobj);
	glPopAttrib();
    }
    glEndList();

    key('?', 0, 0); /* print usage message */

    CHECK_ERROR("end of main");

    if(!glutExtensionSupported("GL_EXT_texture3d")) {
      fprintf(stderr,
        "volume: requires OpenGL texture 3D extension to operate correctly.\n");
    }
    hasBlendColor = glutExtensionSupported("GL_EXT_blend_color");
    if(!hasBlendColor) {
      fprintf(stderr,
        "volume: needs OpenGL blend color extension to attenuate.\n");
#if defined(_WIN32) && !defined(MESA)
      glBlendColorEXT = (PFNGLBLENDCOLOREXTPROC) wglGetProcAddress("glBlendColorEXT");
      if (glBlendColorEXT == NULL) {
        hasBlendColor = 0;
      }
#endif
    }

    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
