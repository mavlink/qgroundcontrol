
/*
 *    sgiflag.c:
 *
 *  This program displays a waving flag with an SGI logo trimmed out of
 *  it.  The flag is a single nurbs surface (bicubic, bezier). It "waves" 
 *  by making it control point oscillate on a sine wave.
 *
 *  The logo is cut from the flag using a combination of piecewise-linear 
 *  and bezier trim curves.
 *
 *                                    Howard Look - December 1990
 *				      David Blythe - June 1995
 */

#include <GL/glut.h>

#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#include "sgiflag.h"
#include "logopoints.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Knot sequences for cubic bezier surface and trims */
Knot sknots[S_NUMKNOTS] = {0., 0., 0., 0., 1., 1., 1., 1.};
Knot tknots[T_NUMKNOTS] = {0., 0., 0., 0., 1., 1., 1., 1.};
Knot trimknots[S_NUMKNOTS] = {0., 0., 0., 0., 1., 1., 1., 1.};

/* Control points for the flag. The Z values are modified to make it wave */
Point ctlpoints[S_NUMPOINTS][T_NUMPOINTS] = {
    { {0., 3., 0.}, {1., 3., 0.}, {2., 3., 0}, {3., 3., 0.}},
    { {0., 2., 0.}, {1., 2., 0.}, {2., 2., 0}, {3., 2., 0.}},
    { {0., 1., 0.}, {1., 1., 0.}, {2., 1., 0}, {3., 1., 0.}},
    { {0., 0., 0.}, {1., 0., 0.}, {2., 0., 0}, {3., 0., 0.}}
};

/* Trim the whole exterior of the, counter clockwise. Necessary to do
 * internal trimming
 */
TrimCurve *whole;
TrimCurve ccl_path;
TrimPiece ccl_square[] = {
    {
        PWL,
        11,
        {
			{0., 0.}, {.25, 0.}, {.5, 0.}, {.75, 0.}, {1., 0.},
			{1., 1.}, {.75, 1.}, {.5, 1.}, {.25, 1.}, {0., 1.}, {0., 0.}
		}
    }
};

/* Three paths for three parts of the logo */
TrimCurve *path[3];

/* Initial one-third of the logo, centered at origin */
TrimCurve initial_path;
TrimPiece initial_pieces[] = {
        {
            PWL, /* 0 */
            6,
            {{Ax,Ay},{Bx,By},{Cx,Cy},{Dx,Dy},{Ex,Ey},{0.,0.}}
        },
        {
            CURVE, /* 1 */
            4,
            {{0.,0.},{Fx,Fy},{Fx,Fy},{0.,0.}}
        },
        {
            PWL, /* 2 */
            2,
            {{Gx, Gy},{Gx,Gy}}
        },
        {
            CURVE, /* 3 */
            4,
            {{0.,0.},{Gx,Gy},{Gx,Gy},{0.,0.}}
        },
        {
            PWL, /* 4 */
            3,
            {{0., 0.},{Z,Z},{0.,0.}}
        },
        {
            CURVE, /* 5 */
            4,
            {{0.,0.},{-Gx,Gy},{-Gx,Gy},{0.,0.}}
        },
        {
            PWL, /* 6 */
            2,
            {{-Fx, Fy},{-Fx,Fy}}
        },
        {
            CURVE, /* 7 */
            4,
            {{0.,0.},{-Fx,Fy},{-Fx,Fy},{0.,0.}}
        },
        {
            PWL, /* 8 */
            6,
            {{0.,0.},{-Ex,Ey},{-Dx,Dy},{-Cx,Cy},{-Bx,By},{Ax,Ay}}
        }
};

static GLUnurbsObj *nurbsflag;

static GLboolean trimming = GL_TRUE, filled = GL_TRUE, hull = GL_TRUE;
static int mousex = 248, mousey = 259, mstate;

/* Given endpoints of the line a and b, the distance d from point a,
 * returns a new point (in result) that is on that line.
 */
void interp(TrimPoint a, TrimPoint b, GLfloat d, TrimPoint result) {

    GLfloat l;

    l = sqrt((a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]));

    result[0] = a[0] + (b[0] - a[0])*d/l; 
    result[1] = a[1] + (b[1] - a[1])*d/l; 
    
}

/* Given two trim pieces, coerces the endpoint of the first and the
 * start point of the second to be indentical.
 *
 * The two trims must be of opposite types, PWL or CURVE.
 */
void join_trims(TrimPiece *trim1, TrimPiece *trim2, GLfloat radius) {

    int last;
    TrimPoint result;

    last = trim1->points - 1;
    
    if (trim1->type == PWL)
        interp(trim2->point[1], trim1->point[last - 1], radius, result);
    else /* trim1 is CURVE */
        interp(trim1->point[last-1], trim2->point[0], radius, result);

    trim1->point[last][0] = trim2->point[0][0] = result[0];
    trim1->point[last][1] = trim2->point[0][1] = result[1];
}    

