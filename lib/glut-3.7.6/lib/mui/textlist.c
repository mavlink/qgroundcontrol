/*
 * Copyright (c) 1990,1997, Silicon Graphics, Inc.
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

#include <GL/glut.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <mui/displaylist.h>
#include <mui/uicolor.h>
#include <mui/gizmo.h>
#include <mui/textlist.h>
#include <string.h>

extern  short sharefont1;

extern	TextList *locatedtl;

int	gettlheightcount(int ymin, int ymax)
{
    if (ymin > ymax) {
	int tmp = ymin;
	ymin	 = ymax;
	ymax	 = tmp;
    }
    return ((ymax-ymin+1-7)/TEXTHEIGHT);
}

void	movetl(TextList *tl, int ymin, int ymax)
{
    tl->listheight = gettlheightcount(ymin, ymax);
}

int	gettextlistheight(int count)
{
    return (count*TEXTHEIGHT+7);
}

TextList	*newtl(muiObject *obj, int listheight)
{
    TextList	*tl;

    tl = (TextList *)malloc(sizeof(TextList));

    tl->strs = 0;
    tl->top = 0;
    tl->selecteditem = -1;
    tl->locateditem = -1;
    tl->listheight = listheight;

    movetl(tl, obj->ymin, obj->ymax = obj->ymin+gettextlistheight(listheight));

    return tl;
}

void	resettl(TextList *tl)
{
    tl->selecteditem = -1;
    tl->top = 0;
}

void	drawtl(muiObject *obj)
{
    TextList *tl = (TextList *)obj->object;
    int	xmin = obj->xmin, xmax = obj->xmax, 
		ymin = obj->ymin, ymax = obj->ymax;
    int	item = 0, ybot, xright;
    char	**s = &tl->strs[tl->top];

    if (!muiGetVisible(obj))
	return;

    drawsetup();

    drawedges(xmin++, xmax--, ymin++, ymax--, uiMmGray, uiWhite);
    drawedges(xmin++, xmax--, ymin++, ymax--, uiDkGray, uiVyLtGray);
    drawedges(xmin++, xmax--, ymin++, ymax--, uiVyDkGray, uiDkGray);

    drawedges(xmin++, xmax--, ymin++, ymax--, uiVyLtGray, uiMmGray);

    uiLtGray();
        uirectfi(xmin, ymin, xmax, ymax);

    uiBlack();
    uipushviewport();
    uiviewport(xmin+3, ymin-1, xmax-xmin-6, ymax-ymin);
    if (s) while (*s && item < tl->listheight) {
	if (item == tl->selecteditem - tl->top && muiGetEnable(obj)) {
	    ybot = ymin + 1 + (tl->listheight - item - 1)*TEXTHEIGHT;
	    xright = xmin + 11 + FONTWIDTH*(int)strlen(*s);
    	    uiWhite();
	    	uirectfi(xmin+4, ybot, xright, ybot + TEXTHEIGHT - 3);
	}
	
	/* locate highlight */
	if (item == tl->locateditem - tl->top && muiGetEnable(obj)) {
	    ybot = ymin + 1 + (tl->listheight - item - 1)*TEXTHEIGHT;
            xright = xmin + 11 + FONTWIDTH*(int)strlen(*s);
    	    uiWhite();
            	uirecti(xmin+3, ybot-1, xright+1, ybot + TEXTHEIGHT - 2);
            	uirecti(xmin+4, ybot, xright, ybot + TEXTHEIGHT - 3);
	}
    	if (muiGetEnable(obj)) uiBlack(); else uiDkGray();
	    uicmov2i(xmin+8, ymin + 6 + (tl->listheight - item - 1)*TEXTHEIGHT);
	    uicharstr(*s, UI_FONT_NORMAL);
	item++; s++;
    }
    uiBlack();
    uipopviewport();
    drawrestore();
}

int	muiGetTLSelectedItem(muiObject *obj)
{
    TextList *tl = (TextList *)obj->object;
    return tl->selecteditem;
}

