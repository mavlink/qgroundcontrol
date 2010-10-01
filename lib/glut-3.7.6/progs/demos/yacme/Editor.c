/*===========================================================================*
 *
 *				YACME
 *			Yet Another ColorMap Editor
 *
 *			Patrick BOUCHAUD - 1993
 *                          SGI Switzerland
 *
 *                    Converted to OpenGL using GLUT
 *
 *===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#include "mallocbis.h"

/* For portability... */
#undef fsqrt
#define fsqrt(_a)     sqrt(_a)

#define	SUP(a,b)	( ((a)>(b)) ? (a) : (b) )
#define	INF(a,b)	( ((a)<(b)) ? (a) : (b) )
#define	ABS(a)		( ((a)<0) ? -(a) : (a) )
#define	POINT		1
#define	CURVE		2
#define	LEFT_TAN	3
#define	RIGHT_TAN	4
#define	MANUAL		1
#define	CONTINUOUS	2

#define	CONSTANT	1
#define	LINEAR		2
#define	POLYNOMIAL	3

#define	TabCmpnt(c,n)	TabCmpnt[(n)*4+(c)]

#define	MAXNDX	(dimlut-1)

#define	WINHEIGHT	381
#define	WINWIDTH	318

#define LUTRATIO	((float)MAXNDX/255.)

#define	LEFT		LUTRATIO*(-5.)
#define	RIGHT		LUTRATIO*(260.)
#define	BOTTOM		LUTRATIO*(-5.)
#define	TOP			LUTRATIO*(315.)
#define	TANLEN		LUTRATIO*(25.)

typedef float Matrix[4][4];

typedef struct	{
	float	x0, y0,
		x1, y1;
} Tangente;

typedef struct UserPointStruct	{

	float		x, y;
	Tangente	tg;
	int			mode;
	float		polynome[4];
	struct		UserPointStruct *next, *last;

} UserPoint;

void ResetCMap(void);
void ApplyCMap(void);
void YACME_makeMenu(void);
extern void invertmat(float from[4][4], float to[4][4]);

static UserPoint *FreePointList = NULL;
#define	newPoint( point ) newItem( point, FreePointList, UserPoint )
#define	freePoint( point ) freeItem( point, FreePointList )

typedef struct	{
	int	type,
		cmpnt,
		ndx;

	UserPoint *upoint;
} PickObject;

struct {
	int leftdown, middledown, rightdown;
} mouse;

void YACME_pick( int mx, int my, PickObject *obj );
void YACME_update( int cmpnt, UserPoint *upoint );
int	YACME_get( unsigned long *table );

void	DeletePoint( int cmpnt, UserPoint *upoint );
int	MovePoint( int, int );
int	MoveTangente( int, int, int );
int	InsertPoint( PickObject *obj );

void	GetPolynome( int mode,
		float a0, float b0, float t0,
		float a1, float b1, float t1,
		float coeff[4] );
void	OrthoTransform( int mx, int my, float *x, float *y );
float	Polynome4( float x, float *polynome );

static int	DrawCurve[4] = { 1, 1, 1, 1, },
		modifiedCurve[4] = { 1, 1, 1, 1 },
		YACME_refresh = MANUAL,
		curCmpnt = 0, curType = 0,
		Mousex, Mousey;

static float *TabCmpnt;
static int	YACME_switch_menu = 0,
	YACME_mode_menu, YACME_edit_menu;
static int YACME_win = 0, W, H , update = 0;
static UserPoint *userPoint[4], *userPointSvg[4], *curPoint = NULL;

static void Redraw( void );
static void Reshape( int, int );
static void Mouse( int, int, int, int );
static void Motion( int, int );
static void Key( unsigned char, int, int );
static void Special( int, int, int );
static void YACME_menuFunc( int );

typedef void (*CallBack)(void);
static CallBack newmapCB, applyCB;
static int dimlut;

/*---------------------------------------------------------------------------*
 *			YACME_init
 *---------------------------------------------------------------------------*/
