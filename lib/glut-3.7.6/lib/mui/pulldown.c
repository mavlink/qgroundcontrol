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

#include <GL/glut.h>
#include <mui/gizmo.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mui/uicolor.h>
#include <mui/displaylist.h>

/*  M E N U   B A R   F U N C T I O N S  */

Pulldown *newpd(void)
{
    Pulldown *pd = (Pulldown *)malloc(sizeof(Pulldown));
    pd->count = 0;
    return pd;
}

void muiAddPulldownEntry(muiObject *obj, char *title, int menu, int ishelp)
{
    Pulldown *pd = (Pulldown *)obj->object;
    int space = obj->xmax - obj->xmin - 66;
    int i, delta;

    if (ishelp) {
	strcpy(pd->helpmenu.title, title);
	pd->ishelp = 1;
	pd->helpmenu.xoffset = obj->xmax - 58;
	pd->helpmenu.menu = menu;
	return;
    }
    if (pd->count == 29) muiError("muiAddPulldownEntry: more than 29 entries");
    strcpy(pd->menus[pd->count].title, title);
    pd->menus[pd->count].menu = menu;
    pd->count++;
    /* now recalculate spacings */
    if (space > 50*pd->count) {
	for (i = 0; i <= pd->count; i++)
	    pd->menus[i].xoffset = 8 + i*50;
    } else {
	delta = space/pd->count;
	for (i = 0; i <= pd->count; i++)
	    pd->menus[i].xoffset = 8 + i*delta;
    }
}

int activemenu = -1;
extern int menuinuse;

/* ARGSUSED2 */
enum muiReturnValue  pdhandler(muiObject *obj, int event, int value, int x, int y)
{
    int i;
    Pulldown *pd = (Pulldown *)obj->object;
 
    if( !muiGetEnable(obj) || !muiGetVisible(obj) ) return MUI_NO_ACTION;
    
    if (event == MUI_DEVICE_UP) {
	for (i = 0; i < pd->count; i++)
	    if (pd->menus[i].xoffset-8 < x && x < pd->menus[i+1].xoffset-8) {
		if (activemenu != pd->menus[i].menu && !menuinuse) {
		    glutSetMenu(activemenu = pd->menus[i].menu);
		    glutAttachMenu(GLUT_LEFT_BUTTON);
		}
		return MUI_NO_ACTION;
	    }
	if (pd->ishelp && (x > pd->helpmenu.xoffset-8)) {
	    if ((activemenu != pd->helpmenu.menu) && !menuinuse) {
		glutSetMenu(activemenu = pd->helpmenu.menu);
		glutAttachMenu(GLUT_LEFT_BUTTON);
	    }
	    return MUI_NO_ACTION;
	}
	if (activemenu && !menuinuse) {
	    glutDetachMenu(GLUT_LEFT_BUTTON);
	    activemenu = -1;
	}
    }
    return MUI_NO_ACTION;
}

void	drawpulldown(muiObject *obj)
{
    int        i;
    int        xmin, xmax, ymin, ymax;

    if (!muiGetVisible(obj))
	return;

    drawsetup();

    xmin = obj->xmin;
    ymin = obj->ymin;
    xmax = obj->xmax;
    ymax = obj->ymax;

    drawedges(xmin++,xmax--,ymin++,ymax--,uiDkGray,uiVyDkGray);
    drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiDkGray);
    drawedges(xmin++,xmax--,ymin++,ymax--,uiVyLtGray,uiMmGray);
    drawedges(xmin++,xmax--,ymin++,ymax--,uiVyLtGray,uiMmGray);

    uiLtGray();
    	uirectfi(xmin,ymin,xmax,ymax);

    if (obj->object) {
        Pulldown *pd = (Pulldown *)obj->object;

    	for (i = 0; i < pd->count; i++) {
	    if (muiGetEnable(obj)) uiBlack(); else uiDkGray();
	    uicmov2i(obj->xmin + pd->menus[i].xoffset, obj->ymin + 8);
	    uicharstr(pd->menus[i].title, UI_FONT_NORMAL);
    	}
	if (pd->ishelp) {
	    if(muiGetEnable(obj)) uiBlack(); else uiDkGray();
	    uicmov2i(obj->xmin + pd->helpmenu.xoffset, obj->ymin + 8);
	    uicharstr(pd->helpmenu.title, UI_FONT_NORMAL);
	}
    }

    drawrestore();
}

