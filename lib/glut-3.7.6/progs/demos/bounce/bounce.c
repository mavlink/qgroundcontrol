
/**************************************************************************
 *									  *
 * 	 Copyright (C) 1988, 1989, 1990, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 *  foo $Revision: 1.4 $
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#include "tb.h"
#include "glui.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define UDIV 12
#define VDIV 12

#define WALLGRIDMAX 32
#define EYEZ 3.3

#define TOTALBALLS 3

#define R 0
#define G 1
#define B 2

#define X 0
#define Y 1
#define Z 2
#define W 3

int wallgrid = 8; /* sqrt of the number of quads in a wall */
float fatt = 1.0;

int freeze = GL_FALSE;
int spin = GL_FALSE;
int spinning = GL_FALSE;
int objecton = GL_FALSE;
int normson = GL_FALSE;
int lighton[3] = {GL_TRUE, GL_TRUE, GL_TRUE};

int window;				/* main window id */

GLboolean performance = GL_FALSE;	/* performance indicator */

struct {
   float p[3];
   float d[3];
   unsigned char color[3];
} balls[TOTALBALLS];

float ballobj[UDIV+1][VDIV+1][4];
float wallobj[WALLGRIDMAX+1][WALLGRIDMAX+1][4];
float wallnorms[WALLGRIDMAX+1][WALLGRIDMAX+1][3];
float wallnorm[3] = { 0.0, 0.0, -1.0 };

int orx, ory;

float ballscale;
float ballsize;

int DELTAX, DELTAY;

int lflag = 0;

float newpos[] = { 0.0, 0.0, 0.0, 1.0 };

GLfloat light_Ka[] = { 0.3, 0.3, 0.3, 1.0 };  /* ambient */
GLfloat light_Ks[] = { 0.0, 0.0, 0.0, 1.0 };  /* specular */

GLfloat light0_Ka[]  = { 0.0, 0.0, 0.0, 1.0 };  /* ambient */
GLfloat light0_Kd[]  = { 1.0, 0.1, 0.1, 1.0 };  /* diffuse */
GLfloat light0_pos[] = { 0.0, 0.0, 0.0, 1.0 };  /* position */

GLfloat light1_Ka[]  = { 0.0, 0.0, 0.0, 1.0 };  /* ambient */
GLfloat light1_Kd[]  = { 0.1, 1.0, 0.1, 1.0 };  /* diffuse */
GLfloat light1_pos[] = { 0.0, 0.0, 0.0, 1.0 };  /* position */

GLfloat light2_Ka[]  = { 0.0, 0.0, 0.0, 1.0 };  /* ambient */
GLfloat light2_Kd[]  = { 0.1, 0.1, 1.0, 1.0 };  /* diffuse */
GLfloat light2_pos[] = { 0.0, 0.0, 0.0, 1.0 };  /* position */

GLfloat attenuation[] = { 1.0, 3.0 };

GLfloat plane_Ka[] = { 0.0, 0.0, 0.0, 1.0 };  /* ambient */
GLfloat plane_Kd[] = { 0.4, 0.4, 0.4, 1.0 };  /* diffuse */
GLfloat plane_Ks[] = { 1.0, 1.0, 1.0, 1.0 };  /* specular */
GLfloat plane_Ke[] = { 0.0, 0.0, 0.0, 1.0 };  /* emission */
GLfloat plane_Se   = 30.0;                    /* shininess */

GLfloat wall_Ka[] = { 0.1, 0.1, 0.1, 1.0 };  /* ambient */
GLfloat wall_Kd[] = { 0.8, 0.8, 0.8, 1.0 };  /* diffuse */
GLfloat wall_Ks[] = { 1.0, 1.0, 1.0, 1.0 };  /* specular */
GLfloat wall_Ke[] = { 0.0, 0.0, 0.0, 1.0 };  /* emission */
GLfloat wall_Se   = 20.0;                    /* shininess */