void
YACME_init(
	int x, int y, int w, int h,
	int dim, float **list,
	CallBack newmapFunc, CallBack applyFunc)
{
	int	i, j;
	UserPoint *userpoint;

	if (YACME_win) return;

	dimlut = dim;
	newmapCB = newmapFunc;
	applyCB = applyFunc;

	if (*list==NULL)
	{
		*list = TabCmpnt = (float *) malloc( 4*dimlut*sizeof(float) );
		for (i=0; i<4; i++)
		{
			newPoint( userPoint[i] );
			userpoint = userPoint[i];
			userpoint->mode = POLYNOMIAL;
			userpoint->x = 0.;
			userpoint->y = 0.;
			userpoint->last	= NULL;
			userpoint->tg.x0 = 0.;
			userpoint->tg.y0 = 0.;
			userpoint->tg.x1 = TANLEN;
			userpoint->tg.y1 = 0.;
	
			newPoint( userpoint->next );
			userpoint = userpoint->next;
			userpoint->mode = POLYNOMIAL;
			userpoint->x = (float)MAXNDX;
			userpoint->y = (float)MAXNDX;
			userpoint->last	= userPoint[i];
			userpoint->next	= NULL;
			userpoint->tg.x0 = (float)(MAXNDX-TANLEN);
			userpoint->tg.y0 = (float)MAXNDX;
			userpoint->tg.x1 = (float)MAXNDX;
			userpoint->tg.y1 = (float)MAXNDX;
		}
	}
	else
	{
		UserPoint *upoint[4], *lastupoint[4]={0,0,0,0};

		TabCmpnt = *list;
		for (i=0; i<dim; i++)
		{
			for (j=0; j<4; j++)
			{
				if (i==0)
				{
					newPoint( userPoint[j] );
					upoint[j] = userPoint[j];
				}
				else
				{
					newPoint( upoint[j]->next );
					upoint[j] = upoint[j]->next;
				}
				upoint[j]->next = NULL;
				upoint[j]->last = lastupoint[j];
				upoint[j]->mode = LINEAR;
				upoint[j]->x = (float)i;
				upoint[j]->y = (float)MAXNDX*TabCmpnt(j,i);
				if (i==0)
				{
					upoint[j]->tg.x0 = 0.;
					upoint[j]->tg.y0 = 0.;
					upoint[j]->tg.x1 = TANLEN;
					upoint[j]->tg.y1 = 0.;
				}
				else
				{
					float	slope = upoint[j]->y - upoint[j]->last->y,
							dx = TANLEN/fsqrt(slope*slope + 1),
							dy = slope*dx;
					upoint[j]->tg.x0 = upoint[j]->x - dx;
					upoint[j]->tg.y0 = upoint[j]->y - dy;
					upoint[j]->tg.x1 = upoint[j]->x + dx;
					upoint[j]->tg.y1 = upoint[j]->y + dy;
				}

				lastupoint[j] = upoint[j];
			}
		}
	}

/*
	TabComponents update
	--------------------
*/
	for (i=0; i<4; i++)
	{
		for (userpoint=userPoint[i]; userpoint!=NULL; userpoint=userpoint->next)
			YACME_update( i, userpoint );
	}
	ApplyCMap();

	if (!YACME_win)
	{
		glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
		glutInitWindowPosition( x, y );
		glutInitWindowSize( w, h );
		YACME_win = glutCreateWindow( "LUT editor" );
		glutDisplayFunc( Redraw );
		glutReshapeFunc( Reshape );
		glutMouseFunc( Mouse );
		glutMotionFunc( Motion );
		glutPassiveMotionFunc( Motion );
		glutSpecialFunc( Special );
		glutKeyboardFunc( Key );
		
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( LEFT, RIGHT, BOTTOM, TOP, -1., 1. );

		glClearColor( 0.66, 0.66, 0.66, 1. );
		glClear( GL_COLOR_BUFFER_BIT );
		glShadeModel( GL_FLAT );
		glutSwapBuffers();

		YACME_makeMenu();
	}
}

/*---------------------------------------------------------------------------*
 *	Key
 *---------------------------------------------------------------------------*/
/* ARGSUSED1 */
static void
Key( unsigned char key, int x, int y )
{
	UserPoint *previousPoint;
	switch (key)
	{
		case 27:	/* Escape */
			exit(0);
			break;

		case 127:	/* BackSpace */
		case 8:		/* Delete */
			if (!curPoint) return;
			previousPoint = (curPoint->last ? curPoint->last : userPoint[curCmpnt]);
			DeletePoint( curCmpnt, curPoint );
			modifiedCurve[curCmpnt] = 1;
			update = 1;
			curPoint = previousPoint;
			break;
	}
	glutPostRedisplay();
}

/*---------------------------------------------------------------------------*
 *      Special
 *---------------------------------------------------------------------------*/
/* ARGSUSED1 */
static void
Special( int key, int xx, int yy )
{
	int x, y, minx, maxx;
	float dx, dy;
	if (curPoint == NULL ) return;
	if (curPoint->next == NULL) return;
	if (curPoint->last == NULL) return;

	x = (int)curPoint->x;
	y = (int)curPoint->y;

	minx = (int)curPoint->last->x; minx++;
	maxx = (int)curPoint->next->x; maxx--;

	switch (key)
	{
		case GLUT_KEY_LEFT:
			if (x > minx) x--;
			break;

		case GLUT_KEY_RIGHT:
			if (x < maxx) x++;
			break;

		case GLUT_KEY_UP:
			if (y < MAXNDX) y++;
			break;

		case GLUT_KEY_DOWN:
			if (y > 0) y--;
			break;
	}

	dx = (float)x - curPoint->x;
	curPoint->x += dx;
	curPoint->tg.x0 += dx;
	curPoint->tg.x1 += dx;

	dy = (float)y - curPoint->y;
	curPoint->y = y;
	curPoint->tg.y0 += dy;
	curPoint->tg.y1 += dy;

	update = 1;
	glutPostRedisplay();
}

/*---------------------------------------------------------------------------*
 *	Reshape
 *---------------------------------------------------------------------------*/
static void
Reshape( int w, int h )
{
	glViewport( 0, 0, w, h );
	W = w;
	H = h;
	glutPostRedisplay();
}

/*---------------------------------------------------------------------------*
 *	Mouse
 *---------------------------------------------------------------------------*/
