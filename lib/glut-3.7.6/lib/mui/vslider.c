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
#include <stdlib.h>
#include <mui/gizmo.h>
#include <mui/vslider.h>
#include <mui/displaylist.h>
#include <mui/uicolor.h>

extern	VSlider *locatedvs;

int getvstrough(muiObject *obj)
{
    return obj->ymax - obj->ymin - 2*ARROWHEIGHT;
}

void	setvscenter(muiObject *obj, int scenter)
{
    VSlider *vs = (VSlider *)obj->object;
    if ((scenter - vs->shalf) < (obj->ymin+ARROWHEIGHT))
        vs->scenter = getvstrough(obj)/2 + obj->ymin+ARROWHEIGHT;
    else
        vs->scenter = scenter;
}

void	muiSetVSArrowDelta(muiObject *obj, int newd)
{
    VSlider *vs = (VSlider *)obj->object;
    vs->arrowdelta = newd;
}

VSlider	*newvs(muiObject *obj, int ymin, int ymax, int scenter, int shalf)
{
    VSlider	*vs;

    vs = (VSlider *)malloc(sizeof(VSlider));
    obj->object = (VSlider *)vs;
    if (shalf == 0) {
	vs->shalf = 0;
    } else if (shalf < MINSHALF) 
	vs->shalf = MINSHALF;
    else
	vs->shalf = shalf;

    if ((ymax - ymin + 1) <= (2*ARROWHEIGHT+2*MINSHALF))
    	vs->thumb = 0;
    else
    	vs->thumb = 1;

    setvscenter(obj, scenter);
    vs->oldpos = vs->scenter;

    muiSetVSArrowDelta(obj, 1);
    return vs;
}

void	freevs(VSlider *vs)
{
    if (vs) {
	free(vs);
    }
}

void	drawvsarrows(muiObject *obj)
{
    int	xmin = obj->xmin, xmax = obj->xmin+ARROWHEIGHT,
		ymin = obj->ymin, ymax = obj->ymax;

    if (!muiGetVisible(obj))
        return;

    /* Draw the arrows: */

    /* down arrow */
    uiDkGray();
    uirecti(xmin,ymin,xmax,ymin+20);

    if (muiGetVisible(obj)) {
        if (obj->locate == SCROLLDOWN) {
    	    if (obj->select == SCROLLDOWN) {
    	        drawedges(xmin+1,xmax-1,ymin+1,ymin+19,uiMmGray,uiWhite);
    	        drawedges(xmin+2,xmax-2,ymin+2,ymin+18,uiLtGray,uiWhite);
	    } else {
    	        drawedges(xmin+1,xmax-1,ymin+1,ymin+19,uiWhite,uiMmGray);
    	        drawedges(xmin+2,xmax-2,ymin+2,ymin+18,uiWhite,uiLtGray);
	    }
	    uiVyLtGray();
	    uirectfi(xmin+3,ymin+3,xmax-3,ymin+17);
        } else {
    	    if (obj->select == SCROLLDOWN) {
    	        drawedges(xmin+1,xmax-1,ymin+1,ymin+19,uiMmGray,uiVyLtGray);
    	        drawedges(xmin+2,xmax-2,ymin+2,ymin+18,uiMmGray,uiVyLtGray);
	    } else {
    	        drawedges(xmin+1,xmax-1,ymin+1,ymin+19,uiWhite,uiMmGray);
    	        drawedges(xmin+2,xmax-2,ymin+2,ymin+18,uiVyLtGray,uiMmGray);
	    }
	    uiLtGray();
	    uirectfi(xmin+3,ymin+3,xmax-3,ymin+17);
        }
    } else {
        drawedges(xmin+1,xmax-1,ymin+1,ymin+19,uiVyLtGray,uiMmGray);
	uiLtGray();
	uirectfi(xmin+2,ymin+2,xmax-2,ymin+18);
    }

    /* arrow */
    if (muiGetEnable(obj))
	uiDkGray();
    else
	uiMmGray();
    uimove2i(xmin+5,ymin+14);
    uidraw2i(xmin+14,ymin+14);
    uiendline();
    uirectfi(xmin+6,ymin+13,xmin+13,ymin+12);
    uirectfi(xmin+7,ymin+11,xmin+12,ymin+10);
    uirectfi(xmin+8,ymin+8,xmin+11,ymin+9);
    uirectfi(xmin+9,ymin+6,xmin+10,ymin+7);
   
    /* up arrow */
    uiDkGray();
    uirecti(xmin,ymax-20,xmax,ymax);

    if (muiGetEnable(obj)) {
        if (obj->locate == SCROLLUP) {
    	    if (obj->select == SCROLLUP) {
    	        drawedges(xmin+1,xmax-1,ymax-19,ymax-1,uiMmGray,uiWhite);
    	        drawedges(xmin+2,xmax-2,ymax-18,ymax-2,uiLtGray,uiWhite);
	    } else {
    	        drawedges(xmin+1,xmax-1,ymax-19,ymax-1,uiWhite,uiMmGray);
    	        drawedges(xmin+2,xmax-2,ymax-18,ymax-2,uiWhite,uiLtGray);
	    }
	    uiVyLtGray();
	    uirectfi(xmin+3,ymax-17,xmax-3,ymax-3);
        } else {
    	    if (obj->select == SCROLLUP) {
    	        drawedges(xmin+1,xmax-1,ymax-19,ymax-1,uiMmGray,uiVyLtGray);
    	        drawedges(xmin+2,xmax-2,ymax-18,ymax-2,uiMmGray,uiVyLtGray);
	    } else {
    	        drawedges(xmin+1,xmax-1,ymax-19,ymax-1,uiWhite,uiMmGray);
    	        drawedges(xmin+2,xmax-2,ymax-18,ymax-2,uiVyLtGray,uiMmGray);
	    }
	    uiLtGray();
	    uirectfi(xmin+3,ymax-17,xmax-3,ymax-3);
        }
    } else {
	drawedges(xmin+1,xmax-1,ymax-19,ymax-1,uiVyLtGray,uiMmGray);
	uiLtGray();
	uirectfi(xmin+2,ymax-18,xmax-2,ymax-2);
    }

    /* arrow */

    if (muiGetEnable(obj))
        uiDkGray();
    else
        uiMmGray();
    uirectfi(xmin+9,ymax-6,xmin+10,ymax-7);
    uirectfi(xmin+8,ymax-8,xmin+11,ymax-9);
    uirectfi(xmin+7,ymax-10,xmin+12,ymax-11);
    uirectfi(xmin+6,ymax-12,xmin+13,ymax-13);
    uimove2i(xmin+5,ymax-14);
    uidraw2i(xmin+14,ymax-14);
    uiendline();
}

