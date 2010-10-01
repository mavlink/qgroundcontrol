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

#include <mui/gizmo.h>
#include <stdio.h>
#include <GL/glut.h>
#include <stdlib.h>

extern int activemenu;
extern int menuinuse;

static muiObject *newmuiobj(void)
{
    muiObject *newobj = (muiObject *)malloc(sizeof(muiObject));
    newobj->active = 0;
    newobj->enable = 1;
    newobj->select = 0;
    newobj->locate = 0;
    newobj->visible = 1;
    newobj->callback = 0;
    muiSetUIList(newobj, muiGetActiveUIList());
    muiAddToUIList(muiGetActiveUIList(), newobj);
    newobj->id = 0;
    return newobj;
}

muiObject *muiNewPulldown(void)
{
    muiObject *newPD = newmuiobj();
    newPD->type = MUI_PULLDOWN;
    newPD->xmin = 0;
    newPD->ymin = glutGet(GLUT_WINDOW_HEIGHT) - PULLDOWN_HEIGHT;
    newPD->xmax = glutGet(GLUT_WINDOW_WIDTH);
    newPD->ymax = glutGet(GLUT_WINDOW_HEIGHT);
    newPD->object = (void *)newpd();
    newPD->handler = pdhandler;
    return newPD;
}

muiObject *muiNewButton(int xmin, int xmax, int ymin, int ymax)
{
    muiObject *newb = newmuiobj();
    newb->type = MUI_BUTTON;
    newb->xmin = xmin;
    newb->ymin = ymin;
    newb->xmax = xmax;
    newb->ymax = ymax;
    newb->object = (void *)newbut();
    ((Button *)(newb->object))->object = newb;
    newb->handler = buttonhandler;
    return newb;
}

muiObject *muiNewRadioButton(int xmin, int ymin)
{
    muiObject *newb = newmuiobj();
    newb->type = MUI_RADIOBUTTON;
    newb->xmin = xmin;
    newb->ymin = ymin;
    newb->xmax = xmin+ RADIOWIDTH-1;
    newb->ymax = ymin+RADIOHEIGHT-1;
    newb->object = (void *)newradiobut();
    ((Button *)(newb->object))->object = newb;
    newb->handler = buttonhandler;
    return newb;
}

muiObject *muiNewTinyRadioButton(int xmin, int ymin)
{
    muiObject *newb = newmuiobj();
    newb->type = MUI_TINYRADIOBUTTON;
    newb->xmin = xmin;
    newb->ymin = ymin;
    newb->xmax = xmin+ TINYRADIOWIDTH-1;
    newb->ymax = ymin+ TINYRADIOHEIGHT-1;
    newb->object = (void *)newradiobut();
    ((Button *)(newb->object))->object = newb;
    newb->handler = buttonhandler;
    return newb;
}

muiObject *muiNewVSlider(int xmin, int ymin, int ymax, int scenter, int shalf)
{
    muiObject *newvsl = newmuiobj();
    newvsl->type = MUI_VSLIDER;
    newvsl->xmin = xmin;
    newvsl->ymin = ymin;
    newvsl->xmax = xmin+SLIDERWIDTH-1;
    newvsl->ymax = ymax;
    newvsl->object = (void *)newvs(newvsl, ymin, ymax, scenter, shalf);
    newvsl->handler = vshandler;
    return newvsl;
}

muiObject *muiNewHSlider(int xmin, int ymin, int xmax, int scenter, int shalf)
{
    muiObject *newhsl = newmuiobj();
    newhsl->type = MUI_HSLIDER;
    newhsl->xmin = xmin;
    newhsl->ymin = ymin;
    newhsl->xmax = xmax;
    newhsl->ymax = ymin+SLIDERWIDTH-1;
    newhsl->object = (void *)newhs(newhsl, xmin, xmax, scenter, shalf);
    newhsl->handler = hshandler;
    return newhsl;
}

muiObject *muiNewTextList(int xmin, int ymin, int xmax, int listheight)
{
    muiObject *newtlo = newmuiobj();
    newtlo->type = MUI_TEXTLIST;
    newtlo->xmin = xmin;
    newtlo->ymin = ymin;
    newtlo->xmax = xmax;
    newtlo->object = (void *)newtl(newtlo, listheight);
    newtlo->handler = tlhandler;
    return newtlo;
}

muiObject *muiNewLabel(int xmin, int ymin, char *label)
{
    muiObject *newlbl = newmuiobj();
    newlbl->type = MUI_LABEL;
    newlbl->xmin = xmin;
    newlbl->ymin = ymin;
    /* XXX maximums */
    newlbl->object = (void *)newlabel(label);
    newlbl->handler = nullhandler;
    return newlbl;
}

