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
#include <mui/hslider.h>
#include <mui/displaylist.h>
#include <mui/uicolor.h>

extern	HSlider *locatedhs;

int gethstrough(muiObject *obj)
{
    return obj->xmax - obj->xmin - 2*ARROWHEIGHT;
}

void	sethscenter(muiObject *obj, int scenter)
{
    HSlider *hs = (HSlider *)obj->object;
    if ((scenter - hs->shalf) < (obj->xmin+ARROWHEIGHT))
        hs->scenter = gethstrough(obj)/2 + obj->xmin+ARROWHEIGHT;
    else
        hs->scenter = scenter;
}

void	muiSetHSArrowDelta(muiObject *obj, int newd)
{
    HSlider *hs = (HSlider *)obj->object;
    hs->arrowdelta = newd;
}

HSlider	*newhs(muiObject *obj, int xmin, int xmax, int scenter, int shalf)
{
    HSlider	*hs;

    hs = (HSlider *)malloc(sizeof(HSlider));
    obj->object = (HSlider *)hs;
    if (shalf == 0) {
	hs->shalf = 0;
    } else if (shalf < MINSHALF) 
	hs->shalf = MINSHALF;
    else
	hs->shalf = shalf;

    if ((xmax - xmin + 1) <= (2*ARROWHEIGHT+2*MINSHALF))
    	hs->thumb = 0;
    else
    	hs->thumb = 1;

    sethscenter(obj, scenter);
    hs->oldpos = hs->scenter;

    muiSetHSArrowDelta(obj, 1);
    return hs;
}

void	freehs(HSlider *hs)
{
    if (hs) {
	free(hs);
    }
}

void	drawhsarrows(muiObject *obj)
{
    int	ymin = obj->ymin, ymax = obj->ymin+ARROWHEIGHT,
		xmin = obj->xmin, xmax = obj->xmax;

    if (!muiGetVisible(obj))
        return;

    /* Draw the arrows: */

    /* down arrow */
    uiDkGray();
    	uirecti(xmin,ymin,xmin+20,ymax);

    if (muiGetVisible(obj)) {
        if (obj->locate == SCROLLDOWN) {
    	    if (obj->select == SCROLLDOWN) {
    	        drawedges(xmin+1,xmin+19,ymin+1,ymax-1,uiMmGray,uiWhite);
    	        drawedges(xmin+2,xmin+18,ymin+2,ymax-2,uiLtGray,uiWhite);
	    } else {
    	        drawedges(xmin+1,xmin+19,ymin+1,ymax-1,uiWhite,uiMmGray);
    	        drawedges(xmin+2,xmin+18,ymin+2,ymax-2,uiWhite,uiLtGray);
	    }
    	    uiVyLtGray();
        	uirectfi(xmin+3,ymin+3,xmin+17,ymax-3);
        } else {
    	    if (obj->select == SCROLLDOWN) {
    	        drawedges(xmin+1,xmin+19,ymin+1,ymax-1,uiMmGray,uiVyLtGray);
    	        drawedges(xmin+2,xmin+18,ymin+2,ymax-2,uiMmGray,uiVyLtGray);
	    } else {
    	        drawedges(xmin+1,xmin+19,ymin+1,ymax-1,uiWhite,uiMmGray);
    	        drawedges(xmin+2,xmin+18,ymin+2,ymax-2,uiVyLtGray,uiMmGray);
	    }
    	    uiLtGray();
        	uirectfi(xmin+3,ymin+3,xmin+17,ymax-3);
        }
    } else {
        drawedges(xmin+1,xmin+19,ymin+1,ymax-1,uiVyLtGray,uiMmGray);
    	uiLtGray();
            uirectfi(xmin+2,ymin+2,xmin+18,ymax-2);
    }

    /* arrow XXX probably wrong for hsliders XXX */
    if (muiGetEnable(obj))
        uiDkGray();
    else
        uiMmGray();
    uimove2i(xmin+14, ymin+5);
    uidraw2i(xmin+14, ymin+14);
    uiendline();
    uirectfi(xmin+13,ymin+6,xmin+12,ymin+13);
    uirectfi(xmin+11,ymin+7,xmin+10,ymin+12);
    uirectfi(xmin+8,ymin+8,xmin+9,ymin+11);
    uirectfi(xmin+6,ymin+9,xmin+7,ymin+10);
   
    /* up arrow */
    uiDkGray();
    	uirecti(xmax-20,ymin,xmax,ymax);

    if (muiGetEnable(obj)) {
        if (obj->locate == SCROLLUP) {
    	    if (obj->select == SCROLLUP) {
    	        drawedges(xmax-19,xmax-1,ymin+1,ymax-1,uiMmGray,uiWhite);
    	        drawedges(xmax-18,xmax-2,ymin+2,ymax-2,uiLtGray,uiWhite);
	    } else {
    	        drawedges(xmax-19,xmax-1,ymin+1,ymax-1,uiWhite,uiMmGray);
    	        drawedges(xmax-18,xmax-2,ymin+2,ymax-2,uiWhite,uiLtGray);
	    }
    	    uiVyLtGray();
        	uirectfi(xmax-17,ymin+3,xmax-3,ymax-3);
        } else {
    	    if (obj->select == SCROLLUP) {
    	        drawedges(xmax-19,xmax-1,ymin+1,ymax-1,uiMmGray,uiVyLtGray);
    	        drawedges(xmax-18,xmax-2,ymin+2,ymax-2,uiMmGray,uiVyLtGray);
	    } else {
    	        drawedges(xmax-19,xmax-1,ymin+1,ymax-1,uiWhite,uiMmGray);
    	        drawedges(xmax-18,xmax-2,ymin+2,ymax-2,uiVyLtGray,uiMmGray);
	    }
    	    uiLtGray();
        	uirectfi(xmax-17,ymin+3,xmax-3,ymax-3);
        }
    } else {
	drawedges(xmin+1,xmax-1,ymin+1,ymax-1,uiVyLtGray,uiMmGray);
    	uiLtGray();
	    uirectfi(xmax-18,ymin+2,xmax-2,ymax-2);
    }

    /* arrow XXX probably wrong for hslider XXX */
    if (muiGetEnable(obj))
        uiDkGray();
    else
        uiMmGray();
    uirectfi(xmax-6,ymin+9,xmax-7,ymin+10);
    uirectfi(xmax-8,ymin+8,xmax-9,ymin+11);
    uirectfi(xmax-10,ymin+7,xmax-11,ymin+12);
    uirectfi(xmax-12,ymin+6,xmax-13,ymin+13);
    uimove2i(xmax-14, ymin+5);
    uidraw2i(xmax-14, ymin+14);
    uiendline();
}