void	muiSetTLStrings(muiObject *obj, char **s)
{
    TextList *tl = (TextList *)obj->object;
    tl->strs = s;

    tl->count = 0;
    while(*s) {
	tl->count++;
	s++;
    }
    resettl(tl);
    tl->selecteditem = 0;
}

/* ARGSUSED3 */
enum muiReturnValue tlhandler(muiObject *obj, int event, int value, int x, int y)
{
    int	i;
    TextList *tl = (TextList *)obj->object;

    if( !muiGetEnable(obj) || !muiGetVisible(obj) ) return MUI_NO_ACTION;

    if (event == MUI_DEVICE_DOUBLE_CLICK) {
	i = (y - obj->ymin - 3)/TEXTHEIGHT;
	tl->selecteditem = (tl->listheight - i - 1) + tl->top;
	return MUI_TEXTLIST_RETURN_CONFIRM;
    }
    if (event == MUI_DEVICE_RELEASE) {
	i = (y - obj->ymin - 3)/TEXTHEIGHT;
	tl->selecteditem = (tl->listheight - i - 1) + tl->top;
	return MUI_TEXTLIST_RETURN;
    }
    if (event == MUI_DEVICE_PRESS || event == MUI_DEVICE_DOWN) {
	i = (y - obj->ymin - 3)/TEXTHEIGHT;
	tl->selecteditem = (tl->listheight - i - 1) + tl->top;
	return MUI_NO_ACTION;
    }
    return MUI_NO_ACTION;
}

void	muiSetTLTopInt(muiObject *obj, int top)
{
    TextList *tl = (TextList *)obj->object;
    if (top < 0) top = 0;
    if (top >= tl->count) top = tl->count - 1;
    tl->top = top;
}

void	muiSetTLTop(muiObject *obj, float p)
{
    TextList *tl = (TextList *)obj->object;
    if (tl->count <= tl->listheight) { tl->top = 0; return; }
    tl->top = (1.0-p)*(tl->count + 1 - tl->listheight);
}


void    settlstrings(TextList *tl, char **s)
{
    tl->strs = s;

    tl->count = 0;
    while(*s) {
        tl->count++;
        s++;
    }
}

#ifdef NOTDEF

/* SELECT STATE FUNCTIONS */

void	unselecttl(TextList *tl)
{
    int oldselect = tl->selecteditem;

    if (tl->selecteditem == -1)
	return;
    tl->selecteditem = -1;
    if (!tl->invisible)
	if (oldselect != -1)
	    drawparttl(tl,oldselect);
}

void	selecttlitem(TextList *tl, int item)
{
    int oldselect = tl->selecteditem;

    if (tl->selecteditem == item)
	return;
    tl->selecteditem = item;
    if (!tl->invisible) {
	if (tl->selecteditem != -1)
	    drawparttl(tl,tl->selecteditem);
	if (oldselect != -1)
	    drawparttl(tl,oldselect);
    }
}

short	getselectedtlitem(TextList *tl)
{
    return tl->selecteditem;		/* returns -1 if none selected */
}

void	highlighttl(TextList *tl, int item)
{
    int oldhighlight = tl->locateditem;

    if (tl->locateditem == item)
	return;
    tl->locateditem = item;
    locatedtl = tl;
    if (!tl->invisible) {
	if (tl->locateditem != -1)
	    drawparttl(tl,tl->locateditem);
	if (oldhighlight != -1)
            drawparttl(tl,oldhighlight);
    }
}

void	unhighlighttl(TextList *tl)
{
    int oldhighlight = tl->locateditem;

    if (tl->locateditem == -1)
	return;
    tl->locateditem = -1;
    if (locatedtl == tl)
	locatedtl = 0;
    if (!tl->invisible) {
        if (oldhighlight != -1)
            drawparttl(tl,oldhighlight);
    }
}

short	gethighlighttl(TextList *tl)
{
    return tl->locateditem;
}

void    makevisibletl(TextList *tl)
{
    if (!tl->invisible)
	return;
    tl->invisible = 0;
}