GLuint wall_material, plane_material; /* material display lists */

char ofile[80];


/************************************************************/
/* XXX - The following is an excerpt from spin.h from spin  */
/************************************************************/

#define POLYGON	1
#define LINES	2
#define TRANSPERENT 3
#define DISPLAY	4
#define LMATERIAL 5

#define FASTMAGIC	0x5423

typedef struct fastobj {
    int npoints;
    int colors;
    int type;
    int material;
    int display;
    int ablend;
    GLint *data;
} fastobj;

/*
 * Wrappers to do either lines or polygons
 */
#define PolyOrLine()	if (lflag == LINES) { \
				glBegin(GL_LINE_LOOP); \
			} else { \
				glBegin(GL_POLYGON); \
			}

#define EndPolyOrLine() if (lflag == LINES) { \
				glEnd(); \
			} else { \
				glEnd(); \
			}

/************************* end of spin.h excerpt *************************/


fastobj	*obj = NULL;

/* 

   general purpose text routine.  draws a string according to the
   format in a stroke font at x, y after scaling it by the scale
   specified.  x, y and scale are all in window-space [i.e., pixels]
   with origin at the lower-left.

*/

void 
text(GLuint x, GLuint y, GLfloat scale, char* format, ...)
{
    va_list args;
    char buffer[255], *p;
    GLfloat font_scale = 119.05 + 33.33;

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glTranslatef(x, y, 0.0);

    glScalef(scale/font_scale, scale/font_scale, scale/font_scale);

    for(p = buffer; *p; p++)
	glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  
    glPopAttrib();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

float
frand(void)
{
    return 2.0*(rand()/32768.0 - .5);
}


void
resetballs(void)
{
    register short i;

    balls[0].color[R] = 255;
    balls[0].color[G] = 64;
    balls[0].color[B] = 64;
    balls[1].color[R] = 64;
    balls[1].color[G] = 255;
    balls[1].color[B] = 64;
    balls[2].color[R] = 64;
    balls[2].color[G] = 64;
    balls[2].color[B] = 255;
    for (i = 0; i < TOTALBALLS; i++) {
	balls[i].p[0] = 0.0;
	balls[i].p[1] = 0.0;
	balls[i].p[2] = 0.0;
	balls[i].d[0] = .1*frand();
	balls[i].d[1] = .1*frand();
	balls[i].d[2] = .1*frand();
    }
}


void
drawface(void)
{
    register int i,j;

    glNormal3fv(wallnorm);
    for (i=0; i < wallgrid; i++) {
	glBegin(GL_TRIANGLE_STRIP);
	for (j=0; j <= wallgrid; j++) {
	    glVertex3fv(wallobj[i][j]);
	    glVertex3fv(wallobj[i+1][j]);
	}
	glEnd();
    }
}


void
drawnorms(void)
{
    register int i,j;

    glDisable(GL_LIGHTING);
    glColor3ub(255, 255, 0);
    for (i=0; i <= wallgrid; i++) {
	for (j=0; j <= wallgrid; j++) {
	    glBegin(GL_LINES);
	    glVertex3fv(wallobj[i][j]);
	    glVertex3fv(wallnorms[i][j]);
	    glEnd();
	}
    }
    glEnable(GL_LIGHTING);
}


void
drawbox(void)
{
    glPushMatrix();

/*  drawface();		*/
    glRotatef(90.0, 0.0, 1.0, 0.0);
    drawface();
    if (normson) drawnorms();
    glRotatef(90.0, 0.0, 1.0, 0.0);
    drawface();
    if (normson) drawnorms();
    glRotatef(90.0, 0.0, 1.0, 0.0);
/*  drawface();		*/
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    drawface();
    if (normson) drawnorms();
    glRotatef(180.0, 1.0, 0.0, 0.0);
/*  drawface();		*/
    glPopMatrix();
}


void
drawfastobj(fastobj *obj)
{
    register GLint *p, *end;
    register int npolys;

    p = obj->data;
    end = p + 8 * obj->npoints;

    if(obj->colors) {
	npolys = obj->npoints/4;
	while(npolys--) {
	    PolyOrLine();
	    glColor3iv(p);
	    glVertex3fv((float *)p+4);
	    glColor3iv(p+8);
	    glVertex3fv((float *)p+12);
	    glColor3iv(p+16);
	    glVertex3fv((float *)p+20);
	    glColor3iv(p+24);
	    glVertex3fv((float *)p+28);
	    EndPolyOrLine();
	    p += 32;
	}
    } else {
	while ( p < end) {
	    PolyOrLine();
	    glNormal3fv((float *)p);
	    glVertex3fv((float *)p+4);
	    glNormal3fv((float *)p+8);
	    glVertex3fv((float *)p+12);
	    glNormal3fv((float *)p+16);
	    glVertex3fv((float *)p+20);
	    glNormal3fv((float *)p+24);
	    glVertex3fv((float *)p+28);
	    EndPolyOrLine();
	    p += 32;
	}
    }
}

void
drawball(void)
{
    register int i,j;

    for (i=0; i < UDIV; i++) {
	for (j=0; j < VDIV; j++) {
	    glBegin(GL_POLYGON);
	    glVertex4fv( ballobj[i][j] );
	    glVertex4fv( ballobj[i+1][j] );
	    glVertex4fv( ballobj[i+1][j+1] );
	    glVertex4fv( ballobj[i][j+1] );
	    glEnd();
	}
    }
}



void
drawimage(void)
{
    register short i;
    static int start, end, last;

    glutSetWindow(window);

    if (performance)
	start = glutGet(GLUT_ELAPSED_TIME);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    tbMatrix();

    for (i=0; i < TOTALBALLS; i++) {
	newpos[0] = balls[i].p[0];
	newpos[1] = balls[i].p[1];
	newpos[2] = balls[i].p[2];
	glLightfv(GL_LIGHT0 + i, GL_POSITION, newpos);
    }

    glCallList(wall_material);
    glEnable(GL_LIGHTING);
    drawbox();

    glEnable(GL_DEPTH_TEST);

    if (objecton)
    {
	glCallList(plane_material);
	glPushMatrix();
	glScalef(1.5, 1.5, 1.5);
	glRotatef(180.0, 0.0, 0.0, 1.0);
	if (spin)
	{
	    orx += 50;
	    ory += 50;
	}
	glRotatef(orx/10.0, 1.0, 0.0, 0.0);
	glRotatef(ory/10.0, 0.0, 1.0, 0.0);
	drawfastobj(obj);
	glPopMatrix();
    }

    glDisable(GL_LIGHTING);

    for (i=0; i < TOTALBALLS; i++) {
	if (lighton[i])
	{
	    glPushMatrix();
	    glTranslatef(balls[i].p[0],balls[i].p[1],balls[i].p[2]);
	    glColor3ubv(balls[i].color);
	    drawball();
	    glPopMatrix();
	}
    }

    glColor3f(1.0, 1.0, 1.0);
    if (performance) {
        if (end - last == 0) {
	    text(10, 73, 20, "unknown fps");
	} else {
	    text(10, 73, 20, "%.0f fps", 1.0 / ((end - last) / 1000.0));
	}
	last = start;
    }
    text(10, 43, 14, "Attenuation [%.2f]", fatt);
    text(10, 13, 14, "Tesselation [%3d]", wallgrid);


    glPopMatrix();
    glutSwapBuffers();

    if (performance)
	end = glutGet(GLUT_ELAPSED_TIME);
}


void
initobjects(void)
{
    register float u,v,du,dv;
    register short i,j;

    du = 2.0*M_PI/UDIV;
    dv = M_PI/VDIV;

    u = 0.;
    for (i=0; i <= UDIV; i++) {
	v = 0.;
	for (j=0; j <= VDIV; j++) {
	    ballobj[i][j][X] = ballsize*cos(u)*sin(v);
	    ballobj[i][j][Y] = ballsize*sin(u)*sin(v);
	    ballobj[i][j][Z] = ballsize*cos(v);
	    ballobj[i][j][W] = 1.0;
	    v += dv;
	}
	u += du;
    }

    for (i=0; i <= wallgrid; i++) {
	for (j=0; j <= wallgrid; j++) {
	    wallobj[i][j][X] = -1.0 + 2.0*i/wallgrid;
	    wallobj[i][j][Y] = -1.0 + 2.0*j/wallgrid;
	    wallobj[i][j][Z] = 1.0;
	    wallobj[i][j][W] = 1.0;
	}
    }

    for (i=0; i <= wallgrid; i++) {
	for (j=0; j <= wallgrid; j++) {
	    wallnorms[i][j][X] = wallobj[i][j][X] + wallnorm[X]*0.1;
	    wallnorms[i][j][Y] = wallobj[i][j][Y] + wallnorm[Y]*0.1;
	    wallnorms[i][j][Z] = wallobj[i][j][Z] + wallnorm[Z]*0.1;
	}
    }
}

int MOUSEX, MOUSEY;

static void
mouse(int button, int state, int x, int y)
{
    MOUSEX = x;
    MOUSEY = y;
    tbMouse(button, state, x, y);
}

static void
motion(int x, int y)
{
    DELTAX -= MOUSEX - x;
    DELTAY += MOUSEY - y;
    MOUSEX = x;
    MOUSEY = y;
    tbMotion(x, y);
}

/* ARGSUSED1 */
void
keyboard(unsigned char key, int x, int y)
{
    switch(key) {
    case 27: /* ESC */
	exit(0);
	break;

    case '+':
	wallgrid++;
	if (wallgrid > WALLGRIDMAX)
	    wallgrid = WALLGRIDMAX;
	initobjects();
	break;

    case '-':
	wallgrid--;
	if (wallgrid < 1)
	    wallgrid = 1;
	initobjects();
	break;
    }
}

void
initialize(void)
{
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    window = glutCreateWindow("bounce");

    initobjects();

    srand(glutGet(GLUT_ELAPSED_TIME));

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_Ka);

    plane_material = glGenLists(1);
    glNewList(plane_material, GL_COMPILE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, plane_Ka);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, plane_Kd);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, plane_Ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, plane_Ke);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, plane_Se);
    glEndList();

    wall_material = glGenLists(1);
    glNewList(wall_material, GL_COMPILE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, wall_Ka);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wall_Kd);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, wall_Ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, wall_Ke);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, wall_Se);
    glEndList();

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_Ka);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_Kd);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, attenuation[0]);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, attenuation[1]);
    /* OpenGL's light0 has different specular properties than the rest
       of the lights.... */
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_Ks);

    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_Ka);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_Kd);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, attenuation[0]);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, attenuation[1]);

    glLightfv(GL_LIGHT2, GL_AMBIENT, light2_Ka);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_Kd);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, attenuation[0]);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, attenuation[1]);

    glutMotionFunc(motion);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
}