muiObject *muiNewBoldLabel(int xmin, int ymin, char *label)
{
    muiObject *newlbl = newmuiobj();
    newlbl->type = MUI_BOLDLABEL;
    newlbl->xmin = xmin;
    newlbl->ymin = ymin;
    /* XXX maximums */
    newlbl->object = (void *)newlabel(label);
    newlbl->handler = nullhandler;
    return newlbl;
}

muiObject *muiNewTextbox(int xmin, int xmax, int ymin)
{
    muiObject *newtextbox = newmuiobj();
    newtextbox->type = MUI_TEXTBOX;
    newtextbox->xmin = xmin;
    newtextbox->ymin = ymin;
    newtextbox->xmax = xmax;
    newtextbox->ymax = ymin + TEXTBOXHEIGHT - 1;
    newtextbox->object = (void *)newtb(xmin, xmax);
    newtextbox->handler = textboxhandler;
    return newtextbox;
}

void muiGetObjectSize(muiObject *obj, int *xmin, int *ymin, int *xmax, int *ymax)
{
    *xmin = obj->xmin;
    *xmax = obj->xmax;
    *ymin = obj->ymin;
    *ymax = obj->ymax;
}

void muiFreeObject(muiObject *obj)
{
    switch (obj->type) {
	case MUI_BUTTON:
	case MUI_RADIOBUTTON:
	case MUI_TINYRADIOBUTTON:
	case MUI_VSLIDER:
	case MUI_HSLIDER:
	case MUI_TEXTBOX:
	case MUI_TEXTLIST:
	case MUI_PULLDOWN:
	    free(obj->object);
	    break;
	case MUI_LABEL:
	case MUI_BOLDLABEL:
	    break;
    }
    free(obj);
}

void	    muiSetID(muiObject *obj, int id)
{
    obj->id = id;
}

int muiGetID(muiObject *obj)
{
    return obj->id;
}

void muiSetCallback(muiObject *obj, void (*callback)(muiObject *, enum muiReturnValue))
{
    obj->callback = callback;
}

int muiInObject(muiObject *obj, int x, int y)
{
    if (obj->xmin <= x && x <= obj->xmax && obj->ymin <= y && y <= obj->ymax)
	return 1;
    return 0;
}

int muiGetLocate(muiObject *obj)
{
    return obj->locate;
}

void muiSetLocate(muiObject *obj, int state)
{
    obj->locate = (short) state;
}

int muiGetSelect(muiObject *obj)
{
    return obj->select;
}

void muiSetUIList(muiObject *obj, int list)
{
    obj->uilist = list;
}

int muiGetUIList(muiObject *obj)
{
    return obj->uilist;
}

void muiSetSelect(muiObject *obj, int state)
{
    obj->select = (short) state;
}

int muiGetVisible(muiObject *obj)
{
    return obj->visible;
}

void muiSetVisible(muiObject *obj, int state)
{
    obj->visible = (short) state;
}

int muiGetActive(muiObject *obj)
{
    return obj->active;
}

void muiSetActive(muiObject *obj, int state)
{
    obj->active = (short) state;
}

int muiGetEnable(muiObject *obj)
{
    return obj->enable;
}

void muiSetEnable(muiObject *obj, int state)
{
    obj->enable = (short) state;
}

void muiDrawObject(muiObject *obj)
{
    switch (obj->type) {
	case MUI_BUTTON:
	    drawbut(obj);
	    break;
	case MUI_RADIOBUTTON:
	    drawradiobutton(obj);
	    break;
	case MUI_TINYRADIOBUTTON:
	    drawtinyradio(obj);
	    break;
	case MUI_LABEL:
	    drawlabel(obj);
	    break;
	case MUI_BOLDLABEL:
	    drawboldlabel(obj);
	    break;
	case MUI_TEXTBOX:
	    drawtb(obj);
	    break;
	case MUI_VSLIDER:
	    drawvs(obj);
	    break;
	case MUI_HSLIDER:
	    drawhs(obj);
	    break;
	case MUI_TEXTLIST:
	    drawtl(obj);
	    break;
	case MUI_PULLDOWN:
	    drawpulldown(obj);
	    break;
    }
}

void muiError(char *s)
{
    fprintf(stderr, "%s\n", s);
    exit(0);
}

#define MAX_UI_LISTS 50
static muiCons *muilist[MAX_UI_LISTS];
static int muilistindex[MAX_UI_LISTS];