void	drawhs(muiObject *obj)
{
    HSlider *hs = (HSlider *)obj->object;

    int	ymin = obj->ymin, xmax = obj->xmax-ARROWHEIGHT, 
		xmin = obj->xmin+ARROWHEIGHT;
    int 	ymax = ymin+SLIDERWIDTH;
    int	sxmin = hs->scenter - hs->shalf;
    int	sxmax = hs->scenter + hs->shalf;
    int	oldsxmin = hs->oldpos - hs->shalf;
    int	oldsxmax = hs->oldpos + hs->shalf;

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

    if (hs->thumb) {
        /* last thumb position */
        if ((hs->oldpos != hs->scenter) && (obj->enable)) {
	
    	    uiDkGray();
	        uimove2i(oldsxmax, ymax-2);
	        uidraw2i(oldsxmax, ymin+1);
		uiendline();
    
    	    uiMmGray();
	        uimove2i(oldsxmax, ymin+1);
	        uidraw2i(oldsxmin, ymin+1);
		uiendline();
    	
    	    uiVyLtGray();
	        uimove2i(oldsxmin, ymin+2);
	        uidraw2i(oldsxmin, ymax-1);
		uiendline();
    
    	    uiLtGray();
	        uimove2i(oldsxmin, ymax-1);
	        uidraw2i(oldsxmax, ymax-1);
		uiendline();

    	    uiVyLtGray();
	        uimove2i(--oldsxmax, ymax-2);
	        uidraw2i(++oldsxmin, ymax-2);
	        uidraw2i(oldsxmin, ymin+2);
		uiendline();
    
    	    uiVyDkGray();
	        uimove2i(oldsxmin, ymin+2);
	        uidraw2i(oldsxmax, ymin+2);
	        uidraw2i(oldsxmax, ymax-3);
		uiendline();
    
    	    uiDkGray();
	        uimove2i(--oldsxmax, ymin+3);
	        uidraw2i(oldsxmax, ymax-3);
		uiendline();
    	
    	    uiLtGray();
	        uimove2i(oldsxmax, ymax-3);
	        uidraw2i(++oldsxmin, ymax-3);
	        uidraw2i(oldsxmin, ymin+3);
		uiendline();
    
    	    uiMmGray();
	        uirectfi(++oldsxmin, ymin+3, --oldsxmax, ymax-4);
    	
        }

        if (obj->enable) {

    	    /* thumb */
    	    uiDkGray();
    	        uirecti(sxmin,ymin,sxmax,ymax);
    	    if (obj->locate == THUMB) {
    		drawedges(sxmin+1,sxmax-1,ymin+1,ymax-1,uiWhite,uiDkGray);
    		drawedges(sxmin+2,sxmax-2,ymin+2,ymax-2,uiWhite,uiLtGray);
    		drawedges(sxmin+3,sxmax-3,ymin+3,ymax-3,uiWhite,uiLtGray);
    
    		uiVyLtGray();
    		    uirectfi(sxmin+4, ymin+4, sxmax-4, ymax-4);
    	
    		/* ridges on thumb */
    		uiDkGray();
    		    uimove2i(hs->scenter, ymin+3);
    		    uidraw2i(hs->scenter, ymax-3);
		    uiendline();
    		    uimove2i(hs->scenter-4, ymin+3);
    		    uidraw2i(hs->scenter-4, ymax-3);
		    uiendline();
    		    uimove2i(hs->scenter+4, ymin+3);
    		    uidraw2i(hs->scenter+4, ymax-3);
		    uiendline();
   
    		uiWhite();
		    uirectfi(hs->scenter+1,ymin+3,hs->scenter+2,ymax-3);
		    uirectfi(hs->scenter+5,ymin+3,hs->scenter+6,ymax-3);
		    uirectfi(hs->scenter-2,ymin+3,hs->scenter-3,ymax-3);
    	    } else {
    		drawedges(sxmin+1,sxmax-1,ymin+1,ymax-1,uiWhite,uiDkGray);
    		drawedges(sxmin+2,sxmax-2,ymin+2,ymax-2,uiVyLtGray,uiMmGray);
    		drawedges(sxmin+3,sxmax-3,ymin+3,ymax-3,uiVyLtGray,uiMmGray);
   
      		uiLtGray();
          	    uirectfi(sxmin+4, ymin+4, sxmax-4, ymax-4);
	    	
    		/* ridges on thumb */
    		uiBlack();
    		    uimove2i(hs->scenter, ymin+3);
    		    uidraw2i(hs->scenter, ymax-3);
		    uiendline();
    		    uimove2i(hs->scenter-4, ymin+3);
    		    uidraw2i(hs->scenter-4, ymax-3);
		    uiendline();
    		    uimove2i(hs->scenter+4, ymin+3);
    		    uidraw2i(hs->scenter+4, ymax-3);
		    uiendline();

    		uiWhite();
    		    uimove2i(hs->scenter+1, ymin+3);
    		    uidraw2i(hs->scenter+1, ymax-3);
		    uiendline();
    		    uimove2i(hs->scenter-3, ymin+3);
    		    uidraw2i(hs->scenter-3, ymax-3);
		    uiendline();
    		    uimove2i(hs->scenter+5, ymin+3);
    		    uidraw2i(hs->scenter+5, ymax-3);
		    uiendline();
    	    }
    	}
    }
    drawhsarrows(obj);

    drawrestore();
}