static void
Mouse( int button, int state, int x, int y )
{
	PickObject	object;

	y = H-y;
	switch (button)
	{
		case GLUT_RIGHT_BUTTON:
			mouse.rightdown = (state==GLUT_DOWN);
			break;

		case GLUT_MIDDLE_BUTTON:
			mouse.middledown = (state==GLUT_DOWN);
			break;

		case GLUT_LEFT_BUTTON:
			mouse.leftdown = (state==GLUT_DOWN);
			if (mouse.leftdown)
			{
				YACME_pick( x, y, &object );
				curType = object.type;
				switch ( object.type )
				{
					case CURVE:
						if ((curType = InsertPoint( &object ) != 0))
						{
							curPoint = object.upoint;
							curCmpnt = object.cmpnt;
						}
						else curPoint = NULL;
						break;

					case POINT:
					case LEFT_TAN:
					case RIGHT_TAN:
						curPoint = object.upoint;
						curCmpnt = object.cmpnt;
						break;

					default:
						curPoint = NULL;
			}
			break;
		}
	}
	glutPostRedisplay();
}

/*---------------------------------------------------------------------------*
 *	Motion
 *---------------------------------------------------------------------------*/
static void
Motion( int x, int y )
{
	Mousex = x;
	Mousey = H-y;
	if (mouse.leftdown)
	{
		switch (curType)
		{
			case POINT:
				update = MovePoint( Mousex, Mousey );
				break;

			case LEFT_TAN:
				update = MoveTangente( -1, Mousex, Mousey );
				break;

			case RIGHT_TAN:
				update = MoveTangente( 1, Mousex, Mousey );
				break;
		}
	}
	glutPostRedisplay();
}

/*---------------------------------------------------------------------------*
 *			YACME_makeMenu
 *---------------------------------------------------------------------------*/

#define	CONTINUOUS_ITEM	5
#define	MANUAL_ITEM		4
#define	APPLY_ITEM		3
#define	RESET_ITEM		2
#define	CLOSE_ITEM		1

#define	RED_ITEM	10
#define	GREEN_ITEM	11
#define	BLUE_ITEM	12
#define	ALPHA_ITEM	13

#define	CONSTANT_ITEM	22
#define	LINEAR_ITEM		21
#define	POLYNOM_ITEM	20

#define	EDIT_RED_ITEM	30
#define	EDIT_GREEN_ITEM	31
#define	EDIT_BLUE_ITEM	32
#define	EDIT_ALPHA_ITEM	33

void
YACME_makeMenu(void)
{
	YACME_edit_menu = glutCreateMenu( YACME_menuFunc );
	glutAddMenuEntry( "red", EDIT_RED_ITEM );
	glutAddMenuEntry( "green", EDIT_GREEN_ITEM );
	glutAddMenuEntry( "blue", EDIT_BLUE_ITEM );
	glutAddMenuEntry( "alpha", EDIT_ALPHA_ITEM );

	YACME_mode_menu = glutCreateMenu( YACME_menuFunc );
	glutAddMenuEntry( "Constant  ", CONSTANT_ITEM );
	glutAddMenuEntry( "Linear    ", LINEAR_ITEM );
	glutAddMenuEntry( "Polynomial", POLYNOM_ITEM );

	YACME_switch_menu = glutCreateMenu( YACME_menuFunc );
	glutAddMenuEntry( "Red    on", RED_ITEM );
	glutAddMenuEntry( "Green on", GREEN_ITEM );
	glutAddMenuEntry( "Blue   on", BLUE_ITEM );
	glutAddMenuEntry( "Alpha on", ALPHA_ITEM );

	glutCreateMenu( YACME_menuFunc );
	glutAddMenuEntry( "Manual    ", MANUAL_ITEM );
	glutAddMenuEntry( "Apply", APPLY_ITEM );
	glutAddMenuEntry( "Reset", RESET_ITEM );
	glutAddMenuEntry( "Close", CLOSE_ITEM );
	glutAddSubMenu(   "Mode... ", YACME_mode_menu );
	glutAddSubMenu(   "Switch... ", YACME_switch_menu );
	glutAddSubMenu(   "Edit... ", YACME_edit_menu );

	glutAttachMenu( GLUT_RIGHT_BUTTON );
}

/*---------------------------------------------------------------------------*
 *	YACME_menuFunc
 *---------------------------------------------------------------------------*/