/* Translates each point in the trim piece by tx and ty */
void translate_trim(TrimPiece *trim, GLfloat tx, GLfloat ty) {

    int i;

    for (i=0; i<trim->points; i++) {
        trim->point[i][0] += tx;
        trim->point[i][1] += ty;
    }
}        

/* Scales each point in the trim piece by sx and sy */
void scale_trim(TrimPiece *trim, GLfloat sx, GLfloat sy) {

    int i;

    for (i=0; i<trim->points; i++) {
        trim->point[i][0] *= sx;
        trim->point[i][1] *= sy;
    }
}        

/* Rotates each point in the trim piece by angle radians about the origin */
void rotate_trim(TrimPiece *trim, GLfloat angle) {

    int i;
    GLfloat s,c;
    TrimPoint t;

    s = sin(angle);
    c = cos(angle);

    for (i=0; i<trim->points; i++) {
        t[0] = trim->point[i][0];
        t[1] = trim->point[i][1];
        
        trim->point[i][0] = c*t[0] - s*t[1];
        trim->point[i][1] = s*t[0] + c*t[1];
    }
}        

/* Creates storage space for dst and copies the contents of src into dst */
void copy_path(TrimCurve *src, TrimCurve **dst) {

    int i,j;

    *dst = (TrimCurve *) malloc(sizeof(TrimCurve));
    (*dst)->pieces = src->pieces;
    (*dst)->trim = (TrimPiece *) malloc((src->pieces)*sizeof(TrimPiece));

    for(i=0; i < src->pieces; i++) {
        (*dst)->trim[i].type = src->trim[i].type;
        (*dst)->trim[i].points = src->trim[i].points;

        for (j=0; j < src->trim[i].points; j++) {
            (*dst)->trim[i].point[j][0] = src->trim[i].point[j][0];
            (*dst)->trim[i].point[j][1] = src->trim[i].point[j][1];
        }
    }
}    

/* Initializes the outer whole trim plus the three trimming paths 
 * required to trim the logo.
 */
void init_trims(void) {

    int i;

	/* whole outer path, counter clockwise, so NuRB is not trimmed */
    whole = &ccl_path;
    whole->pieces = 1;
    whole->trim = ccl_square;

	/* initial third of logo, centered at origin */
    path[0] = &initial_path;
    path[0]->pieces = ELEMENTS(initial_pieces);
    path[0]->trim = initial_pieces;
    for(i=0; i < path[0]->pieces - 1; i++)
        join_trims(&path[0]->trim[i], &path[0]->trim[i+1], LOGO_RADIUS);

	/* copy other to other two thirds */
    copy_path(path[0],&path[1]);
    copy_path(path[0],&path[2]);

	/* scale and translate first third */
    for (i=0; i<path[0]->pieces; i++) {
        scale_trim(&path[0]->trim[i],0.5,1.0);
        translate_trim(&path[0]->trim[i],0.5,0.52);
    }

	/* rotate, scale and translate second third */
    for (i=0; i<path[1]->pieces; i++) {        
        rotate_trim(&path[1]->trim[i],2.0*M_PI/3.0);
        scale_trim(&path[1]->trim[i],0.5,1.0);
        translate_trim(&path[1]->trim[i],0.49,0.5);
    }

	/* rotate, scale and translate last third */
    for (i=0; i<path[2]->pieces; i++) {        
        rotate_trim(&path[2]->trim[i],2.0*2.0*M_PI/3.0);
        scale_trim(&path[2]->trim[i],0.5,1.0);
        translate_trim(&path[2]->trim[i],0.51,0.5);
    }
}
    
/* Opens a square window, and initializes the window, interesting devices,
 * viewing volume, material, and lights.
 */
static void
initialize(void) {

    GLfloat mat_diffuse[] = { .8, .1, .8, 1. };
    GLfloat mat_specular[] = { .6, .6, .6, 1. };
    GLfloat mat_ambient[] = { .1, .1, .1, 1. };

    glClearColor(.58, .58, .58, 0.);
    glClearDepth(1.);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    gluPerspective(60,1.0,1.0,10.0);
    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0., 0., -6.);
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);

    nurbsflag = gluNewNurbsRenderer();
    gluNurbsProperty(nurbsflag, GLU_SAMPLING_TOLERANCE, 100.0);
    gluNurbsProperty(nurbsflag, GLU_DISPLAY_MODE, GLU_FILL);

    init_trims();
}