void
calcball(void)
{
    register short i,j;

    for (j=0; j < TOTALBALLS; j++) {
	for (i=0; i < 3; i++) {
	    balls[j].p[i] += balls[j].d[i];
	    if (fabs(balls[j].p[i]) > ballscale) {
		balls[j].p[i] = (balls[j].p[i] > 0.0) ?
		ballscale : -ballscale;
		balls[j].d[i] = -balls[j].d[i];
	    }
	}
    }
}

static void menu(int value);
static void idle(void);

static void
make_menu(void)
{
    static int main_menu = 0;

    if (main_menu)
	glutDestroyMenu(main_menu);

    main_menu = glutCreateMenu(menu);
    glutAddMenuEntry("bounce", 0);
    glutAddMenuEntry("", 0);
    if (lighton[0])
	glutAddMenuEntry("red light off", 1);
    else
	glutAddMenuEntry("red light on", 1);
    if (lighton[1])
	glutAddMenuEntry("green light off", 2);
    else
	glutAddMenuEntry("green light on", 2);
    if (lighton[2])
	glutAddMenuEntry("blue light off", 3);
    else
	glutAddMenuEntry("blue light on", 3);

    if (freeze)
	glutAddMenuEntry("unfreeze lights", 4);
    else
	glutAddMenuEntry("freeze lights", 4);

    if (normson)
	glutAddMenuEntry("normals off", 7);
    else
	glutAddMenuEntry("normals on", 7);

    if (performance)
	glutAddMenuEntry("frame rate off", 8);
    else
	glutAddMenuEntry("frame rate on", 8);

    if (obj)
    {
	if (objecton)
	    glutAddMenuEntry("object off", 5);
	else
	    glutAddMenuEntry("object on", 5);
	if (spin)
	    glutAddMenuEntry("object spin off", 6);
	else
	    glutAddMenuEntry("object spin on", 6);
    }

    glutAddMenuEntry("", 0);
    glutAddMenuEntry("exit", 9);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static void
menu(int value)
{
    switch(value) {
    case 1:
	if ((lighton[0] = !lighton[0]))
	    glEnable(GL_LIGHT0);
	else
	    glDisable(GL_LIGHT0);
	break;
    case 2:
	if ((lighton[1] = !lighton[1]))
	    glEnable(GL_LIGHT1);
	else
	    glDisable(GL_LIGHT1);
	break;
    case 3:
	if ((lighton[2] = !lighton[2]))
	    glEnable(GL_LIGHT2);
	else
	    glDisable(GL_LIGHT2);
	break;
    case 4:
	freeze =  !freeze;
        if (!freeze || spinning) {
	  glutIdleFunc(idle);
        } else {
	  glutIdleFunc(NULL);
        }
	break;
    case 5:
	if (obj)
	    objecton =  !objecton;
	else
	    exit(1);
	break;
    case 6:
	spin =  !spin;
	break;
    case 7:
        normson = !normson;
        break;
    case 8:
	performance = !performance;
	break;
    case 9:
	exit(0);
	break;
    }
    glutPostWindowRedisplay(window);
    make_menu();
}

/**********************************************************/
/* XXX - The following is a clone of fastobj.c from spin  */
/**********************************************************/


fastobj*
readfastobj(char *name)
{
    FILE *inf;
    fastobj *obj;
    int i;
    int nlongs;
    int magic;
    GLint *ip;
    char filename[512];

    inf = fopen(name,"r");
    if(!inf) {
  	sprintf(filename,"%s",name);
	inf = fopen(filename,"r");
	if(!inf) {
	    fprintf(stderr,"readfast: can't open input file %s\n",name);
	    exit(1);
	}
    }
    fread(&magic,sizeof(int),1,inf);
    if(magic != FASTMAGIC) {
	fprintf(stderr,"readfast: bad magic in object file\n");
	fclose(inf);
	exit(1);
    }
    obj = (fastobj *)malloc(sizeof(fastobj));
    fread(&obj->npoints,sizeof(int),1,inf);
    fread(&obj->colors,sizeof(int),1,inf);

    /*
     * Insure that the data is quad-word aligned and begins on a page
     * boundary.  This shields us from the performance loss which occurs 
     * whenever we try to fetch data which straddles a page boundary  (the OS
     * has to map in the next virtual page and re-start the DMA transfer).
     */
    nlongs = 8 * obj->npoints;
    obj->data = (GLint *) malloc(nlongs*sizeof(int) + 4096);
    obj->data = (GLint *) (((int)(obj->data)) + 0xfff);
    obj->data = (GLint *) (((int)(obj->data)) & 0xfffff000);

    /* XXX Careful, sizeof(GLint) could change from implementation
       to implementation making this file format implementation
       dependent. -mjk */
    for (i = 0, ip = obj->data;  i < nlongs/4;  i++, ip += 4)
	fread(ip, 3 * sizeof(GLint), 1, inf);
    fclose(inf);
    return obj;
}


/*
 * objmaxpoint
 *
 * find the vertex farthest from the origin,
 * so we can set the near and far clipping planes tightly.
 */

#define MAXVERT(v) if ( (len = sqrt(	(*(v))  *  (*(v))  +	       \
					(*(v+1)) * (*(v+1)) +	       \
					(*(v+2)) * (*(v+2)) )) > max)  \
			max = len;