static void
YACME_menuFunc( int item )
{
	switch( item )
	{
		case CLOSE_ITEM:
			break;

		case RESET_ITEM:
			if (applyCB) (*applyCB)();
			else if (newmapCB) (*newmapCB)();
			ResetCMap();
			curPoint = NULL;
			break;

		case APPLY_ITEM:
			if (applyCB) (*applyCB)();
			else if (newmapCB) (*newmapCB)();
			ApplyCMap();
			break;

		case MANUAL_ITEM:	/* switch to continuous */
			glutChangeToMenuEntry( 1, "Continuous", CONTINUOUS_ITEM );
			YACME_refresh = CONTINUOUS;
			break;

		case CONTINUOUS_ITEM: /* switch to manual */
			glutChangeToMenuEntry( 1, "Manual    ", MANUAL_ITEM );
			if (newmapCB) (*newmapCB)();
			ApplyCMap();
			YACME_refresh = MANUAL;
			break;

		case RED_ITEM:
			DrawCurve[0] = !DrawCurve[0];
			if (DrawCurve[0])
				glutChangeToMenuEntry( 1, "Red     on", RED_ITEM );
			else
				glutChangeToMenuEntry( 1, "Red    off", RED_ITEM );
			curPoint = NULL;
			break;

		case GREEN_ITEM:
			DrawCurve[1] = !DrawCurve[1];
			if (DrawCurve[1])
				glutChangeToMenuEntry( 2, "Green on", GREEN_ITEM );
			else
				glutChangeToMenuEntry( 2, "Green off", GREEN_ITEM );
			curPoint = NULL;
			break;

		case BLUE_ITEM:
			DrawCurve[2] = !DrawCurve[2];
			if (DrawCurve[2])
				glutChangeToMenuEntry( 3, "Blue   on", BLUE_ITEM );
			else
				glutChangeToMenuEntry( 3, "Blue   off", BLUE_ITEM );
			curPoint = NULL;
			break;

		case ALPHA_ITEM:
			DrawCurve[3] = !DrawCurve[3];
			if (DrawCurve[3])
				glutChangeToMenuEntry( 4, "Alpha on", ALPHA_ITEM );
			else
				glutChangeToMenuEntry( 4, "Alpha off", ALPHA_ITEM );
			curPoint = NULL;
			break;

		case CONSTANT_ITEM:
			if (curPoint != NULL)
			{
				update = 1;
				curPoint->mode = CONSTANT;
			}
			break;

		case LINEAR_ITEM:
			if (curPoint != NULL)
			{
				update = 1;
				curPoint->mode = LINEAR;
			}
			break;

		case POLYNOM_ITEM:
			if (curPoint != NULL)
			{
				update = 1;
				curPoint->mode = POLYNOMIAL;
			}
			break;

		case EDIT_RED_ITEM:
		case EDIT_GREEN_ITEM:
		case EDIT_BLUE_ITEM:
		case EDIT_ALPHA_ITEM:
			curCmpnt = item-30;
			curPoint = userPoint[curCmpnt];
			break;
	}
	glutPostRedisplay();
}

/*---------------------------------------------------------------------------*
 *	output
 *---------------------------------------------------------------------------*/
static void
output( float x, float y, char *string )
{
	int len, i;

	glRasterPos2f(x, y);
	len = (int) strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, string[i]);
	}
}

/*---------------------------------------------------------------------------*
 *	Redraw
 *---------------------------------------------------------------------------*/
