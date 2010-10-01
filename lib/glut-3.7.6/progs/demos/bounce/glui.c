
/*
  glui.c
  Nate Robins, 1997.

  OpenGL based user interface.

 */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#define GLUI_BORDER  3
#define GLUI_KNOB   40

#define GLUI_RAISED  1
#define GLUI_SUNKEN  0

#define GLUI_HORIZONTAL 0
#define GLUI_VERTICAL   1

#define GLUI_LESS -1
#define GLUI_HIT   0
#define GLUI_MORE  1

#define GLUI_HILITE 0.15

typedef struct _GLUIslider {
    int type;				/* vertical/horizontal */
    int parent;				/* parent of this slider */
    int window;				/* this sliders window */
    int win_x;				/* slider window x (parent relative) */
    int win_y;				/* slider window y (parent relative) */
    int win_w;				/* slider window width */
    int win_h;				/* slider window height */
    int length;				/* length of the slider in pixels */
    int knob;				/* position of the knob in pixels */
    int lit;				/* currently lit? */
    void (*update)(float);		/* callback for updating (returns %) */
    struct _GLUIslider* next;
} GLUIslider;

static GLUIslider* _gluiSliders = NULL;
static GLUIslider* _gluiHit = NULL;

static GLUIslider*
_gluiCurrentSlider(void)
{
    GLUIslider* slider = _gluiSliders;
    int window = glutGetWindow();

    while(slider) {
	if (slider->window == window)
	    break;
	slider = slider->next;
    }

    if (!slider)
	printf("glui: _gluiCurrentSlider() failed!\n");

    return slider;
}

static void
_gluiEmboss(int raised, int lit, int x, int y, int width, int height)
{
    int i;
    float c;

    for (i = 0; i < GLUI_BORDER; i++) {
	c = (float)i / (GLUI_BORDER * 5) + (lit ? GLUI_HILITE : 0.0);
	if (raised)
	    glColor3f(0.275+c, 0.2+c, 0.2+c);
	else
	    glColor3f(0.875-c, 0.8-c, 0.8-c);
	glBegin(GL_LINE_STRIP);
	glVertex2f(x+i+1, y+i);
	glVertex2f(x+width-i-1, y+i);
	glVertex2f(x+width-i-1, y+height-i);
	glEnd();
    }

    for (i = 0; i < GLUI_BORDER; i++) {
	c = (float)i / (GLUI_BORDER * 5);
	if (raised)
	    glColor3f(0.875-c, 0.8-c, 0.8-c);
	else
	    glColor3f(0.275+c, 0.2+c, 0.2+c);
	glBegin(GL_LINE_STRIP);
	glVertex2f(x+i, y+i);
	glVertex2f(x+i, y+height-i-1);
	glVertex2f(x+width-i-1, y+height-i-1);
	glEnd();
    }

    c = lit ? GLUI_HILITE : 0.0;

    if (raised)
	glColor3f(0.575+c, 0.5+c, 0.5+c);
    else
	glColor3f(0.475+c, 0.3+c, 0.3+c);
    glRectf(x+i, y+i, x+width-i, y+height-i);
}