void	drawvs(muiObject *obj)
{
    VSlider *vs = (VSlider *)obj->object;

    int	xmin = obj->xmin, ymax = obj->ymax-ARROWHEIGHT, 
		ymin = obj->ymin+ARROWHEIGHT;
    int 	xmax = xmin+SLIDERWIDTH;
    int	symin = vs->scenter - vs->shalf;
    int	symax = vs->scenter + vs->shalf;
    int	oldsymin = vs->oldpos - vs->shalf;
    int	oldsymax = vs->oldpos + vs->shalf;

    drawsetup();

    if (!muiGetVisible(obj)) {
	backgrounddraw(xmin,ymin,xmax,ymax);
        drawrestore();
        return;
    }

    /* trough */

    uiDkGray();
    uirecti(xmin, ymin, xmax, ymax);

    drawedges(xmin+1,xmax-1,ymin+1,ymax-1,uiVyLtGray,uiMmGray);

    uiLtGray();
    	uirectfi(xmin+2, ymin+2, xmax-2, ymax-2);

    if (vs->thumb) {
        /* last thumb position */
        if ((vs->oldpos != vs->scenter) && (obj->enable)) {
	
    	    uiDkGray();
	        uimove2i(xmax-2, oldsymax);
	        uidraw2i(xmin+1, oldsymax);
		uiendline();
    
    	    uiMmGray();
	        uimove2i(xmin+1, oldsymax);
	        uidraw2i(xmin+1, oldsymin);
		uiendline();
    	
    	    uiVyLtGray();
	        uimove2i(xmin+2, oldsymin);
	        uidraw2i(xmax-1, oldsymin);
		uiendline();
    
    	    uiLtGray();
	        uimove2i(xmax-1, oldsymin);
	        uidraw2i(xmax-1, oldsymax);
		uiendline();

    	    uiVyLtGray();
	        uimove2i(xmax-2, --oldsymax);
	        uidraw2i(xmax-2, ++oldsymin);
	        uidraw2i(xmin+2, oldsymin);
		uiendline();
    
    	    uiVyDkGray();
	        uimove2i(xmin+2, oldsymin);
	        uidraw2i(xmin+2, oldsymax);
	        uidraw2i(xmax-3, oldsymax);
		uiendline();
    
    	    uiDkGray();
	        uimove2i(xmin+3, --oldsymax);
	        uidraw2i(xmax-3, oldsymax);
		uiendline();
    	
    	    uiLtGray();
	        uimove2i(xmax-3, oldsymax);
	        uidraw2i(xmax-3, ++oldsymin);
	        uidraw2i(xmin+3, oldsymin);
		uiendline();
    
    	    uiMmGray();
	        uirectfi(xmin+3, ++oldsymin, xmax-4, --oldsymax);
    	
        }

        if (obj->enable) {

    	    /* thumb */
    	    uiDkGray();
    	        uirecti(xmin,symin,xmax,symax);
    
    	    if (obj->locate == THUMB) {
    		drawedges(xmin+1,xmax-1,symin+1,symax-1,uiWhite,uiDkGray);
    		drawedges(xmin+2,xmax-2,symin+2,symax-2,uiWhite,uiLtGray);
    		drawedges(xmin+3,xmax-3,symin+3,symax-3,uiWhite,uiLtGray);
    
    		uiVyLtGray();
    		    uirectfi(xmin+4, symin+4, xmax-4, symax-4);
    	
    		/* ridges on thumb */
    		uiDkGray();
    		    uimove2i(xmin+3,vs->scenter);
    		    uidraw2i(xmax-3,vs->scenter);
		    uiendline();
    		    uimove2i(xmin+3,vs->scenter-4);
    		    uidraw2i(xmax-3,vs->scenter-4);
		    uiendline();
    		    uimove2i(xmin+3,vs->scenter+4);
    		    uidraw2i(xmax-3,vs->scenter+4);
		    uiendline();
   
    		uiWhite();
		    uirectfi(xmin+3,vs->scenter+1,xmax-3,vs->scenter+2);
		    uirectfi(xmin+3,vs->scenter+5,xmax-3,vs->scenter+6);
		    uirectfi(xmin+3,vs->scenter-2,xmax-3,vs->scenter-3);
    	    } else {
    		drawedges(xmin+1,xmax-1,symin+1,symax-1,uiWhite,uiDkGray);
    		drawedges(xmin+2,xmax-2,symin+2,symax-2,uiVyLtGray,uiMmGray);
    		drawedges(xmin+3,xmax-3,symin+3,symax-3,uiVyLtGray,uiMmGray);
   
      		uiLtGray();
          	    uirectfi(xmin+4, symin+4, xmax-4, symax-4);
	    	
    		/* ridges on thumb */
    		uiBlack();
    		    uimove2i(xmin+3,vs->scenter);
    		    uidraw2i(xmax-3,vs->scenter);
		    uiendline();
    		    uimove2i(xmin+3,vs->scenter-4);
    		    uidraw2i(xmax-3,vs->scenter-4);
		    uiendline();
    		    uimove2i(xmin+3,vs->scenter+4);
    		    uidraw2i(xmax-3,vs->scenter+4);
		    uiendline();

    		uiWhite();
    		    uimove2i(xmin+3,vs->scenter+1);
    		    uidraw2i(xmax-3,vs->scenter+1);
		    uiendline();
    		    uimove2i(xmin+3,vs->scenter-3);
    		    uidraw2i(xmax-3,vs->scenter-3);
		    uiendline();
    		    uimove2i(xmin+3,vs->scenter+5);
    		    uidraw2i(xmax-3,vs->scenter+5);
		    uiendline();
    	    }
    	}
    }
    drawvsarrows(obj);

    drawrestore();
}