static void
Redraw( void )
{
	int	i, j, ndx, cmpnt, priority[4];
	Tangente *tg;
	UserPoint *upoint;
	char string[256];

	if (update)
	{
		modifiedCurve[curCmpnt] = 1;
		YACME_update( curCmpnt, curPoint );
		if ( newmapCB && (YACME_refresh == CONTINUOUS))
			(*newmapCB)();
		update = 0;
	}

	glClear( GL_COLOR_BUFFER_BIT );

/*
	Barettes representant la table des couleurs
	-------------------------------------------
*/
	glBegin( GL_TRIANGLE_STRIP );
		glColor3f(TabCmpnt(0,0), TabCmpnt(1,0), TabCmpnt(2,0));
		glVertex2f( 0., 270.*LUTRATIO );
		glVertex2f( 0., 310.*LUTRATIO );
		glVertex2f( .5, 270.*LUTRATIO );
		glVertex2f( .5, 310.*LUTRATIO );
	glEnd();
	for ( i=1; i<MAXNDX; i++ )
	{
		float x=(float)i - .5;
		glBegin( GL_TRIANGLE_STRIP );
			glColor3f(TabCmpnt(0,i), TabCmpnt(1,i), TabCmpnt(2,i));
			glVertex2f( x, 270.*LUTRATIO );
		  	glVertex2f( x, 310.*LUTRATIO );
			glVertex2f( x+1., 270.*LUTRATIO );
			glVertex2f( x+1., 310.*LUTRATIO );
		glEnd();
 	}
	glBegin( GL_TRIANGLE_STRIP );
		glColor3f(TabCmpnt(0,MAXNDX), TabCmpnt(1,MAXNDX), TabCmpnt(2,MAXNDX));
		glVertex2f( (float)MAXNDX-.5, 270.*LUTRATIO );
		glVertex2f( (float)MAXNDX-.5, 310.*LUTRATIO );
		glVertex2f( (float)MAXNDX, 270.*LUTRATIO );
		glVertex2f( (float)MAXNDX, 310.*LUTRATIO );
	glEnd();

/*
	Graphe y = cpste(x)
	-------------------
*/ 
	glColor3f(0.,0.,0.);
	glBegin( GL_TRIANGLE_STRIP );
		glVertex2f( 0., 0. );
		glVertex2f( (float) MAXNDX, 0. );
		glVertex2f( 0., (float) MAXNDX );
		glVertex2f( (float) MAXNDX, (float) MAXNDX );
	glEnd();

/*
	Curves y = cpste(x)
	--------------------
*/
	for ( i=0; i<curCmpnt; i++)
	{
		priority[i] = i;
	}
	for ( i=curCmpnt+1; i<4; i++ ) {
		priority[i-1] = i;
	}
	priority[3] = curCmpnt;
 
	cmpnt = priority[0];
	for ( i=0; i<4; i++, cmpnt=priority[i] ) if ( DrawCurve[cmpnt] )
	{

		switch (cmpnt)
		{
			case 3: glColor3f(1.,1.,1.); break;
			case 2: glColor3f(0.,0.,1.); break;
			case 1: glColor3f(0.,1.,0.); break;
			case 0: glColor3f(1.,0.,0.); break;
		}

		for ( j=0; j<MAXNDX; j++ )
		{
			glBegin( GL_LINES );
				glVertex2f( (float)j, (float)MAXNDX*TabCmpnt(cmpnt,j) );
				glVertex2f( (float)(j+1), (float)MAXNDX*TabCmpnt(cmpnt,j+1) );
			glEnd();
		}
	}

	if ( curPoint != NULL )
	{
		float h;
/*
		user defined points
		-------------------
*/
		upoint = userPoint[curCmpnt];
		do {
/*
			Point
			-----
*/
#define	DIMPOINT (2.*LUTRATIO)

			glBegin( GL_TRIANGLE_STRIP );
				glVertex2f( upoint->x-DIMPOINT, upoint->y-DIMPOINT );
				glVertex2f( upoint->x-DIMPOINT, upoint->y+DIMPOINT );
				glVertex2f( upoint->x+DIMPOINT, upoint->y-DIMPOINT );
				glVertex2f( upoint->x+DIMPOINT, upoint->y+DIMPOINT );
			glEnd();
/*
			Tangente
			--------
*/
			tg = &upoint->tg;
			if (upoint->last ? upoint->last->mode == POLYNOMIAL : 0)
			{
				glBegin( GL_LINES );
					glVertex2f( tg->x0, tg->y0 );
					glVertex2f( upoint->x, upoint->y );
				glEnd();
				glBegin( GL_LINE_LOOP );
					glVertex2f( tg->x0-DIMPOINT, tg->y0-DIMPOINT );
					glVertex2f( tg->x0-DIMPOINT, tg->y0+DIMPOINT );
					glVertex2f( tg->x0+DIMPOINT, tg->y0+DIMPOINT );
					glVertex2f( tg->x0+DIMPOINT, tg->y0-DIMPOINT );
				glEnd();
			}
			if (upoint->mode == POLYNOMIAL)
			{
				glBegin( GL_LINES );
					glVertex2f( upoint->x, upoint->y );
					glVertex2f( tg->x1, tg->y1 );
				glEnd();
				glBegin( GL_LINE_LOOP );
					glVertex2f( tg->x1-DIMPOINT, tg->y1-DIMPOINT );
					glVertex2f( tg->x1-DIMPOINT, tg->y1+DIMPOINT );
					glVertex2f( tg->x1+DIMPOINT, tg->y1+DIMPOINT );
					glVertex2f( tg->x1+DIMPOINT, tg->y1-DIMPOINT );
				glEnd();
			}
			
		} while ( (upoint=upoint->next) != NULL );

/*
		Highlight current point
		-----------------------
*/
		ndx = curPoint->x;
		h = curPoint->y;
		switch (curCmpnt)
		{
			case 3: glColor3f(1.,1.,1.); break;
			case 2: glColor3f(0.,0.,1.); break;
			case 1: glColor3f(0.,1.,0.); break;
			case 0: glColor3f(1.,0.,0.); break;
		}
		glBegin( GL_LINE_LOOP );
			glVertex2f( (float)ndx-2.*DIMPOINT, h-2.*DIMPOINT );
			glVertex2f( (float)ndx-2.*DIMPOINT, h+2.*DIMPOINT );
			glVertex2f( (float)ndx+2.*DIMPOINT, h+2.*DIMPOINT );
			glVertex2f( (float)ndx+2.*DIMPOINT, h-2.*DIMPOINT );
		glEnd();
	}
	else
	{
		float x, y;
		OrthoTransform( Mousex, Mousey, &x, &y );
		ndx = (int) SUP( INF(x,(float)MAXNDX), 0 );
	}

/*
	Coordinates text string
	-----------------------
*/
	glColor3f( 0., 0., 0. );
	sprintf( string,
		"%.5d: rgba %.3f %.3f %.3f %.3f",
		ndx,
		TabCmpnt(0,ndx),
		TabCmpnt(1,ndx),
		TabCmpnt(2,ndx),
		TabCmpnt(3,ndx)
	);
	output( LUTRATIO, 257.*LUTRATIO, string );

	glutSwapBuffers();
}

/*---------------------------------------------------------------------------*
 *			YACME_pick
 *---------------------------------------------------------------------------*/