#ifdef NOTDEF

/* static variables */
static int offrightside;
static PullDown *basepd;
static int menubut;
static int menuretval;
static int inpdmode = 0;
static int indopd = 0;
static int savep;
static short savedev[SAVELEN];
static short saveval[SAVELEN];

extern	MenuBar	*locatedmb;

static short pdinit = 0;

void	movemenubar(muiObject *obj)
{
    int	i, tx = MENUXENDGAP-9, totaltwidth = 0;
    int	xsize, ysize, width;

    mb->xmin = mb->xorg;
    mb->xmax = mb->xorg+xsize-1;
    mb->ymin = mb->yorg+ysize-MENUBARHEIGHT;
    mb->ymax = mb->yorg+ysize-1;

    width = mb->xmax - mb->xmin + 1;

    font(PULLDOWNFONT);	/* for correct strwidths */
    for (i = 0; i < mb->count; i++)
        totaltwidth += strwidth(mb->pds[i]->title) + 2*MENUXENDGAP;

    for (i = 0; i < mb->count; i++) {
	if (mb->pds[i]->title)
	    mb->pds[i]->twidth = strwidth(mb->pds[i]->title);
        if ((mb->pds[i]->title) && (!strcmp(mb->pds[i]->title,"Help")))
           mb->pds[i]->txorg = mb->xmax-MENUXENDGAP-mb->pds[i]->twidth-MENUXGAP;
        else
           mb->pds[i]->txorg = mb->xmin + tx;
        mb->pds[i]->xorg = mb->pds[i]->txorg;
	mb->pds[i]->yorg = mb->ymin-TITLESEP;
	mb->pds[i]->orglocked = 1;
	mb->pds[i]->mb = mb;

	if (totaltwidth > width)
            tx += (width-2*MENUXENDGAP)/mb->count-2;
        else
            tx += mb->pds[i]->twidth+24;
    } 
}

MenuBar *newmenubar(void)
{
    MenuBar	*mb;

    mb = (MenuBar *)calloc(1,sizeof(MenuBar));
    mb->count = 0;
    mb->pds = 0;
    mb->locate = -1;
    mb->enable = 1;
    mb->invisible = 0;

    return mb;
}

/* ENABLE STATE FUNCTIONS FOR MENU BARS */

void	enablemb(MenuBar *mb)
{
    if (mb->enable)
	return;
    mb->enable = 1;
}

void	disablemb(MenuBar *mb)
{
    if (!mb->enable)
	return;
    mb->enable = 0;
}

short	getenablemb(MenuBar *mb)
{
    return mb->enable;
}

/* LOCATE STATE FUNCTIONS FOR MENU BARS */

short	gethighlightmb(MenuBar *mb)
{
    return mb->locate;
}

void	highlightmb(MenuBar *mb, int pdnum)
{
    short oldpdnum = mb->locate;

    if (mb->locate == pdnum)
	return;
    mb->locate = pdnum;
    locatedmb = mb;
    if (!mb->invisible) {
	if (pdnum != -1)
	    drawmenubartext(mb,pdnum);
	if (oldpdnum != -1)
	    drawmenubartext(mb,oldpdnum);
    }
}

void	unhighlightmb(MenuBar *mb)
{
    short oldpdnum = mb->locate;

    if (mb->locate == -1)
	return;
    mb->locate = -1;
    if (locatedmb == mb)
	locatedmb = 0;
    if (!mb->invisible)
	if (oldpdnum != -1)
	    drawmenubartext(mb,oldpdnum);
}

/* VISIBLE STATE FUNCTIONS */

void    makevisiblemb(MenuBar *mb)
{
    if (!mb->invisible)
	return;
    mb->invisible = 0;
}

void    makeinvisiblemb(MenuBar *mb)
{
    if (mb->invisible)
	return;
    mb->invisible = 1;
}

short   getvisiblemb(MenuBar *mb)
{
    return 1-mb->invisible;
}

void	loadmenubar(MenuBar *mb, int menucount, PullDown **pdarray)
{
    int	i;

    drawsetup();

    mb->count = menucount;
    mb->pds = (PullDown **)calloc(menucount,sizeof(PullDown));
    mb->locate = -1;
    mb->enable = 1;
    mb->invisible = 0;

    for (i = 0; i < mb->count; i++){
	mb->pds[i] = pdarray[i];
    }
    movemenubar(mb);

    drawrestore();
}

