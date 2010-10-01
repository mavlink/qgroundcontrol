/*
 * Copyright (c) 1993-1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

 /* jot text editor source code. */
 /*	    Tom Davis		 */
 /*	 February 7, 1992	 */

/* defines for gizmos */

#include <mui/mui.h>

#define BUTSTRLEN	60
#define LABELSTRLEN	150

#define FONTWIDTH	9	/* for fixed font */
#define BASELINE	9

/* BUTTON STUFF */
#define PUSHBUTTON	3
#define RADIOBUTTON	6
#define INDICATOR	9
#define BED		10
#define BUTTON		11

#define BUTHEIGHT	28
#define BUTWIDTH	75
#define RADIOWIDTH	24
#define RADIOHEIGHT	24
#define TINYRADIOHEIGHT 16
#define TINYRADIOWIDTH  16

typedef struct butn {
    char str[BUTSTRLEN+1];
    int type;
    void (*butcolor)();
    struct butn *link;	    /* for linking radio buttons, e.g. */
    muiObject *object;
} Button;


/* TEXT BOX STUFF */
#define TBSTRLEN 	200 
#define TEXTHEIGHT	17
#define TEXTBOXHEIGHT   28

typedef struct {
    char	str[TBSTRLEN+1];
    char	label[LABELSTRLEN+1];
    int	tp1, tp2;
    int	charWidth;
    int	type;
} TextBox;

TextBox *newtb(int xmin, int xmax);

/* LABEL STUFF */

#define LBLSTRLEN   200	    /* max length of a label string */

typedef struct {
    char	str[LBLSTRLEN+1];
} Label;

Label *newlabel(char *s);

/* SLIDER STUFF */

#define SLIDERWIDTH	20
#define MINSHALF	13
#define ARROWHEIGHT	20

#define SCROLLDOWN	-1
#define SCROLLUP	1
#define THUMB		2

typedef struct {
    int        scenter;                /* the center of the thumb */
    int        shalf;                  /* half of the thumb length */
    int        oldpos;                 /* old scenter  */
    int        arrowdelta;             /* arrow delta  */
    int        thumb;                  /* whether the thumb should show */
} Slider;

typedef Slider VSlider;
typedef Slider HSlider;

/* TEXTLIST STUFF */

typedef struct {
    int		listheight;     	/* in lines of text */
    char        **strs;			/* text	*/
    int		top;    		/* index into strs */
    int		count;  		/* total number of strings */
    int		selecteditem;		/* index into selecteditem or -1 */
    int		locateditem;		/* index into locateditem or -1 */
} TextList;

/* PULLDOWN STUFF */

#define PULLDOWN_HEIGHT 25

typedef struct {
    char    title[40];
    int	    menu;
    int	    xoffset;
} menuentry;

typedef struct {
    int		count;
    int		ishelp;
    menuentry	menus[30];
    menuentry	helpmenu;
} Pulldown;


/* Define for the settbtype() and gettypein() flag */
#define TYPEIN_STRING	0
#define TYPEIN_INT	1
#define TYPEIN_FILE	2
#define TYPEIN_FLOAT	3

/* Color Stuff */

extern Button	*newbed(void);
extern Button	*newbut(void);
extern Button	*newradiobut(void);
extern Pulldown	*newpd(void);
extern void	drawbut(muiObject *);
extern void	drawvs(muiObject *obj);
extern void	drawhs(muiObject *obj);
extern void	drawtl(muiObject *obj);
extern void	drawradiobutton(muiObject *obj);
extern void	drawtinyradio(muiObject *obj);
extern void	drawpulldown(muiObject *obj);
extern int	getcurrentcolor(void);
extern void	setcurrentcolor(int c);
extern void	drawedges(int, int, int, int, void (*)(void), void (*)(void));
extern void	loadbut(Button *,  char *);
extern void	drawbut(muiObject *);
extern int	pressbut(muiObject *);
extern void	drawlabel(muiObject *);
extern void	drawboldlabel(muiObject *);
extern void	loadtb(TextBox *, char *);
extern int	handletb(muiObject *, int, int);
extern void	drawtb(muiObject *);
extern int	inbut(Button *, int, int);
extern int	intb(muiObject *, int, int);
extern void	activatetb(TextBox *);
extern void	deactivatetb(TextBox *);
extern char	*gettbstr(TextBox *);

extern VSlider	*newvs(muiObject *obj, int ymin, int ymax, int scenter, int shalf);
extern VSlider	*newhs(muiObject *obj, int xmin, int xmax, int scenter, int shalf);
extern void	drawsetup(void);
extern void	drawrestore(void);
extern void	backgrounddraw(int xmin, int ymin, int xmax, int ymax);
extern TextList *newtl(muiObject *obj, int listheight);

extern enum muiReturnValue  buttonhandler(muiObject *obj, int  event, int  value, int  x, int  y);
extern enum muiReturnValue  nullhandler(muiObject *obj, int  event, int  value, int  x, int  y);
extern enum muiReturnValue  textboxhandler(muiObject *obj, int  event, int  value, int  x, int  y);
extern enum muiReturnValue  vshandler(muiObject *obj, int event, int value, int x, int y);
extern enum muiReturnValue  hshandler(muiObject *obj, int event, int value, int x, int y);
extern enum muiReturnValue  tlhandler(muiObject *obj, int event, int value, int x, int y);
extern enum muiReturnValue  pdhandler(muiObject *obj, int event, int value, int x, int y);


/* mui events */

#define MUI_DEVICE_DOWN		    1
#define MUI_DEVICE_UP		    2
#define MUI_DEVICE_PRESS	    3
#define MUI_DEVICE_RELEASE	    4
#define MUI_DEVICE_CLICK	    5
#define MUI_DEVICE_DOUBLE_CLICK	    6
#define MUI_KEYSTROKE		    7

#define MUI_BUTTONFONT		    0
#define MUI_BUTTONFONT_BOLD	    0

typedef struct muicons {
    struct muicons  *next;
    muiObject	    *object;
} muiCons;

void	    muiBackgroundClear(void);

void	    muiFreeObject(muiObject *obj);
int	    muiInObject(muiObject *obj, int x, int y);

int	    muiGetLocate(muiObject *obj);
void	    muiSetLocate(muiObject *obj, int state);
int	    muiGetSelect(muiObject *obj);
void	    muiSetSelect(muiObject *obj, int state);
muiCons	    *muiGetListCons(int uilist);
muiObject   *muiGetActiveTB(void);
void	    muiSetUIList(muiObject *obj, int list);
int	    muiGetUIList(muiObject *obj);

void	    muiDrawObject(muiObject *obj);

void	    muiError(char *s);

muiObject   *muiHitInList(int  uilist, int  x, int  y);
void	    muiDrawUIList(int  uilist);
void	    muiHandleEvent(int  event, int  value, int  x, int  y);