void
YACME_pick( int mousex, int mousey, PickObject *obj )
{
 int	cmpnt, i, priority[4];
 float	x, y, val;
 Tangente *tg;
 UserPoint *upoint;
 float pickrad;

 obj->type = -1;
 OrthoTransform( mousex, mousey, &x, &y );

/*
 On travaille en priorite sur les objets definis par l'utilisateur
 -----------------------------------------------------------------
*/
 for ( i=0; i<curCmpnt; i++) {
	priority[i] = i;
 }
 for ( i=curCmpnt+1; i<4; i++ ) {
	priority[i-1] = i;
 }
 priority[3] = curCmpnt;
 
 pickrad = 3.*LUTRATIO;
 cmpnt = priority[3];
 for ( i=3; i>=0; i--, cmpnt=priority[i] ) if ( DrawCurve[cmpnt] ) {

	upoint = userPoint[cmpnt];
	while ( upoint != NULL ) {
/*
		On cherche le POINT
*/
		if ( ABS(y-upoint->y) < pickrad && ABS(x-upoint->x) < pickrad ) {
			obj->upoint	= upoint;
			obj->cmpnt	= cmpnt;
			obj->type	= POINT;
			return;
		}
/*
		On cherche la demi-tangente GAUCHE
*/
		tg = &upoint->tg;
		if ( ABS(x-tg->x0) < pickrad && ABS(y-tg->y0) < pickrad ) {
			obj->upoint	= upoint;
			obj->cmpnt	= cmpnt;
			obj->type	= LEFT_TAN;
			return;
		}
/*
		On cherche la demi-tangente DROITE
*/
		if ( ABS(x-tg->x1) < pickrad && ABS(y-tg->y1) < pickrad ) {
			obj->upoint	= upoint;
			obj->cmpnt	= cmpnt;
			obj->type	= RIGHT_TAN;
			return;
		}
		upoint = upoint->next;
	}
/*
	Puis on cherche les coordonnees sur la CURVE
	(On considere la precision suffisante pour le picking suivant l'axe X)
	Remarque : on ne sort pas en ce cas, car la priorite est sur les objets
	utilisateurs.
*/
	upoint = userPoint[cmpnt];
	while ( upoint->next->x < x ) {
		upoint = upoint->next;
		if ( upoint->next == NULL ) return;
	}

	val = INF( SUP( Polynome4(x,upoint->polynome), 0.), (float)MAXNDX );
	if ( ABS(y-val) < pickrad ) {
		obj->cmpnt	= cmpnt;
		obj->ndx	= ((x-(int)x)<.5) ? (int)x : (int)x+1;
		obj->type	= CURVE;
		return;
	}

 }
}

/*---------------------------------------------------------------------------*
 *			YACME_update
 *---------------------------------------------------------------------------*/
void
YACME_update( int cmpnt, UserPoint *upoint )
{
 int	i;
 float	val, t0, t1;
 Tangente *tg;

 tg = &upoint->tg;
 if ( tg->x1 != tg->x0 ) {
	t0 = (tg->y1-tg->y0) / (tg->x1-tg->x0);
 } else {
	t0 = (tg->y1-tg->y0)*10000.;
 }

 TabCmpnt(cmpnt,(int) upoint->x) = upoint->y/(float)MAXNDX;
 if ( upoint->last != NULL ) {

	tg = &upoint->last->tg;
	if ( tg->x1 != tg->x0 ) {
		t1 = (tg->y1-tg->y0) / (tg->x1-tg->x0);
	} else {
		t1 = (tg->y1-tg->y0)*10000.;
	}

	GetPolynome(
		upoint->last->mode,
		upoint->last->x,
		upoint->last->y,
		t1,
		upoint->x,
		upoint->y,
		t0,
		upoint->last->polynome
	);

	for ( i=1+(int) upoint->last->x; i<(int) upoint->x; i++ ) {
		val = Polynome4((float) i, upoint->last->polynome)/(float)MAXNDX;
		TabCmpnt(cmpnt,i) = INF( SUP(val,0.), 1. );
	}
 }

 if ( upoint->next != NULL ) {

	tg = &upoint->next->tg;
	if ( tg->x1 != tg->x0 ) {
		t1 = (tg->y1-tg->y0) / (tg->x1-tg->x0);
	} else {
		t1 = (tg->y1-tg->y0)*10000.;
	}

	GetPolynome(
		upoint->mode,
		upoint->x,
		upoint->y,
		t0,
		upoint->next->x,
		upoint->next->y,
		t1,
		upoint->polynome
	);

	for ( i=1+(int) upoint->x; i<(int) upoint->next->x; i++ ) {
		val = Polynome4( (float) i, upoint->polynome )/(float)MAXNDX;
		TabCmpnt(cmpnt,i) = INF( SUP(val,0.), 1. );
	}
	TabCmpnt(cmpnt,(int) upoint->next->x) = upoint->next->y/(float)MAXNDX;
 }
}

/*---------------------------------------------------------------------------*
 *			freePointList
 *---------------------------------------------------------------------------*/
static void
freePointList( UserPoint *upoint )
{
	while (upoint != NULL)
	{
		UserPoint *unext = upoint->next;
		freePoint( upoint );
		upoint = unext;
	}
}

/*---------------------------------------------------------------------------*
 *			clonePointList
 *---------------------------------------------------------------------------*/
static UserPoint *
clonePointList( UserPoint *base )
{
	UserPoint *clone, *upoint;

	if (base == NULL) return NULL;

	newPoint( clone );
	upoint = clone;

	memcpy( upoint, base, sizeof(UserPoint) );
	upoint->last = NULL;
	upoint->next = NULL;

	while (base->next != NULL)
	{
		base = base->next;
		newPoint( upoint->next );
		memcpy( upoint->next, base, sizeof(UserPoint) );
		upoint->next->last = upoint;
		upoint->next->next = NULL;
		upoint = upoint->next;
	}

	return clone;
}