enum muiReturnValue hshandler(muiObject *obj, int event, int value, int x, int y)
{
    int my = x;
    static int mfudge=0;
    static enum muiReturnValue retval = MUI_NO_ACTION;
    HSlider *hs = (HSlider *)obj->object;

    if (!muiGetEnable(obj) || !muiGetVisible(obj))
	return MUI_NO_ACTION;
    switch (event) {
	case MUI_DEVICE_RELEASE:
	    if (value == 0) { 
		hs->oldpos = hs->scenter; 
		muiSetSelect(obj, 0);
		return MUI_SLIDER_RETURN; 
	    }
	case MUI_DEVICE_PRESS:
	case MUI_DEVICE_CLICK:
	    /* in the arrows */
	    if (my >= obj->xmin && my <= obj->xmax && 
	       (my < obj->xmin+ARROWHEIGHT || my > obj->xmax-ARROWHEIGHT)) {
		mfudge = -10000;
		if (my < obj->xmin+ARROWHEIGHT) { /* boink down */
		    my = hs->scenter - hs->arrowdelta;
		    retval = MUI_SLIDER_SCROLLDOWN;
		} else { /* boink up */
		    my = hs->scenter + hs->arrowdelta;
		    retval = MUI_SLIDER_SCROLLUP;
		}
		if (event == MUI_DEVICE_CLICK) {
		    muiSetSelect(obj, 0);
		    retval = MUI_SLIDER_RETURN;
		}
		if (my - hs->shalf < obj->xmin+1+ARROWHEIGHT)
		    my = obj->xmin+1+hs->shalf+ARROWHEIGHT;
		if (my + hs->shalf > obj->xmax-1-ARROWHEIGHT) 
		    my = obj->xmax-1-hs->shalf-ARROWHEIGHT;
		hs->scenter = my; 
		break;
	    } else if (my >= obj->xmin && my <= obj->xmax)
	    	retval = MUI_SLIDER_THUMB;
	    hs->oldpos = hs->scenter;
	    if (my >= hs->scenter-hs->shalf && my <= hs->scenter+hs->shalf)
		mfudge = hs->scenter - my;
	    else
		mfudge = 0;
	    break;
	case MUI_DEVICE_DOWN:
	    if (mfudge == -10000) {	/* auto - repeat the arrow keys */
                if (retval == MUI_SLIDER_SCROLLDOWN) {
                    my = hs->scenter - hs->arrowdelta;
                    if (my - hs->shalf < obj->xmin+1+ARROWHEIGHT)
                        my = obj->xmin+1+hs->shalf+ARROWHEIGHT;
                } else {
                    my = hs->scenter + hs->arrowdelta;
                    if (my + hs->shalf > obj->xmax-1-ARROWHEIGHT)
                        my = obj->xmax-1-hs->shalf-ARROWHEIGHT;
                }
                hs->scenter = my;
                break;
	    }
	    my = x+mfudge;
	    if (my - hs->shalf < obj->xmin+1+ARROWHEIGHT)
		my = obj->xmin+1+hs->shalf+ARROWHEIGHT;
	    if (my + hs->shalf > obj->xmax-1-ARROWHEIGHT) 
		my = obj->xmax-1-hs->shalf-ARROWHEIGHT;

	    /* adjust thumb */
	    hs->scenter = my; 
	    break;
    }
    y = y;	/* for lint's sake */
    return retval;
}

