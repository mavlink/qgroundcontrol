#ifndef __mui_h__
#define __mui_h__

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

#ifdef __cplusplus
extern "C" {
#endif

enum muiObjType { MUI_BUTTON, MUI_LABEL, MUI_BOLDLABEL, MUI_TEXTBOX, 
		  MUI_VSLIDER, MUI_TEXTLIST, MUI_RADIOBUTTON, 
		  MUI_TINYRADIOBUTTON, MUI_PULLDOWN, MUI_HSLIDER };

/* MUI Return Values: */

enum muiReturnValue { MUI_NO_ACTION,
			MUI_SLIDER_MOVE,
			MUI_SLIDER_RETURN,
			MUI_SLIDER_SCROLLDOWN,
			MUI_SLIDER_SCROLLUP, 
			MUI_SLIDER_THUMB, 
			MUI_BUTTON_PRESS,
			MUI_TEXTBOX_RETURN,
			MUI_TEXTLIST_RETURN,
			MUI_TEXTLIST_RETURN_CONFIRM
};

typedef struct muiobj {
    enum muiObjType	type;
    int	xmin, xmax, ymin, ymax;	    /* bounding box */
    short	active;	    /* 1 = toggled on, or pressed radio button, or can
				be typed in (textbox), etc */
    short	enable;	    /* 1 = can be accessed; drawn with solid text */
    short	select;	    /* 1 = pressed (must be located at the time */
    short	locate;	    /* 1 = located; usually the cursor is over it */
    short	visible;    /* 1 = drawn.  not visible => not enabled */
    enum muiReturnValue	    (*handler)(struct muiobj *obj, int event, int value, int x, int y);
    int		id;	    /* available for users */
    int		uilist;
    void *	object;
    void	(*callback)(struct muiobj *, enum muiReturnValue);
} muiObject;

/* General MUI Routines */

void	    muiInit(void);
void        muiAttachUIList(int uilist);
void	    muiNewUIList(int  listid);
void	    muiAddToUIList(int  uilist, muiObject *obj);
void	    muiSetCallback(muiObject *obj, void (*callback)(muiObject *, enum muiReturnValue));
void	    muiGetObjectSize(muiObject *obj, int *xmin, int *ymin, int *xmax, int *ymax);
void	    muiSetID(muiObject *obj, int id);
int	    muiGetID(muiObject *obj);

/* for a click that doesn't hit anything: */

void	    muiSetNonMUIcallback(void (*nc)(int, int));

int	    muiGetVisible(muiObject *obj);
void	    muiSetVisible(muiObject *obj, int state);
int	    muiGetActive(muiObject *obj);
void	    muiSetActive(muiObject *obj, int state);
int	    muiGetEnable(muiObject *obj);
void	    muiSetEnable(muiObject *obj, int state);
void	    muiSetActiveUIList(int i);
int	    muiGetActiveUIList(void);

/* Button Routines */

muiObject   *muiNewButton(int xmin, int xmax, int ymin, int ymax);
void	    muiLoadButton(muiObject *but, char *str);
muiObject   *muiNewRadioButton(int xmin, int ymin);
muiObject   *muiNewTinyRadioButton(int xmin, int ymin);
void	    muiLinkButtons(muiObject *obj1, muiObject *obj2);
void	    muiClearRadio(muiObject *rad);

/* Label Routines */

muiObject   *muiNewLabel(int xmin, int ymin, char *label);
muiObject   *muiNewBoldLabel(int xmin, int ymin, char *label);
void	    muiChangeLabel(muiObject *obj, char *s);

/* Text Box Routines */

muiObject   *muiNewTextbox(int xmin, int xmax, int ymin);
char	    *muiGetTBString(muiObject *obj);
void	    muiClearTBString(muiObject *obj);
void	    muiSetTBString(muiObject *obj, char *s);

/* Vertical Slider Routines */

muiObject   *muiNewVSlider(int xmin, int ymin, int ymax, int scenter, int shalf);
float	    muiGetVSVal(muiObject *obj);
void	    muiSetVSValue(muiObject *obj, float val);
void	    muiSetVSArrowDelta(muiObject *obj, int newd);

/* Horizontal Slider Routines */

muiObject   *muiNewHSlider(int xmin, int ymin, int xmax, int scenter, int shalf);
float	    muiGetHSVal(muiObject *obj);
void	    muiSetHSValue(muiObject *obj, float val);
void	    muiSetHSArrowDelta(muiObject *obj, int newd);

/* Text List Routines */

muiObject   *muiNewTextList(int xmin, int ymin, int xmax, int listheight);
void	    muiSetTLTop(muiObject *obj, float p);
int	    muiGetTLSelectedItem(muiObject *obj);
void	    muiSetTLStrings(muiObject *obj, char **s);
void	    muiSetTLTopInt(muiObject *obj, int top);

/* Pulldown Menu Routines */

muiObject   *muiNewPulldown(void);
void	    muiAddPulldownEntry(muiObject *obj, char *title, int menu, int ishelp);

#ifdef __cplusplus
}

#endif
#endif /* __mui_h__ */