static void
_gluiDisplay(void)
{
    int lit;
    GLUIslider* slider = _gluiCurrentSlider();

    lit = slider->lit ? GLUI_HILITE : 0.0;

    glClear(GL_COLOR_BUFFER_BIT);
    if (slider->type == GLUI_HORIZONTAL) {
	_gluiEmboss(GLUI_SUNKEN, 0, /* never lit */
		    0, 0, slider->length, slider->win_h);
	_gluiEmboss(GLUI_RAISED, slider->lit, 
		    slider->knob - GLUI_KNOB/2, GLUI_BORDER,
		    GLUI_KNOB + GLUI_BORDER, slider->win_h - GLUI_BORDER*2);
	/* XXX why is it GLUI_KNOB+GLUI_BORDER ? */
	glColor3f(0.975+lit, 0.9+lit, 0.9+lit);
	glBegin(GL_LINES);
	glVertex2f(slider->knob-2, GLUI_BORDER*2-1);
	glVertex2f(slider->knob-2, slider->win_h-GLUI_BORDER*2+1);
	glVertex2f(slider->knob+1, GLUI_BORDER*2-1);
	glVertex2f(slider->knob+1, slider->win_h-GLUI_BORDER*2+1);
	glVertex2f(slider->knob+4, GLUI_BORDER*2-1);
	glVertex2f(slider->knob+4, slider->win_h-GLUI_BORDER*2+1);
	glEnd();
	glColor3f(0.175+lit, 0.1+lit, 0.1+lit);
	glBegin(GL_LINES);
	glVertex2f(slider->knob-3, GLUI_BORDER*2-1);
	glVertex2f(slider->knob-3, slider->win_h-GLUI_BORDER*2+1);
	glVertex2f(slider->knob+0, GLUI_BORDER*2-1);
	glVertex2f(slider->knob+0, slider->win_h-GLUI_BORDER*2+1);
	glVertex2f(slider->knob+3, GLUI_BORDER*2-1);
	glVertex2f(slider->knob+3, slider->win_h-GLUI_BORDER*2+1);
	glEnd();
    } else {
	_gluiEmboss(GLUI_SUNKEN, 0, /* never lit */
		    0, 0, slider->win_w, slider->length);
	_gluiEmboss(GLUI_RAISED, slider->lit, 
		    GLUI_BORDER, slider->knob - GLUI_KNOB/2,
		    slider->win_w - GLUI_BORDER*2, GLUI_KNOB + GLUI_BORDER);
	/* XXX why is it GLUI_KNOB+GLUI_BORDER ? */
	glColor3f(0.175+lit, 0.1+lit, 0.1+lit);
	glBegin(GL_LINES);
	glVertex2f(GLUI_BORDER*2-1, slider->knob-2);
	glVertex2f(slider->win_w-GLUI_BORDER*2+1, slider->knob-2);
	glVertex2f(GLUI_BORDER*2-1, slider->knob+1);
	glVertex2f(slider->win_w-GLUI_BORDER*2+1, slider->knob+1);
	glVertex2f(GLUI_BORDER*2-1, slider->knob+4);
	glVertex2f(slider->win_w-GLUI_BORDER*2+1, slider->knob+4);
	glEnd();
	glColor3f(0.975+lit, 0.9+lit, 0.9+lit);
	glBegin(GL_LINES);
	glVertex2f(GLUI_BORDER*2-1, slider->knob-3);
	glVertex2f(slider->win_w-GLUI_BORDER*2+1, slider->knob-3);
	glVertex2f(GLUI_BORDER*2-1, slider->knob+0);
	glVertex2f(slider->win_w-GLUI_BORDER*2+1, slider->knob+0);
	glVertex2f(GLUI_BORDER*2-1, slider->knob+3);
	glVertex2f(slider->win_w-GLUI_BORDER*2+1, slider->knob+3);
	glEnd();
    }

    glutSwapBuffers();
}

static int
_gluiHitKnob(GLUIslider* slider, int x, int y)
{
    if (slider->type == GLUI_HORIZONTAL) {
	/* we know that we don't have to test the y coordinate because
	   the mouse came down in the window (this means that they can
	   hit the borders and still move the knob, but that's okay).
	 */
	if (x > slider->knob - GLUI_KNOB/2 && x < slider->knob + GLUI_KNOB/2)
	    return GLUI_HIT;
	else if (x < slider->knob)
	    return GLUI_LESS;
	else
	    return GLUI_MORE;
    } else {
	/* we know that we don't have to test the x coordinate because
	   the mouse came down in the window (this means that they can
	   hit the borders and still move the knob, but that's okay).
	 */
	if (y > slider->knob - GLUI_KNOB/2 && y < slider->knob + GLUI_KNOB/2)
	    return GLUI_HIT;
	else if (y < slider->knob)
	    return GLUI_LESS;
	else
	    return GLUI_MORE;
    }
}

static void
_gluiConstrainKnob(GLUIslider* slider)
{
    if (slider->knob > slider->length - GLUI_BORDER*2 - GLUI_KNOB/2)
	slider->knob = slider->length - GLUI_BORDER*2 - GLUI_KNOB/2;
    else if (slider->knob < GLUI_BORDER + GLUI_KNOB/2)
	slider->knob = GLUI_BORDER + GLUI_KNOB/2;
}


static float
_gluiKnobPercent(GLUIslider* slider)
{
    return (float)(slider->knob - GLUI_KNOB/2 - GLUI_BORDER) /
	(slider->length - GLUI_BORDER*3 - GLUI_KNOB);
}