/*---------------------------------------------------------------------------*
 *			ApplyCMap
 *---------------------------------------------------------------------------*/
void
ApplyCMap(void)
{
	int	i;

	for (i=0; i<4; i++) if ( modifiedCurve[i] )
	{
		freePointList( userPointSvg[i] );
		userPointSvg[i] = clonePointList( userPoint[i] );
		modifiedCurve[i] = 0;
	}
}

/*---------------------------------------------------------------------------*
 *			ResetCMap
 *---------------------------------------------------------------------------*/
void
ResetCMap(void)
{
	int	i;
	UserPoint *upoint;

	for (i=0; i<4; i++) if ( modifiedCurve[i] )
	{
		freePointList( userPoint[i] );
		userPoint[i] = clonePointList( userPointSvg[i] );
		modifiedCurve[i] = 0;

		for (upoint=userPoint[i]; upoint!=NULL; upoint=upoint->next )
			YACME_update( i, upoint );
	}
}

/*---------------------------------------------------------------------------*
 *			InsertPoint
 *---------------------------------------------------------------------------*/
int
InsertPoint( PickObject *obj )
{
 float	dx, dy, slope, coeff[4];
 int	i;
 Tangente *tg;
 UserPoint *upoint, *userpoint;

 upoint = userPoint[obj->cmpnt];
 while ( (int) upoint->next->x <= obj->ndx ) {
	upoint = upoint->next;
	if ( upoint->next == NULL ) return 0;
 }
 if ( (int) upoint->x == obj->ndx ) {
	obj->upoint = upoint;
	return POINT;
 }

 newPoint( userpoint );

 userpoint->x = (float) obj->ndx;
 userpoint->y = Polynome4( obj->ndx, upoint->polynome );
 userpoint->y = SUP( INF(userpoint->y, (float)MAXNDX), 0. );
 userpoint->mode = upoint->mode;
 for ( i=0; i<4; i++ ) {
	userpoint->polynome[i] = upoint->polynome[i];
 }
 userpoint->next = upoint->next;
 userpoint->last = upoint;
 upoint->next = userpoint;
 userpoint->next->last = userpoint;

/*
 calcul de slope = P'(index)
 ---------------------------
*/

 if ( userpoint->y == (float)MAXNDX || userpoint->y == 0. ) {
	dx = 25.;
	dy = 0.;
 } else {
	coeff[0] = userpoint->polynome[0]*3.;
	coeff[1] = userpoint->polynome[1]*2.;
	coeff[2] = userpoint->polynome[2];
	slope = coeff[0];
	for (i=1; i<3; i++) {
		slope *= (float) obj->ndx;	
		slope += coeff[i];
	}
/*
	Rappel :
	cos( arctg(x) ) = 1/sqr( 1 + x^2 );
	sin( arctg(x) ) = |x|/sqr( 1 + x^2 );
*/
	dx = 25. / fsqrt( slope*slope + 1. );
	dy = dx*slope;
 }
 dx *= LUTRATIO;
 dy *= LUTRATIO;

 tg = &userpoint->tg;
 tg->x0 = userpoint->x - dx;
 tg->x1 = userpoint->x + dx;
 tg->y0 = userpoint->y - dy;
 tg->y1 = userpoint->y + dy;

 obj->upoint = userpoint;
 return POINT;
}

/*---------------------------------------------------------------------------*
 *			DeletePoint
 *---------------------------------------------------------------------------*/
void
DeletePoint( int cmpnt, UserPoint *upoint )
{
	if ( upoint == NULL || upoint->last == NULL || upoint->next == NULL )
		return;
	upoint->last->next = upoint->next;
	upoint->next->last = upoint->last;
	YACME_update( cmpnt, upoint->last );
	freePoint( upoint );
}

/*---------------------------------------------------------------------------*
 *			MovePoint
 *---------------------------------------------------------------------------*/
int
MovePoint( int mousex, int mousey )
{
	float	x, y, dx, dy, minx, maxx;

	OrthoTransform( mousex, mousey, &x, &y );

	y = SUP( INF(y,(float)MAXNDX), 0. );
	x = ((x-(int)x)<.5) ? (float)(int)x : (float)(int)(x+1);
	y = ((y-(int)y)<.5) ? (float)(int)y : (float)(int)(y+1);
	if ( curPoint->next != NULL && curPoint->last!= NULL )
	{
		minx = curPoint->last->x + 1.;
		maxx = curPoint->next->x - 1.;
		x = SUP( INF(x, maxx), minx );

		dx = (float)x - curPoint->x;
		curPoint->x = (float) x;
		curPoint->tg.x0 += (float) dx;
		curPoint->tg.x1 += (float) dx;
	}
	dy = (float)y - curPoint->y;
	curPoint->y = (float)y;
	curPoint->tg.y0 += (float) dy;
	curPoint->tg.y1 += (float) dy;

	return 1;
}

/*---------------------------------------------------------------------------*
 *			MoveTangente
 *---------------------------------------------------------------------------*/