enum muiReturnValue vshandler(muiObject *obj, int event, int value, int x, int y)
{
    int my = y;
    static int mfudge=0;
    static enum muiReturnValue retval = MUI_NO_ACTION;
    VSlider *vs = (VSlider *)obj->object;

    if (!muiGetEnable(obj) || !muiGetVisible(obj))
	return MUI_NO_ACTION;
    switch (event) {
	case MUI_DEVICE_RELEASE:
	    if (value == 0) { 
		vs->oldpos = vs->scenter; 
		muiSetSelect(obj, 0);
		return MUI_SLIDER_RETURN; 
	    }
	case MUI_DEVICE_PRESS:
	case MUI_DEVICE_CLICK:
	    /* in the arrows */
	    if (my >= obj->ymin && my <= obj->ymax && 
	       (my < obj->ymin+ARROWHEIGHT || my > obj->ymax-ARROWHEIGHT)) {
		mfudge = -10000;
		if (my < obj->ymin+ARROWHEIGHT) { /* boink down */
		    my = vs->scenter - vs->arrowdelta;
		    retval = MUI_SLIDER_SCROLLDOWN;
		} else { /* boink up */
		    my = vs->scenter + vs->arrowdelta;
		    retval = MUI_SLIDER_SCROLLUP;
		}
		if (event == MUI_DEVICE_CLICK) {
		    muiSetSelect(obj, 0);
		    retval = MUI_SLIDER_RETURN;
		}
		if (my - vs->shalf < obj->ymin+1+ARROWHEIGHT)
		    my = obj->ymin+1+vs->shalf+ARROWHEIGHT;
		if (my + vs->shalf > obj->ymax-1-ARROWHEIGHT) 
		    my = obj->ymax-1-vs->shalf-ARROWHEIGHT;
		vs->scenter = my; 
		break;
	    } else if (my >= obj->ymin && my <= obj->ymax)
	    	retval = MUI_SLIDER_THUMB;
	    vs->oldpos = vs->scenter;
	    if (my >= vs->scenter-vs->shalf && my <= vs->scenter+vs->shalf)
		mfudge = vs->scenter - my;
	    else
		mfudge = 0;
	    break;
	case MUI_DEVICE_DOWN:
	    if (mfudge == -10000) {	/* auto - repeat the arrow keys */
                if (retval == MUI_SLIDER_SCROLLDOWN) {
                    my = vs->scenter - vs->arrowdelta;
                    if (my - vs->shalf < obj->ymin+1+ARROWHEIGHT)
                        my = obj->ymin+1+vs->shalf+ARROWHEIGHT;
                } else {
                    my = vs->scenter + vs->arrowdelta;
                    if (my + vs->shalf > obj->ymax-1-ARROWHEIGHT)
                        my = obj->ymax-1-vs->shalf-ARROWHEIGHT;
                }
                vs->scenter = my;
                break;
	    }
	    my = y+mfudge;
	    if (my - vs->shalf < obj->ymin+1+ARROWHEIGHT)
		my = obj->ymin+1+vs->shalf+ARROWHEIGHT;
	    if (my + vs->shalf > obj->ymax-1-ARROWHEIGHT) 
		my = obj->ymax-1-vs->shalf-ARROWHEIGHT;

	    /* adjust thumb */
	    vs->scenter = my; 
	    break;
    }
    x = x;	/* for lint's sake */
    return retval;
}