float
objmaxpoint(obj)
fastobj *obj;
{
    register float *p, *end;
    register int npolys;
    register float len;
    register float max = 0.0;

    p = (float *) (obj->data);

    if (obj->colors) {
	npolys = obj->npoints/4;
	while(npolys--) {
	    MAXVERT(p+4);
	    MAXVERT(p+12);
	    MAXVERT(p+20);
	    MAXVERT(p+28);
	    p += 32;
	}
    } else {
	end = p + 8 * obj->npoints;
	while ( p < end) {
	    MAXVERT(p+4);
	    MAXVERT(p+12);
	    MAXVERT(p+20);
	    MAXVERT(p+28);
	    p += 32;
	}
    }

    return max;
}

static void
idle(void)
{
    assert(!freeze || spinning);
    if (!freeze) {
        calcball();
    }
    if (spinning) {
        tbStepAnimation();
    }
    glutPostWindowRedisplay(window);
}

/* When not visible, stop animating.  Restart when visible again. */
static void 
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (!freeze || spinning)
      glutIdleFunc(idle);
  } else {
    if (!freeze || spinning)
      glutIdleFunc(NULL);
  }
}

static void
reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)width/height, EYEZ-2.0, EYEZ+2.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.25, -EYEZ);

    tbReshape(width, height);
    gluiReshape(width, height);
}