void    makeinvisibletl(TextList *tl)
{
    if (tl->invisible)
	return;
    tl->invisible = 1;
}

/* whether or not the text list is showing */
short   getvisibletl(TextList *tl)
{
    return 1-tl->invisible;
}

/* whether or not the item is showing */
short	getitemvisibletl(TextList *tl, int item)
{
    if ((item < tl->top) || (item >= (tl->top+tl->listheight)) || 
	(item >= tl->count) || (item == -1))
	return 0;
    else
	return 1;
}

short	locatetl(TextList *tl, int x, int y)
{
    int highlight = gethighlighttl(tl);
    int intextlist  = intl(tl,x,y);

    if ((intextlist == highlight) || !getvisibletl(tl))
	return 0;
    if (intextlist != -1) {
	unlocateall();
        highlighttl(tl,intextlist);
    	return 1;
    } else {
	unhighlighttl(tl);
	return 0;
    }
}

char *selectedstring(TextList *tl)
{
    if (tl->selecteditem == -1) return 0;
    return tl->strs[tl->selecteditem];
}

void	drawtlnow(TextList *tl)
{
    uifrontbuffer(1);
    drawtl(tl);
    uifrontbuffer(0);
}

void	drawparttl(TextList *tl,int item)
{
    int	xmin = tl->xmin+8, xmax = tl->xmax-8-SLIDERWIDTH;
    int	ymin = 
		tl->ymin+5+(tl->listheight - item + tl->top - 1)*TEXTHEIGHT;
    int 	ymax = ymin + TEXTHEIGHT - 3;
    char        **s;

    if (!getvisibletl(tl) || !getitemvisibletl(tl,item))
	return;

    s = &tl->strs[item];
    drawsetup();

    uifrontbuffer(1);

    uiLtGray();
    	uirectfi(xmin-1, ymin-1, xmax+1, ymax+1);

    font(TEXTLISTFONT);
    pushviewport();
    scrmask(xmin-1, xmax+1, ymin-1, ymax+1);
    if (s) {
        xmax = xmin + 7 + strwidth(*s);
        if (item == tl->selecteditem) {
    	    uiWhite();
            	uirectfi(xmin, ymin, xmax, ymax);
        }
        if (item == tl->locateditem) {
    	    uiWhite();
            	uirecti(xmin-1, ymin-1, xmax+1, ymax+1);
            	uirecti(xmin, ymin, xmax, ymax);
        }
    	uiBlack();
            uicmov2i(xmin+4, ymin + 5);
            uicharstr(*s);
    }
    popviewport();

    uifrontbuffer(0);

    drawrestore();
}

void	settlstrslinkhandle(char **l, TextList *tl)
{
    int	vshalf, d;
    float	p;

    settlstrings(tl, l);

    if (tl->count <= tl->listheight) {
	vshalf = ((tl->listheight*TEXTHEIGHT - 36)/2) - 2;
	d = 0;
	p = 1.0;
	disablevs(tl->vs);
    } else {
	enablevs(tl->vs);
	p = getvsval(tl->vs);
	vshalf = tl->listheight*(tl->listheight*TEXTHEIGHT-36)/(2*tl->count);
	d = (tl->listheight*TEXTHEIGHT - 2*vshalf)/(tl->count - tl->listheight);
	if (d == 0) d = 1;
    }
    setvsarrowdelta(tl->vs, d);
    setvshalf(tl->vs, vshalf);
    movevsval(tl->vs, p);
}

void	adjustslider(TextList *tl, VSlider *sl)
{
    setvsval(sl, (float) (1.0 - (float) tl->top/ (float) tl->count));
}

short	selectedtl(TextList *tl, int x, int y, int val)
{
    if (intl(tl, x, y) == -1) {
	if (intlboundaries(tl, x, y)) {
    	    tl->selecteditem = -1;
	    drawtlnow(tl);
	}
    } else {
        if (val == UIBUTTONUP) return 0;
        return handletl(tl, LEFTMOUSE, val);
    }
    return 0;
}

#endif /* NOTDEF */