float muiGetHSVal(muiObject *obj)
{
    HSlider *hs = (HSlider *)obj->object;

    return (hs->scenter-obj->xmin-1.0-hs->shalf-ARROWHEIGHT)/
		(obj->xmax - obj->xmin - 2.0*hs->shalf - 2.0-2*ARROWHEIGHT);
}

void	sethshalf(muiObject *obj, int shalf)
{
    HSlider *hs = (HSlider *)obj->object;
    hs->shalf = shalf;
    if (hs->shalf==0)
	muiSetEnable(obj, 0);
    else if (hs->shalf < MINSHALF) {
        hs->shalf = MINSHALF;
	muiSetEnable(obj, 1);
    } else if (2*hs->shalf >= gethstrough(obj)) {
        hs->shalf = gethstrough(obj)/2;
	muiSetEnable(obj, 0);
    }
}

void	movehsval(muiObject *obj, float val)
{
    float	f;
    HSlider *hs = (HSlider *)obj->object;

    if (val < 0.0) val = 0.0;
    if (val > 1.0) val = 1.0;
    f = val*(obj->xmax - obj->xmin - 2.0*hs->shalf - 2.0-2*ARROWHEIGHT);
    hs->scenter = f + hs->shalf + obj->xmin + 1.0+ARROWHEIGHT;

    if ((hs->scenter + hs->shalf) > (obj->xmax - ARROWHEIGHT))
        hs->scenter = obj->xmax - ARROWHEIGHT - hs->shalf;
    if ((hs->scenter - hs->shalf) < (obj->xmin+ ARROWHEIGHT))
        hs->scenter = obj->xmin + ARROWHEIGHT + hs->shalf;
}

void	muiSetHSValue(muiObject *obj, float val)
{
    HSlider *hs = (HSlider *)obj->object;
    movehsval(obj, (float) val);
    hs->oldpos = hs->scenter;
}

/* 
 * visible is windowheight/dataheight
 * top is top/dataheight
 */

void
adjusthsthumb(muiObject *obj, double visible, double top)
{
    int size;
    
    if (visible >= 1.0) {
	    size = gethstrough(obj) + 1;
    } else {
	    size = visible*gethstrough(obj);
    }
    muiSetEnable(obj, 1);
    sethshalf(obj, size/2);
    muiSetHSValue(obj, (float) (1.0 - top));
}