static int
_gluiKnobPosition(GLUIslider* slider, float percent)
{
    return GLUI_BORDER + GLUI_KNOB/2 + percent * 
	(slider->length - GLUI_BORDER*3 - GLUI_KNOB);
}

static int _gluiX;
static int _gluiY;
static int _gluiMouseDown;

static void
_gluiTimer(int value)
{
    GLUIslider* slider = (GLUIslider*)value;
    float percent;

    percent = _gluiKnobPercent(slider); 

    if (_gluiMouseDown != 0 && percent > 0.0 && percent < 1.0) {
	if (_gluiMouseDown == GLUI_LESS) {
	    slider->knob -= slider->length / 25.0;
	    _gluiConstrainKnob(slider);
	} else {
	    slider->knob += slider->length / 25.0;
	    _gluiConstrainKnob(slider);
	}
	glutSetWindow(slider->window);
 	glutPostRedisplay();
	slider->update(_gluiKnobPercent(slider));
	glutTimerFunc(20, _gluiTimer, (int)slider);
    }
}

static void
_gluiConvertY(GLUIslider* slider, int* y)
{
    if (slider->win_h < 0) {
	glutSetWindow(slider->parent);
	*y = glutGet(GLUT_WINDOW_HEIGHT) + slider->win_h - slider->win_y - *y;
	glutSetWindow(slider->window);
    } else {
	*y = slider->win_h - *y;
    }
}

/* ARGSUSED */
static void
_gluiMouse(int button, int state, int x, int y)
{
    GLUIslider* slider = _gluiCurrentSlider();
    int side;

    _gluiConvertY(slider, &y);

    _gluiX = x;
    _gluiY = y;
    _gluiHit = NULL;
    _gluiMouseDown = GL_FALSE;

    if (state == GLUT_DOWN) {
	side = _gluiHitKnob(slider, x, y);
	if (side == GLUI_HIT) {
	    _gluiHit = slider;
	} else if (side == GLUI_LESS) {
	    slider->knob -= slider->length / 25.0;
	    _gluiConstrainKnob(slider);
	} else {
	    slider->knob += slider->length / 25.0;
	    _gluiConstrainKnob(slider);
	}
	glutPostRedisplay();
	slider->update(_gluiKnobPercent(slider));
	_gluiMouseDown = side;
	if (side != 0) {
	    glutTimerFunc(500, _gluiTimer, (int)slider);
	}
    } else {
	slider->lit = GL_FALSE;
    }
}

static void
_gluiMotion(int x, int y)
{
    GLUIslider* slider = _gluiHit;

    if (slider) {
	_gluiConvertY(slider, &y);

	if (slider->type == GLUI_HORIZONTAL) {
	    /* clamp the incoming old position, or else the knob will
               possibly "jump" due to the false delta. */
	    if (_gluiX < GLUI_BORDER+1)
		_gluiX = GLUI_BORDER+1;
	    if (_gluiX > slider->length - GLUI_BORDER*2)
		_gluiX = slider->length - GLUI_BORDER*2;
	    /* we don't want to take any action if the mouse pointer
               has moved passed the extents of the slider. */
	    if (x > GLUI_BORDER && x < slider->length - GLUI_BORDER*2) {
		slider->knob -= _gluiX - x;
		_gluiX = x;
	    }
	} else {
	    /* clamp the incoming old position, or else the knob will
               possibly "jump" due to the false delta. */
	    if (_gluiY < GLUI_BORDER+1)
		_gluiY = GLUI_BORDER+1;
	    if (_gluiY > slider->length - GLUI_BORDER*2)
		_gluiY = slider->length - GLUI_BORDER*2;
	    /* we don't want to take any action if the mouse pointer
               has moved passed the extents of the slider. */
	    if (y > GLUI_BORDER && y < slider->length - GLUI_BORDER*2) {
		slider->knob -= _gluiY - y;
		_gluiY = y;
	    }
	}
	_gluiConstrainKnob(slider);

	/* post a display _before_ updating the user, so that the knob
           won't lag behind. */
	glutPostRedisplay();

	/* make sure to set the parent window current, otherwise if
           there is OpenGL state being changed in the update callback,
           it will be done to the sliders context! */
	glutSetWindow(slider->parent);
	slider->update(_gluiKnobPercent(slider));
    }
}

static void
_gluiPassive(int x, int y)
{
    GLUIslider* slider = _gluiCurrentSlider();

    _gluiConvertY(slider, &y);

    if (_gluiHitKnob(slider, x, y) == 0)
	slider->lit = GL_TRUE;
    else
	slider->lit = GL_FALSE;

    glutPostRedisplay();
}