int
MoveTangente( int sens, int mousex, int mousey )
{
	float	x, y, dx, dy, slope;
	Tangente *tg;

	OrthoTransform( mousex, mousey, &x, &y );

	if ( sens == -1 )
	{
		x = INF(x, curPoint->x);
	}
	else
	{
		x = SUP(x, curPoint->x);
	}
	if ( x != curPoint->x )
	{
		slope =		(curPoint->y - y);
		slope /=	(curPoint->x - x);
		dx = 25. / fsqrt( slope*slope + 1. );
		dy = dx*slope;
	}
	else
	{
		dx = 0.;
		dy = (curPoint->y < y) ? sens*25. : -sens*25.;
	}

	dx *= LUTRATIO;
	dy *= LUTRATIO;

	tg = &curPoint->tg;
	tg->x0 = curPoint->x;
	tg->y0 = curPoint->y;
	if ( curPoint->last != NULL ) {
		tg->x0 -= dx;
		tg->y0 -= dy;
	}
	tg->x1 = curPoint->x;
	tg->y1 = curPoint->y;
	if ( curPoint->next != NULL )
	{
		tg->x1 += dx;
		tg->y1 += dy;
	}

	return 1;
}

/*---------------------------------------------------------------------------*
 *			GetPolynome
 *---------------------------------------------------------------------------*/
/*
	we want a polynom P(x) = Ax^3 + Bx^2 + Cx + D
	so that : P(a0) = b0, P'(a0) = t0, P(a1) = b1, P'(a1) = t1 
	i.e.

	| a0^3		a0^2	a0	1 | |A|	  |b0|
	| a1^3		a1^2	a1	1 | |B| = |b1|
	| 3a0^2		2a0		1	0 | |C|	  |t0|
	| 3a1^2		2a1		1	0 | |D|   |t1|
*/
void
GetPolynome(
	int mode,
	float a0, float b0, float t0,
	float a1, float b1, float t1,
	float coeff[4] )
{
	Matrix	mat, INV_mat;
	int	i;
	float	val;

	switch (mode)
	{
		case CONSTANT:
/*			we want P(x) = b0 */
			coeff[0] = 0.;
			coeff[1] = 0.;
			coeff[2] = 0.;
			coeff[3] = b0;
			break;

		case LINEAR:
/*
			we want P(x) = b0 + (x-a0)/(a1-a0)*(b1-b0)
			i.e. P(x) = x*[(b1-b0)/(a1-a0)] + b0 - a0*[(b1-b0)/(a1-a0)]
*/
			val = (b1-b0)/(a1-a0);
			coeff[0] = 0.;
			coeff[1] = 0.;
			coeff[2] = val;
			coeff[3] = b0 - a0*val;
			break;

		case POLYNOMIAL:
/*
			we want a polynom P(x) = Ax^3 + Bx^2 + Cx + D
			so that : P(a0) = b0, P'(a0) = t0, P(a1) = b1, P'(a1) = t1 
			i.e.
		
			| a0^3		a0^2	a0	1 | |A|	  |b0|
			| a1^3		a1^2	a1	1 | |B| = |b1|
			| 3a0^2		2a0		1	0 | |C|	  |t0|
			| 3a1^2		2a1		1	0 | |D|   |t1|
*/
			val = 1.;
			for (i=3; i>=0; i--)
			{
				mat[0][i] = val;
				val *= a0;
			}
		
			val = 1.;
			for (i=3; i>=0; i--)
			{
				mat[1][i] = val;
				val *= a1;
			}
		
			mat[2][3] = 0.;
			mat[2][2] = 1.;
			mat[2][1] = 2.*a0;
			mat[2][0] = 3.*a0*a0;
		
			mat[3][3] = 0.;
			mat[3][2] = 1.;
			mat[3][1] = 2.*a1;
			mat[3][0] = 3.*a1*a1;
		
			invertmat( mat, INV_mat );
		
			for (i=0; i<4; i++)
				coeff[i] = INV_mat[i][0]*b0 +
					INV_mat[i][1]*b1 +
					INV_mat[i][2]*t0 +
					INV_mat[i][3]*t1;
			break;
	}
}

/*---------------------------------------------------------------------------*
 *			OrthoTransform
 *---------------------------------------------------------------------------*/
void
OrthoTransform( int mx, int my, float *x, float *y )
{
 float	xf, yf;

 xf =	(float) mx;
 xf /=	(float) W;
 xf *=	(float) (RIGHT - LEFT);
 xf +=	(float) LEFT;

 yf =	(float) my;
 yf /=	(float) H;
 yf *=	(float) (TOP - BOTTOM);
 yf +=	(float) BOTTOM;

 *x = xf;
 *y = yf;
}

/*---------------------------------------------------------------------------*
 *			Polynome4
 *---------------------------------------------------------------------------*/
float
Polynome4( float x, float *coeff )
{
 int	j;
 float	val = coeff[0];

 for (j=1; j<4; j++) {
	val *= x;
	val += coeff[j];
 }

 return	val;
}

/*---------------------------------------------------------------------------*
 *			MAIN
 *---------------------------------------------------------------------------*/
#ifdef	YACME_DBG
#include "RGBA.h"
int
main( int argc, char *argv[] )
{
	glutInit( &argc, argv );

	YACME_init(
		0, 0, WINWIDTH, WINHEIGHT,
		256, (float**)RGBA,
		NULL, NULL
	);
	glutMainLoop();
	return 0;             /* ANSI C requires main to return int. */
}
#endif