void	addtomenubar(MenuBar *mb, PullDown *pd)
{
    PullDown 	**pds;
    int 	i;

    mb->count++;
    pds = (PullDown **)calloc(mb->count,sizeof(PullDown));

    for (i = 0; i < mb->count-1; i++){
	pds[i] = mb->pds[i];
    }
    pds[i] = pd;
 
    free(mb->pds);
    mb->pds = pds;
}

void addtopd(PullDown *pd, MenuItem *mi)
{
    MenuItem *m, *tail;

    tail = pd->entries;

    if (tail) {
    	while (tail->next)
    	    tail = tail->next;
	tail->next = mi;
    } else
	tail = pd->entries = mi;

    m = pd->entries;

    while (m) {
    	pd->nentries++;
    	m->no = pd->nentries;
        m = m->next;
    }

    fixuppd(pd);
}

void	removefrommenubar(MenuBar *mb, PullDown *pd)
{
    PullDown    **pds;
    int        i, j;

    mb->count--;
    pds = (PullDown **)calloc(mb->count,sizeof(PullDown));

    for (i = 0,j = 0; i <= mb->count; i++) {
	if (mb->pds[i] != pd) {
	    pds[j] = mb->pds[i];
	    j++;
	}
    }

    free(mb->pds);
    mb->pds = pds;
}

void	movemenubar(MenuBar *mb)
{
    int	i, tx = MENUXENDGAP-9, totaltwidth = 0;
    int	xsize, ysize, width;

    getorigin(&mb->xorg, &mb->yorg);
    getsize(&xsize, &ysize);
    mb->xmin = mb->xorg;
    mb->xmax = mb->xorg+xsize-1;
    mb->ymin = mb->yorg+ysize-MENUBARHEIGHT;
    mb->ymax = mb->yorg+ysize-1;

    width = mb->xmax - mb->xmin + 1;

    font(PULLDOWNFONT);	/* for correct strwidths */
    for (i = 0; i < mb->count; i++)
        totaltwidth += strwidth(mb->pds[i]->title) + 2*MENUXENDGAP;

    for (i = 0; i < mb->count; i++) {
	if (mb->pds[i]->title)
	    mb->pds[i]->twidth = strwidth(mb->pds[i]->title);
        if ((mb->pds[i]->title) && (!strcmp(mb->pds[i]->title,"Help")))
           mb->pds[i]->txorg = mb->xmax-MENUXENDGAP-mb->pds[i]->twidth-MENUXGAP;
        else
           mb->pds[i]->txorg = mb->xmin + tx;
        mb->pds[i]->xorg = mb->pds[i]->txorg;
	mb->pds[i]->yorg = mb->ymin-TITLESEP;
	mb->pds[i]->orglocked = 1;
	mb->pds[i]->mb = mb;

	if (totaltwidth > width)
            tx += (width-2*MENUXENDGAP)/mb->count-2;
        else
            tx += mb->pds[i]->twidth+24;
    } 
}

int	inmenubar(MenuBar *mb, int mx, int my) /* Window coordinates	*/
{
    int i;
    
    if (!getenablemb(mb))
	return -1;

    if (getdrawmode() == NORMALDRAW) {
	mx += mb->xorg;
	my += mb->yorg;
    }
    if (mb->xmin <= mx && mx <= mb->xmax && 
	mb->ymin+2 <= my && my <= mb->ymax-2)	/* +-2 for locate highlight*/
       for (i = 0; i < mb->count; i++) {
            if ((mx >= mb->pds[i]->txorg) &&
                (mx <= mb->pds[i]->txorg+mb->pds[i]->twidth+MENUXGAP*2))
		return i;
	}
    return -1;
}

short	locatemenubar(MenuBar *mb, int mx, int my) /* window coordinates */
{
    int highlight = gethighlightmb(mb);
    int inmb  = inmenubar(mb,mx,my);

    if (!getenablemb(mb) || !getvisiblemb(mb) || (highlight == inmb))
        return 0;
    if (inmb != -1) {
	unlocateall();
        highlightmb(mb,inmb);
	return 1;
    } else {
	unhighlightmb(mb);
	return 0;
    }
}

/* DRAWING MENUBAR FUNCTIONS */

void	drawmenubarnow(MenuBar *mb)
{
    uifrontbuffer(1);
    drawmenubar(mb);
    uifrontbuffer(0);
}


#endif /* NOTDEF */