float muiGetVSVal(muiObject *obj)
{
    VSlider *vs = (VSlider *)obj->object;

    return (vs->scenter-obj->ymin-1.0-vs->shalf-ARROWHEIGHT)/
		(obj->ymax - obj->ymin - 2.0*vs->shalf - 2.0-2*ARROWHEIGHT);
}

void	setvshalf(muiObject *obj, int shalf)
{
    VSlider *vs = (VSlider *)obj->object;
    vs->shalf = shalf;
    if (vs->shalf==0)
	muiSetEnable(obj, 0);
    else if (vs->shalf < MINSHALF) {
        vs->shalf = MINSHALF;
	muiSetEnable(obj, 1);
    } else if (2*vs->shalf >= getvstrough(obj)) {
        vs->shalf = getvstrough(obj)/2;
	muiSetEnable(obj, 0);
    }
}

void	movevsval(muiObject *obj, float val)
{
    float	f;
    VSlider *vs = (VSlider *)obj->object;

    if (val < 0.0) val = 0.0;
    if (val > 1.0) val = 1.0;
    f = val*(obj->ymax - obj->ymin - 2.0*vs->shalf - 2.0-2*ARROWHEIGHT);
    vs->scenter = f + vs->shalf + obj->ymin + 1.0+ARROWHEIGHT;

    if ((vs->scenter + vs->shalf) > (obj->ymax - ARROWHEIGHT))
        vs->scenter = obj->ymax - ARROWHEIGHT - vs->shalf;
    if ((vs->scenter - vs->shalf) < (obj->ymin+ ARROWHEIGHT))
        vs->scenter = obj->ymin + ARROWHEIGHT + vs->shalf;
}

void	muiSetVSValue(muiObject *obj, float val)
{
    VSlider *vs = (VSlider *)obj->object;
    movevsval(obj, (float) val);
    vs->oldpos = vs->scenter;
}

/* 
 * visible is windowheight/dataheight
 * top is top/dataheight
 */

void
adjustvsthumb(muiObject *obj, double visible, double top)
{
    int size;
    
    if (visible >= 1.0) {
	    size = getvstrough(obj) + 1;
    } else {
	    size = visible*getvstrough(obj);
    }
    muiSetEnable(obj, 1);
    setvshalf(obj, size/2);
    muiSetVSValue(obj, (float) (1.0 - top));
}