void muiNewUIList(int listid)
{
    static int inited = 0;
    int i;

    if (inited == 0) {
	inited = 1;
	for (i = 1; i < MAX_UI_LISTS; i++)
	    muilistindex[i] = -1;
	muilistindex[0] = listid;
	muiSetActiveUIList(listid);
	return;
    }
    for (i = 0; i < MAX_UI_LISTS; i++)
	if (muilistindex[i] == -1) {
	    muilistindex[i] = listid;
	    muiSetActiveUIList(listid);
	    return;
	}
    muiError("muiNewUIList: No more UI lists available");
}

int muiGetListId(int uilist)
{
    int i;
    
    for (i = 0; i < MAX_UI_LISTS; i++) {
	if (muilistindex[i] == uilist) return i;	
    }
    muiError("muiAddToUIList: illegal UI list identifier");
    return -1;
}

muiCons *muiGetListCons(int uilist)
{
    int i;
    
    for (i = 0; i < MAX_UI_LISTS; i++) {
	if (muilistindex[i] == uilist) return muilist[i];	
    }
    muiError("muiGetListCons: illegal UI list identifier");
    return (muiCons *)0;
}

void muiAddToUIList(int uilist, muiObject *obj)
{
    int i;
    muiCons *mcons;

    if (uilist == 0) {
	muiError("muiAddToUIList: no active UI list");
    }
    if ((i = muiGetListId(uilist)) == -1) return;
    mcons = (muiCons *)malloc(sizeof(muiCons));
    mcons->next = muilist[i];
    muilist[i] = mcons;
    mcons->object = obj;
}

static muiObject *muiFastHitInList(muiCons *mcons, int x, int y)
{
    while (mcons) {
	if (muiInObject(mcons->object, x, y))
	    switch (mcons->object->type) {
		case MUI_BUTTON:
		case MUI_TEXTBOX:
		case MUI_VSLIDER:
		case MUI_HSLIDER:
		case MUI_TEXTLIST:
		case MUI_RADIOBUTTON:
		case MUI_TINYRADIOBUTTON:
		case MUI_PULLDOWN:
		    return mcons->object;
		case MUI_LABEL:
		case MUI_BOLDLABEL:
		    return 0;
	    }
	mcons = mcons->next;
    }
    return (muiObject *)0;	/* not found */
}

muiObject *muiHitInList(int uilist, int x, int y)
{
    muiCons *mcons;
    
    if ((mcons = muiGetListCons(uilist)) == (muiCons *)0) return (muiObject *)0;
    return muiFastHitInList(mcons, x, y);
}

void muiDrawUIList(int uilist)
{
    muiCons *mcons;

    if ((mcons = muiGetListCons(uilist)) == (muiCons *)0) return;
    muiBackgroundClear();
    while (mcons) {
	muiDrawObject(mcons->object);
	mcons = mcons->next;
    }
}

static muiObject *SelectedObj, *LocatedObj;
static muiCons *ActiveCons;
static int ActiveUIList = 0;
static muiObject *ActiveSlider = 0;

void muiSetActiveUIList(int i)
{
    ActiveUIList = i;
}

int muiGetActiveUIList(void)
{
    return ActiveUIList;
}

static void muiInitInteraction(int uilist)
{
    muiCons *mcons;
    muiObject *obj;

    if ((mcons = muiGetListCons(uilist)) == (muiCons *)0) return;
    SelectedObj = LocatedObj = (muiObject *)0;
    ActiveCons = mcons;
    ActiveUIList = uilist;
    while (mcons) {
	obj = mcons->object;
	muiSetSelect(obj, 0);
	muiSetLocate(obj, 0);
	mcons = mcons->next;
    }
}

static void (*noncallback)(int, int) = 0;

static void nonmuicallback(int x, int y)
{
    if (noncallback == 0) return;
    noncallback(x, y);
}

void muiSetNonMUIcallback(void (*nc)(int, int))
{
    noncallback = nc;
}

