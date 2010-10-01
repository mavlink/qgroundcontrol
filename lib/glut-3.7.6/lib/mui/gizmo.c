/*
 * Copyright (c) 1992, Silicon Graphics, Inc.
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

#include <GL/glut.h>
#include <mui/gizmo.h>
#include <stdlib.h>
#include <string.h>
#include <mui/uicolor.h>
#include <mui/displaylist.h>

#define BUTTONUP        0
#define BUTTONDOWN      1
#define BUTTONCLICK     2
#define BUTTONDOUBLE    3

void drawtb(muiObject *);

int 	definescaledfont = 0;
char	*tmpfilename;

/*   NEW BUTTON PROCEDURES	*/

int strwidth(char *s)
{
    int len = 0;
    while (*s) {
	len += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *s++);
    }
    return len;
}

Button	*newbut(void)
{
    Button *b;
    b = newbed();
    b->type = BUTTON;
    b->link = 0;
    return b;
}

Button	*newradiobut(void)
{
    Button *b;
    b = newbed();
    b->type = RADIOBUTTON;
    b->link = 0;
    return b;
}

Button	*newbed(void)
{
    Button	*b;

    b = (Button *)malloc(sizeof(Button));
    b->str[0] = 0;
    b->type = BED;
    return b;
}

void muiLoadButton(muiObject *b, char *s)
{
    int temp;
    Button *but = (Button *)b->object;

    strcpy(but->str, s);
    switch (but->type) {
	case PUSHBUTTON:
	    temp = b->xmin + strwidth(but->str) + 20;
	    if (temp > b->xmax)
		b->xmax = temp;
	    break;
	default:
	    break;
    }
}

void drawbuttonbackground(muiObject *b)
{
    int xmin = b->xmin, xmax = b->xmax, ymin = b->ymin, ymax = b->ymax;

    if (b->locate) {
	if (b->select) {
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiVyDkGray,uiWhite);
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiDkGray,uiWhite);
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiLtGray,uiBlack);
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiLtGray);
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiVyLtGray);
	} else {
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiDkGray,uiVyDkGray);
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiDkGray);
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiLtGray);
	    drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiLtGray);
	}
    } else {
	drawedges(xmin++,xmax--,ymin++,ymax--,uiDkGray,uiVyDkGray);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiDkGray);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiVyLtGray,uiMmGray);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiVyLtGray,uiMmGray);
    }
    (b->locate)?uiVyLtGray():uiLtGray();
    uirectfi(xmin,ymin,xmax+1,ymax+1);
}

void drawpushbut(muiObject *b)
{
    Button *but = (Button *)b->object;

    drawbuttonbackground(b);
    uiBlack();
    uicmov2i(b->xmin+ (b->xmax - b->xmin - strwidth(but->str))/2 + 1, b->ymin+9);
    uicharstr(but->str, UI_FONT_NORMAL);
}

void drawbut(muiObject *b)
{
    switch(b->type) {
	case BUTTON:
	case PUSHBUTTON:	
		drawpushbut(b);
		break;
	case BED:	
		break;
	default:
		drawpushbut(b);
		break;
    }
}

void muiChangeLabel(muiObject *obj, char *s)
{
    Label *l;
    
    if (obj->type != MUI_LABEL && obj->type != MUI_BOLDLABEL)
	muiError("muiChangeLabel: not a label");
    l = (Label *)obj->object;
    strncpy(l->str, s, LABELSTRLEN);
    l->str[LABELSTRLEN] = 0;
    return;
}

Label *newlabel(char *s)
{
    Label *l = (Label *)malloc(sizeof(Label));
    strncpy(l->str, s, LABELSTRLEN);
    l->str[LABELSTRLEN] = 0;
    return l;
}

TextBox *activetb = 0;

TextBox	*newtb(int xmin, int xmax)
{
    TextBox	*tb;
    static int inited = 0;

    if (!inited) {
	inited = 1;
    }

    tb = (TextBox *)malloc(sizeof(TextBox));
    tb->charWidth = (xmax - xmin - 9)/FONTWIDTH;
    tb->tp1 = tb->tp2 = 0;
    *tb->str = 0;
    *tb->label = 0;
    return tb;
}

char *muiGetTBString(muiObject *obj)
{
    TextBox *tb = (TextBox *)obj->object;
    return tb->str;
}