/* ARGSUSED */
static void
_gluiEntry(int state)
{
    GLUIslider* slider = _gluiCurrentSlider();

    /* set the lit flag to false whether we are coming or going
       because if we are doing either, we can't be on top of the knob!  */
    slider->lit = GL_FALSE;
    glutPostRedisplay();
}

void
gluiReshape(int width, int height)
{
    float percent;
    int x, y, w, h;
    GLUIslider* slider = _gluiSliders;

    while (slider) {
	/* we need to get the width and height of the parent, so set
           it current. */
	glutSetWindow(slider->parent);

	/* all this mumbo jumbo takes care of the negative arguments
           to attach the slider to different sides of the window. */
	x = slider->win_x;
	if (x < 0)
	    x = width - slider->win_w + x + 1;
	y = slider->win_y;
	if (y < 0)
	    y = height - slider->win_h + y + 1;
	w = slider->win_w;
	if (w < 0)
	    w = glutGet(GLUT_WINDOW_WIDTH) + slider->win_w - slider->win_x;
	h = slider->win_h;
	if (h < 0)
	    h = glutGet(GLUT_WINDOW_HEIGHT) + slider->win_h - slider->win_y;

	glutSetWindow(slider->window);
	glutPositionWindow(x, y);
	glutReshapeWindow(w, h);

 	percent = _gluiKnobPercent(slider);

	if (slider->type == GLUI_HORIZONTAL)
	    slider->length = w;
	else
	    slider->length = h;

	slider->knob = _gluiKnobPosition(slider, percent);

	_gluiConstrainKnob(slider);

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, 0, h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	slider = slider->next;
    }
}



int
gluiVerticalSlider(int parent, int x, int y, int width, int height,
		   float percent, void (*update)(float))
{
    GLUIslider* slider = (GLUIslider*)malloc(sizeof(GLUIslider));
    slider->next = _gluiSliders;
    _gluiSliders = slider;

    slider->type = GLUI_VERTICAL;
    slider->parent = parent;
    slider->window = glutCreateSubWindow(parent, x, y, width, height);
    slider->win_x = x;
    slider->win_y = y;
    slider->win_w = width;
    slider->win_h = height;
    slider->update = update;
    slider->lit = GL_FALSE;

/*     glutSetCursor(GLUT_CURSOR_LEFT_RIGHT); */
    glutDisplayFunc(_gluiDisplay);
    glutEntryFunc(_gluiEntry);
    glutMouseFunc(_gluiMouse);
    glutMotionFunc(_gluiMotion);
    glutPassiveMotionFunc(_gluiPassive);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    slider->length = height;
    if (height < 0) {
	glutSetWindow(parent);
	slider->length = glutGet(GLUT_WINDOW_HEIGHT) + height - slider->win_y;
    }

    slider->knob = _gluiKnobPosition(slider, percent);
    _gluiConstrainKnob(slider);

    return slider->window;
}


/* On a horizontal slider, the height must be non-negative. */

int
gluiHorizontalSlider(int parent, int x, int y, int width, int height,
		     float percent, void (*update)(float))
{
    GLUIslider* slider = (GLUIslider*)malloc(sizeof(GLUIslider));
    slider->next = _gluiSliders;
    _gluiSliders = slider;

    slider->type = GLUI_HORIZONTAL;
    slider->parent = parent;
    slider->window = glutCreateSubWindow(parent, x, y, width, height);
    slider->win_x = x;
    slider->win_y = y;
    slider->win_w = width;
    slider->win_h = height;
    slider->update = update;
    slider->lit = GL_FALSE;

/*     glutSetCursor(GLUT_CURSOR_LEFT_RIGHT); */
    glutDisplayFunc(_gluiDisplay);
    glutEntryFunc(_gluiEntry);
    glutMouseFunc(_gluiMouse);
    glutMotionFunc(_gluiMotion);
    glutPassiveMotionFunc(_gluiPassive);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    slider->length = width;
    if (width < 0) {
	glutSetWindow(parent);
	slider->length = glutGet(GLUT_WINDOW_WIDTH) + width - slider->win_x;
    }

    slider->knob = _gluiKnobPosition(slider, percent);
    _gluiConstrainKnob(slider);

    return slider->window;
}