void
update_fatt(float value)
{
    fatt = 5 * value;
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, fatt);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, fatt);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, fatt);
    glutPostWindowRedisplay(window);
}

void
update_grid(float value)
{
    wallgrid = WALLGRIDMAX*value;
    if (wallgrid < 1)
	wallgrid = 1;
    initobjects();
    glutPostWindowRedisplay(window);
}

void
spinChange(int state)
{
  spinning = state;
  if (spinning || !freeze) {
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}

int
main(int argc, char **argv)
{
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(64, 64);
    glutInit(&argc, argv);

    if (argc > 1)
    {
	int i;

	for (i=0; argv[1][i] != '/' && argv[1][i] != '\0'; i++);
	if (argv[1][i] != '/')
	{
	    strcpy(ofile, "/usr/demos/data/models/");
	    strcat(ofile, argv[1]);
	}
	else
	    strcpy(ofile, argv[1]);

	if (obj = readfastobj(ofile))
	    objecton = GL_TRUE;
    }

    ballsize = .04;
    ballscale = 1.0 - ballsize;

    initialize();

    make_menu();

    resetballs();

    /* Use local lights for the box */
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    make_menu();
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutDisplayFunc(drawimage);
    glutReshapeFunc(reshape);
    glutVisibilityFunc(visible);

    gluiHorizontalSlider(window, 130, -10, -10, 20,
			 (float)wallgrid/WALLGRIDMAX, update_grid);
    gluiHorizontalSlider(window, 130, -40, -10, 20, fatt/5.0, update_fatt);

    tbInit(GLUT_LEFT_BUTTON);
    tbAnimate(1);
    tbAnimateFunc(spinChange);

    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