void muiClearTBString(muiObject *obj)
{
    TextBox *tb = (TextBox *)obj->object;
    *tb->str = 0;
    tb->tp1 = tb->tp2 = 0;
}

void loadtb(TextBox *tb, char *s)
{
    if (s == 0)
	*tb->str = 0;
    else
	strcpy(tb->str, s);
    tb->tp1 = tb->tp2 = (int) strlen(s);
}

void muiSetTBString(muiObject *obj, char *s)
{
    TextBox *tb = (TextBox *)obj->object;
    loadtb(tb, s);
}

void backspacetb(TextBox *tb)
{
    char *s1, *s2, *stemp;

    if ((tb->tp1 == tb->tp2) && tb->tp1 > 0) {
	s1 = &tb->str[tb->tp1-1];
	while (*s1) {
	    *s1 = *(s1+1);
	    s1++;
	}
	tb->tp1--; tb->tp2--;
	return;
    }
    s1 = &tb->str[tb->tp1];
    s2 = &tb->str[tb->tp2];
    if (s1 > s2) { stemp = s1; s1 = s2; s2 = stemp; }
    stemp = s1;
    while (*s2) {*s1++ = *s2++;}
    *s1 = 0;
    tb->tp1 = tb->tp2 = (int) (stemp - tb->str);
}

void inserttbchar(TextBox *tb, char c)
{
    char	*s1, *s2;
    int	len;

    if (tb->tp1 != tb->tp2) backspacetb(tb);
    len = (int) strlen(tb->str);
    if (len == TBSTRLEN) return;
    s1 = &tb->str[tb->tp1];
    s2 = &tb->str[len+1];
    while (s2 != s1) {
	*s2 = *(s2 - 1);
	s2--;
    }
    *s1 = c;
    tb->tp1++; tb->tp2++;
}

int findtp(muiObject *obj, int x)
{
    TextBox *tb = (TextBox *)obj->object;
    int tp, sl = (int) strlen(tb->str);

    tp = (x - obj->xmin)/FONTWIDTH;
    if (tp < 0) tp = 0;
    if (tp > tb->charWidth) tp = tb->charWidth;
    if (tp > sl) tp = sl;
    return tp;
}

void drawtbcontents(muiObject *obj)
{
    int	xmin = obj->xmin, ymin = obj->ymin; 
    int	ymax = ymin+TEXTBOXHEIGHT;
    int	s1, s2;
    char	str[160], *s;
    TextBox	*tb = (TextBox *)obj->object;

    strncpy(str, tb->str, (unsigned int)tb->charWidth);
    for (s = str; *s; s++)
        if (*s < ' ' || *s >= '\177') *s = '*';
    str[tb->charWidth] = 0;
    s1 = tb->tp1; s2 = tb->tp2;
    if (s1 > tb->charWidth) s1 = tb->charWidth;
    if (s2 > tb->charWidth) s2 = tb->charWidth;

    /* selected area */
    if (obj->active && (s1 != s2)) { 
	uiVyLtGray();
	uirectfi(xmin+6+FONTWIDTH*s1, ymin+7, xmin+6+FONTWIDTH*s2, ymax-6);
    }

    /* contents of text box */
    if (muiGetEnable(obj)) uiBlack(); else uiDkGray();
    uicmov2i(xmin+6, ymin+9);
    uicharstr(str, UI_FONT_FIXED_PITCH);

    /* Blue bar */
    if ((obj->active == 0) || (obj->enable == 0) || (s1 != s2)) return;
    uiBlue();
    uimove2i(xmin+4+FONTWIDTH*s1, ymin+7); uidraw2i(xmin+4+FONTWIDTH*s1, ymax-6); uiendline();
    uimove2i(xmin+5+FONTWIDTH*s1, ymin+7); uidraw2i(xmin+5+FONTWIDTH*s1, ymax-6); uiendline();
}