void muiHandleEvent(int event, int value, int x, int y)
{
    muiObject *obj;
    static int lastactive = 0;
    enum muiReturnValue retval;

    if (ActiveUIList == 0) {
	muiError("muiHandleEvent: no active UI list");
    }
    if (lastactive != ActiveUIList) {
	muiInitInteraction(lastactive = ActiveUIList);
    }
    if ((event == MUI_KEYSTROKE)) {
	if (obj = muiGetActiveTB()) {
	    retval = (obj->handler)(obj, event, value, x, y);
	    if (retval && obj->callback)
		(obj->callback)(obj, retval);
	    return;
	}
	/* may have to add text editors, et cetera */
	return;
    }
    if (event == MUI_DEVICE_RELEASE && ActiveSlider) {
	retval = (ActiveSlider->handler)(ActiveSlider, event, value, x, y);
	if (retval && ActiveSlider->callback)
	    (ActiveSlider->callback)(ActiveSlider, retval);
	ActiveSlider = 0;
	return;
    }
    ActiveCons = muiGetListCons(ActiveUIList);
    obj = muiFastHitInList(ActiveCons, x, y);
    if (obj == 0 && event == MUI_DEVICE_PRESS) {
	nonmuicallback(x, y);
	return;
    }
    if (event == MUI_DEVICE_UP && (!menuinuse) && (activemenu != -1) && (obj == 0 || obj->type != MUI_PULLDOWN)) {
	activemenu = -1;
	glutDetachMenu(GLUT_LEFT_BUTTON);
    }
    if (obj && (obj->type == MUI_VSLIDER || obj->type == MUI_HSLIDER)
					    && event == MUI_DEVICE_PRESS)
							    ActiveSlider = obj;
    if (obj == 0) {
	if (ActiveSlider) {
	    retval = (ActiveSlider->handler)(ActiveSlider, event, value, x, y);
	    if (retval && ActiveSlider->callback)
		(ActiveSlider->callback)(ActiveSlider, retval);
	    return;
	}
        if (LocatedObj) {
	    muiSetLocate(LocatedObj, 0);
	    muiDrawObject(LocatedObj);
	    LocatedObj = 0;
	}
	if ((event == MUI_DEVICE_RELEASE) && SelectedObj) {
	    muiSetSelect(SelectedObj, 0);
	    muiSetLocate(SelectedObj, 0);
	    muiDrawObject(SelectedObj);
	    LocatedObj = SelectedObj = 0;
	}
	return;
    }
    retval = (obj->handler)(obj, event, value, x, y);
    if (retval && obj->callback)
	(obj->callback)(obj, retval);
    return;
}

/* ARGSUSED2 */
enum muiReturnValue buttonhandler(muiObject *obj, int event, int value, int x, int y)
{
    if (!muiGetEnable(obj) || !muiGetVisible(obj)) return MUI_NO_ACTION;

    switch (event) {
	case MUI_DEVICE_DOWN:
	    return MUI_NO_ACTION;
	case MUI_DEVICE_UP:
	    if (LocatedObj != obj) {
	        if (LocatedObj) {
		    muiSetLocate(LocatedObj, 0);
		    muiDrawObject(LocatedObj);
		}
		muiSetLocate(obj, 1);
		muiDrawObject(obj);
		LocatedObj = obj;
	    }
	    return MUI_NO_ACTION;
	case MUI_DEVICE_PRESS:
	    muiSetSelect(obj, 1);
	    muiSetLocate(obj, 1);
	    SelectedObj = LocatedObj = obj;
	    muiDrawObject(obj);
	    return MUI_NO_ACTION;
	case MUI_DEVICE_RELEASE:
	    if (SelectedObj != obj) {
		muiSetSelect(SelectedObj, 0);
		muiSetLocate(SelectedObj, 0);
		muiDrawObject(SelectedObj);
		muiSetLocate(obj, 1);
		LocatedObj = obj;
		muiDrawObject(obj);
		return MUI_NO_ACTION;
	    }
	    if (obj->type == MUI_RADIOBUTTON || obj->type == MUI_TINYRADIOBUTTON) {
		Button *b = (Button *)obj->object, *b1;
		if (b->link) {
		    muiSetActive(obj, 1);
		    b1 = b->link;
		    while (b1 != b) {
			muiSetActive(b1->object, 0);
			b1 = b1->link;
		    }
		} else {
 		    muiSetActive(obj, ( muiGetActive(obj) ? 0 : 1 ) );
  		}
	    }
	    muiSetSelect(obj, 0);
	    muiDrawObject(obj);
	    return MUI_BUTTON_PRESS;
	case MUI_DEVICE_CLICK:
	    muiSetSelect(obj, 0);
	    muiSetLocate(obj, 1);
	    LocatedObj = obj;
	    muiDrawObject(obj);
	    return MUI_BUTTON_PRESS;
	case MUI_DEVICE_DOUBLE_CLICK:
	    muiSetSelect(obj, 0);
	    muiSetLocate(obj, 1);
	    LocatedObj = obj;
	    muiDrawObject(obj);
	    return MUI_BUTTON_PRESS; /* XXX this may not be right; */
	case MUI_KEYSTROKE:
	    return MUI_NO_ACTION;
	default:
	    muiError("buttonhandler: wacko event");
	    return MUI_NO_ACTION;
    }
}

/* ARGSUSED */
enum muiReturnValue nullhandler(muiObject *obj, int event, int value, int x, int y)
{
    return MUI_NO_ACTION;
}