/* Draw the nurb, possibly with trimming */
void draw_nurb(GLboolean trimming) {

    static GLfloat angle = 0.0;
    int i,j;


	/* wave the flag by rotating Z coords though a sine wave */
    for (i=1; i<4; i++)
        for (j=0; j<4; j++)
            ctlpoints[i][j][2] = sin((GLfloat)i+angle);

    angle += 0.1;
    
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

        glTranslatef(2.5,-1.0,0.0);
        glScalef(1.5,1.0,1.0);
        glRotatef(90,0.,0.,1.);
        glRotatef(mousey/10.,1.,0.,0.);
        glRotatef(mousex/10.,0.,1.,0.);
        
	gluBeginSurface(nurbsflag);
            gluNurbsSurface(nurbsflag,S_NUMKNOTS, sknots, T_NUMKNOTS, tknots,
                          3 * T_NUMPOINTS, 3,
                          &ctlpoints[0][0][0], T_ORDER, S_ORDER, GL_MAP2_VERTEX_3);
            if (trimming) {
                dotrim(whole);
                dotrim(path[0]);
                dotrim(path[1]);
                dotrim(path[2]);
            }
        gluEndSurface(nurbsflag);
        
	if (hull) draw_hull(ctlpoints);
    
    glPopMatrix();
    
}

/* Draw the convex hull of the control points */
void draw_hull(Point cpoints[S_NUMPOINTS][T_NUMPOINTS]) {

    int s,t;
    
    glDisable(GL_LIGHTING);
    glColor3f(0.,1.,0.);

    glBegin(GL_LINES);
    for (s=0; s<S_NUMPOINTS; s++)
        for (t=0; t<T_NUMPOINTS-1; t++) {
                glVertex3fv(cpoints[s][t]);
                glVertex3fv(cpoints[s][t+1]);
        }
        
    for (t=0; t<T_NUMPOINTS; t++)
        for (s=0; s<S_NUMPOINTS-1; s++) {
                glVertex3fv(cpoints[s][t]);
                glVertex3fv(cpoints[s+1][t]);
        }
    glEnd();
    glEnable(GL_LIGHTING);
}


/* Execute a bgn/endtrim pair for the given trim curve structure. */
void dotrim(TrimCurve *trim_curve) {

    int i;

    gluBeginTrim(nurbsflag);
        for (i=0; i<trim_curve->pieces; i++) {

            if (trim_curve->trim[i].type == PWL) {
                    gluPwlCurve(nurbsflag, trim_curve->trim[i].points,
                              &trim_curve->trim[i].point[0][0],
                              2, GLU_MAP1_TRIM_2);
            } else {
                gluNurbsCurve(nurbsflag, ELEMENTS(trimknots),trimknots,
                            2,&trim_curve->trim[i].point[0][0],
                            trim_curve->trim[i].points, GLU_MAP1_TRIM_2);
            }
        }
    gluEndTrim(nurbsflag);
}


/* ARGSUSED1 */
static void
Key(unsigned char c, int x, int y)
{
    switch(c) {
    case 27: /* Escape */
	exit(0);
	break;
    default:
	break;
    }
}

static void
Button(int button, int down, int x, int y)
{
    if (down) {
	if (button == GLUT_LEFT_BUTTON) {
	    mstate = 1;
	    mousex = x;
	    mousey = y;
	}
    } else {
	if (button == GLUT_LEFT_BUTTON)
	    mstate = 0;
    }
    if (mstate) {
	mousex = x;
	mousey = y;
    }
}

static void
resize(int width, int height)
{
   glViewport(0, 0, width, height);
}


static void
expose(void)
{
    draw_nurb(trimming);
    glutSwapBuffers();
}

static void
idle(void)
{
  glutPostRedisplay();
}

static void
visibility(int state)
{
  if (state == GLUT_VISIBLE) {
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}

static void
Menu(int value)
{
    switch (value) {
    case 1:
	trimming ^= 1; break;
    case 2:
	filled ^= 1;
	gluNurbsProperty(nurbsflag, GLU_DISPLAY_MODE,
			 filled ? GLU_FILL : GLU_OUTLINE_POLYGON);
	break;
    case 3:
	hull ^= 1; break;
    case 4:
	exit(0); break;
    default:
	break;
    }
}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(400, 400);
    glutCreateWindow("NURBS Surface");
    glutDisplayFunc(expose);
    glutReshapeFunc(resize);
    glutKeyboardFunc(Key);
    glutMouseFunc(Button);

    glutCreateMenu(Menu);
    glutAddMenuEntry("Trim", 1);
    glutAddMenuEntry("Fill", 2);
    glutAddMenuEntry("Hull", 3);
    glutAddMenuEntry("Exit", 4);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutVisibilityFunc(visibility);

    initialize();

    glutMainLoop();
    return 0;
}