void drawtb(muiObject *tb)
{
    int	xmin = tb->xmin, xmax = tb->xmax, ymin = tb->ymin;
    int 	ymax = ymin+TEXTBOXHEIGHT;

    if(!muiGetVisible(tb)) return;
    
    if( muiGetEnable(tb) ) {
	drawedges(xmin++,xmax--,ymin++,ymax--,uiDkGray,uiWhite);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiBlack,uiVyLtGray);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiLtGray,uiDkGray);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiTerraCotta,uiTerraCotta);
	uiTerraCotta();
    }
    else {
	drawedges(xmin++,xmax--,ymin++,ymax--,uiDkGray,uiWhite);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiMmGray,uiVyLtGray);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiLtGray,uiDkGray);
	drawedges(xmin++,xmax--,ymin++,ymax--,uiLtGray,uiDkGray);
	uiLtGray();
    }
    uirectfi(xmin, ymin, xmax+1, ymax+1);

    drawtbcontents(tb);
}

void muiActivateTB(muiObject *obj)
{
    muiCons *mcons;

    if ((mcons = muiGetListCons(muiGetUIList(obj))) == (muiCons *)0) return;   
    muiSetActive(obj, 1);
    while (mcons) {
	if (mcons->object != obj && mcons->object->type == MUI_TEXTBOX)
	    muiSetActive(mcons->object, 0);
	mcons = mcons->next;
    }
}

muiObject *muiGetActiveTB(void)
{
    muiCons *mcons;
    int list = muiGetActiveUIList();

    if (list == 0) return 0;
    if ((mcons = muiGetListCons(list)) == (muiCons *)0) return 0;
    while (mcons) {
	if (mcons->object->type == MUI_TEXTBOX && muiGetActive(mcons->object))
	    return mcons->object;
	mcons = mcons->next;
    }
    return 0;
}

enum muiReturnValue textboxhandler(muiObject *obj, int event, int value, int x, int y)
{
    int tp;
    TextBox *tb = (TextBox *)obj->object;

    if( !muiGetEnable(obj) || !muiGetVisible(obj) ) return MUI_NO_ACTION;
    
    switch (event) {
	case MUI_DEVICE_DOWN:
	    tp = findtp(obj, x);
	    tb->tp2 = tp;
	    break;
	case MUI_DEVICE_UP:
	    break;
	case MUI_DEVICE_PRESS:
	    muiActivateTB(obj);
	    tp = findtp(obj, x);
	    tb->tp1 = tb->tp2 = tp;
	    break;
	case MUI_DEVICE_RELEASE:
	    break;
	case MUI_DEVICE_CLICK:
	case MUI_DEVICE_DOUBLE_CLICK:
	    muiActivateTB(obj);
	    tp = findtp(obj, x);
	    tb->tp1 = tb->tp2 = tp;
	    break;
	case MUI_KEYSTROKE:
	    if (value == '\n' || value == '\r')	/* carriage return */
		return MUI_TEXTBOX_RETURN;
	    if (value == '\025') { muiClearTBString(obj); }
	    else if (value == '\b') { backspacetb((TextBox *)obj->object); }
	    else inserttbchar((TextBox *)obj->object, (char)value);
	    break;
    }
    x = y;  /* for lint's sake */
    return MUI_NO_ACTION;
}

void	helpdrawlabel(char *s, int x, int y)
{
    uiBlack();
    	uicmov2i(x, y);
    	uicharstr(s, UI_FONT_NORMAL);
}

void	helpdrawboldlabel(char *s, int x, int y)
{
    uiBlack();	/* XXX Hack! -- no bold font in GLUT */
    	uicmov2i(x, y);
    	uicharstr(s, UI_FONT_NORMAL);
    	uicmov2i(x+1, y);
    	uicharstr(s, UI_FONT_NORMAL);
}

void drawlabel(muiObject *lab)
{
    Label *l = (Label *)lab->object;
    if(!muiGetVisible(lab)) return;
    if(muiGetEnable(lab)) uiBlack(); else uiDkGray();
    uicmov2i(lab->xmin, lab->ymin);
    uicharstr(l->str, UI_FONT_NORMAL);
}

void drawboldlabel(muiObject *lab)
{
    Label *l = (Label *)lab->object;
    if(!muiGetVisible(lab)) return;
    if(muiGetEnable(lab)) uiBlack(); else uiDkGray();
    uicmov2i(lab->xmin, lab->ymin);     /* XXX Hack! -- no bold font in GLUT */
    uicharstr(l->str, UI_FONT_NORMAL);
    uicmov2i(lab->xmin+1, lab->ymin);
    uicharstr(l->str, UI_FONT_NORMAL);
}
