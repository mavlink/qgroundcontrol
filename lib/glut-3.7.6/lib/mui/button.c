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
#include <mui/displaylist.h>
#include <mui/uicolor.h>

void muiLinkButtons(muiObject *obj1, muiObject *obj2)
{
    Button *b1, *b2, *tmp;
    
    if ((obj1->type != MUI_RADIOBUTTON || obj2->type != MUI_RADIOBUTTON) &&
        (obj1->type != MUI_TINYRADIOBUTTON || obj2->type != MUI_TINYRADIOBUTTON)) {
	muiError("muiLinkButtons: attempt to link non radio buttons");
    }
    b1 = (Button *)obj1->object;
    b2 = (Button *)obj2->object;
    if (b1->link == 0 && b2->link == 0) {
	b1->link = b2;
	b2->link = b1;
	return;
    }
    if (b1->link == 0) {
	b1->link = b2->link;
	b2->link = b1;
	return;
    }
    if (b2->link == 0) {
	b2->link = b1->link;
	b1->link = b2;
	return;
    }
    tmp = b1->link;
    b1->link = b2->link;
    b2->link = tmp;
    return;
}

void     muiClearRadio(muiObject *rad)
{
    Button *b1, *b2;
    b2 = b1 = (Button *)rad->object;
    muiSetActive(b1->object, 0);
    if (b1->link == 0) return;
    b1 = b1->link;
    while (b1 != b2) {
	muiSetActive(b1->object, 0);
	b1 = b1->link;
    }
}


static void	drawbuttonlabel(muiObject *obj)
{
    int xmin, ymin;
    Button *b = (Button *)obj->object;

    if (!b->str)
	return;

    switch (obj->type) {
/*
	case TINYTOGGLE:
	    xmin = obj->xmin+16;
	    ymin = obj->ymin+4;
	    break;
	case GENERICBUTTON:
	    xmin = obj->xmin+ (obj->xmax - obj->xmin - strwidth(obj->str))/2 + 1;
	    ymin = obj->ymin+9;
	    break;
	case PUSHBUTTON:
	    if (getdefaultbut(b)) {
	    	xmin = obj->xmin+ (obj->xmax - 15 - obj->xmin - strwidth(obj->str))/2 + 1;
	    	ymin = obj->ymin+9;
	    } else {
	    	xmin = obj->xmin+ (obj->xmax - obj->xmin - strwidth(obj->str))/2 + 1;
	    	ymin = obj->ymin+9;
	    }
	    break;
    	case BED:
	    xmin = obj->xmin+ (obj->xmax - obj->xmin - strwidth(obj->str))/2 + 1;
	    ymin = obj->ymin+8;
	    break;
	case CHECKBUTTON:
	    xmin = obj->xmin+27;
	    ymin = obj->ymin+6;
	    break;
*/
	case MUI_TINYRADIOBUTTON:
	    xmin = obj->xmin+19;
	    ymin = obj->ymin+4;
	    break;
	case MUI_RADIOBUTTON:
	    xmin = obj->xmin+30;
	    ymin = obj->ymin+8;
	    break;
/*
	case INDICATOR:
	    xmin = obj->xmin+20;
	    ymin = obj->ymin+8;
	    break;
*/
    	default:
	    xmin = obj->xmin+20;
	    ymin = obj->ymin+9;
	    break;
    }
    {
    	if (muiGetEnable(obj))
            uiBlack();
    	else
	    uiDkGray();
	uicmov2i(xmin, ymin);
	uicharstr(b->str, UI_FONT_NORMAL);

    }
}

void	drawradiobutton(muiObject *obj)
{
    int xmin = obj->xmin, xmax = obj->xmax, ymin = obj->ymin, ymax = obj->ymax;

    if (!muiGetVisible(obj)) {
/*
    	int sl = 0;
    	font(BUTTONFONT1);
	if (b->str) sl = strwidth(b->str);
	backgrounddraw(b->xmin,b->ymin,b->xmin+30+sl,b->ymax);
*/
	return;
    }

    if (muiGetEnable(obj)) {
    	if (obj->locate) {
            if (obj->select) {
		if (obj->active) {
		    uiDkGray();
    	    	        uipmv2i(xmin,ymin+10); uipdr2i(xmin,ymin+13); 
			uipdr2i(xmin+10,ymax); 
    	    	        uipdr2i(xmin+13,ymax); uipdr2i(xmax,ymin+13); 							uipdr2i(xmax,ymin+10); 
    	    	        uipdr2i(xmin+13,ymin); uipdr2i(xmin+10,ymin); uipclos();
	
		    uiVyLtGray();
    	    	        uipmv2i(xmin+1,ymin+11); uipdr2i(xmin+1,ymin+13); 
	    	        uipdr2i(xmin+10,ymax-1); uipdr2i(xmin+13,ymax-1); 
	    	        uipdr2i(xmax-1,ymin+13); uipdr2i(xmax-1,ymin+11); 
    	    	        uipdr2i(xmin+13,ymin+2); uipdr2i(xmin+10,ymin+2); 
			uipclos();
    
		    uiWhite();
    	    	        uimove2i(xmin+1,ymin+12);  uidraw2i(xmin+11,ymax-1);  
    	    	        uidraw2i(xmin+12,ymax-1);  uidraw2i(xmax-1,ymin+12);
			uiendline();
    	    	        uimove2i(xmin+1,ymin+13);  uidraw2i(xmin+10,ymax-1);  
    	    	        uidraw2i(xmin+13,ymax-1);  uidraw2i(xmax-1,ymin+13);
			uiendline();

		} else {

		    uiDkGray();
    	    	        uimove2i(xmin,ymin+13);   uidraw2i(xmin+10,ymax);  
    	    	        uidraw2i(xmin+13,ymax);   uidraw2i(xmax,ymin+13);
			uiendline();
    	    	        uimove2i(xmin,ymin+12);   uidraw2i(xmin+10,ymax-1);  
    	    	        uidraw2i(xmin+13,ymax-1); uidraw2i(xmax,ymin+12);
			uiendline();

	            uiLtGray();
    	    	        uimove2i(xmin+1,ymin+12); uidraw2i(xmin+11,ymax-1);  
    	    	        uidraw2i(xmin+12,ymax-1); uidraw2i(xmax-1,ymin+12);
			uiendline();

		    uiWhite();
    	    	        uimove2i(xmin+2,ymin+12); uidraw2i(xmin+11,ymax-2);  
    	    	        uidraw2i(xmin+12,ymax-2); uidraw2i(xmax-2,ymin+12);
			uiendline();
    	    	        uimove2i(xmin+3,ymin+12); uidraw2i(xmin+11,ymax-3);  
    	    	        uidraw2i(xmin+12,ymax-3); uidraw2i(xmax-3,ymin+12);
			uiendline();

		    uiVyLtGray();
    	    	        uipmv2i(xmin+4,ymin+12); uipdr2i(xmin+11,ymax-4); 
	    	        uipdr2i(xmin+12,ymax-4); uipdr2i(xmax-4,ymin+12); 
	    	        uipdr2i(xmin+12,ymin+5); uipdr2i(xmin+11,ymin+5);
		        uipclos();

		    uiLtGray();
    	    	        uimove2i(xmin+3,ymin+11);  uidraw2i(xmin+11,ymin+3);  
    	    	        uidraw2i(xmin+12,ymin+3);  uidraw2i(xmax-3,ymin+11);
			uiendline();
    	    	        uimove2i(xmin+4,ymin+11);  uidraw2i(xmin+11,ymin+4);  
    	    	        uidraw2i(xmin+12,ymin+4);  uidraw2i(xmax-4,ymin+11);
			uiendline();

		    uiBlack();
    	    	        uimove2i(xmin+2,ymin+11);  uidraw2i(xmin+11,ymin+2);  
    	    	        uidraw2i(xmin+12,ymin+2);  uidraw2i(xmax-2,ymin+11);
			uiendline();

		    uiWhite();
    	    	        uimove2i(xmin,ymin+10);  uidraw2i(xmin+10,ymin);  
    	    	        uidraw2i(xmin+13,ymin);  uidraw2i(xmax,ymin+10);
			uiendline();
    	    	        uimove2i(xmin,ymin+11);  uidraw2i(xmin+10,ymin+1);  
    	    	        uidraw2i(xmin+13,ymin+1);uidraw2i(xmax,ymin+11);
			uiendline();
    	    	        uimove2i(xmin+1,ymin+11);uidraw2i(xmin+10,ymin+2);  
    	    	        uimove2i(xmin+13,ymin+2);uidraw2i(xmax-1,ymin+11);
			uiendline();  
		}

	    } else {		/* not selected */

		if (obj->active) {

		    uiDkGray();
    	    	        uimove2i(xmin,ymin+13);   uidraw2i(xmin+10,ymax);  
    	    	        uidraw2i(xmin+13,ymax);   uidraw2i(xmax,ymin+13);
			uiendline();
    	    	        uimove2i(xmin,ymin+12);   uidraw2i(xmin+10,ymax-1);  
    	    	        uidraw2i(xmin+13,ymax-1); uidraw2i(xmax,ymin+12);
			uiendline();

	            uiLtGray();
    	    	        uimove2i(xmin+1,ymin+12); uidraw2i(xmin+11,ymax-1);  
    	    	        uidraw2i(xmin+12,ymax-1); uidraw2i(xmax-1,ymin+12);
			uiendline();

		    uiWhite();
    	    	        uimove2i(xmin+2,ymin+12); uidraw2i(xmin+11,ymax-2);  
    	    	        uidraw2i(xmin+12,ymax-2); uidraw2i(xmax-2,ymin+12);
			uiendline();
    	    	        uimove2i(xmin+3,ymin+12); uidraw2i(xmin+11,ymax-3);  
    	    	        uidraw2i(xmin+12,ymax-3); uidraw2i(xmax-3,ymin+12);
			uiendline();

		    uiVyLtGray();
    	    	        uipmv2i(xmin+4,ymin+12); uipdr2i(xmin+11,ymax-4); 
	    	        uipdr2i(xmin+12,ymax-4); uipdr2i(xmax-4,ymin+12); 
	    	        uipdr2i(xmin+12,ymin+5); uipdr2i(xmin+11,ymin+5);
		        uipclos();

		    uiLtGray();
    	    	        uimove2i(xmin+3,ymin+11);  uidraw2i(xmin+11,ymin+3);  
    	    	        uidraw2i(xmin+12,ymin+3);  uidraw2i(xmax-3,ymin+11);
			uiendline();
    	    	        uimove2i(xmin+4,ymin+11);  uidraw2i(xmin+11,ymin+4);  
    	    	        uidraw2i(xmin+12,ymin+4);  uidraw2i(xmax-4,ymin+11);
			uiendline();

		    uiBlack();
    	    	        uimove2i(xmin+2,ymin+11);  uidraw2i(xmin+11,ymin+2);  
    	    	        uidraw2i(xmin+12,ymin+2);  uidraw2i(xmax-2,ymin+11);
			uiendline();

		    uiWhite();
    	    	        uimove2i(xmin,ymin+10);  uidraw2i(xmin+10,ymin);  
    	    	        uidraw2i(xmin+13,ymin);  uidraw2i(xmax,ymin+10);
			uiendline();
    	    	        uimove2i(xmin,ymin+11);  uidraw2i(xmin+10,ymin+1);  
    	    	        uidraw2i(xmin+13,ymin+1);uidraw2i(xmax,ymin+11);
			uiendline();
    	    	        uimove2i(xmin+1,ymin+11);uidraw2i(xmin+10,ymin+2);  
    	    	        uimove2i(xmin+13,ymin+2);uidraw2i(xmax-1,ymin+11); 
			uiendline(); 

		} else {	/* not active */

		    uiDkGray();
    	    	        uipmv2i(xmin,ymin+10); uipdr2i(xmin,ymin+13); 
			uipdr2i(xmin+10,ymax); 
    	    	        uipdr2i(xmin+13,ymax); uipdr2i(xmax,ymin+13); 							uipdr2i(xmax,ymin+10); 
    	    	        uipdr2i(xmin+13,ymin); uipdr2i(xmin+10,ymin); uipclos();
	
		    uiVyLtGray();
    	    	        uipmv2i(xmin+1,ymin+11); uipdr2i(xmin+1,ymin+13); 
	    	        uipdr2i(xmin+10,ymax-1); uipdr2i(xmin+13,ymax-1); 
	    	        uipdr2i(xmax-1,ymin+13); uipdr2i(xmax-1,ymin+11); 
    	    	        uipdr2i(xmin+13,ymin+2); uipdr2i(xmin+10,ymin+2); 
			uipclos();
    
		    uiWhite();
    	    	        uimove2i(xmin+1,ymin+12);  uidraw2i(xmin+11,ymax-1);  
    	    	        uidraw2i(xmin+12,ymax-1);  uidraw2i(xmax-1,ymin+12);
			uiendline();
    	    	        uimove2i(xmin+1,ymin+13);  uidraw2i(xmin+10,ymax-1);  
    	    	        uidraw2i(xmin+13,ymax-1);  uidraw2i(xmax-1,ymin+13);
			uiendline();
		}
	    }

	} else {	/* not located */

	    if (obj->active) {

		uiDkGray();
    	    	    uimove2i(xmin,ymin+13);   uidraw2i(xmin+10,ymax);  
    	    	    uidraw2i(xmin+13,ymax);   uidraw2i(xmax,ymin+13);
		    uiendline();
    	    	    uimove2i(xmin,ymin+12);   uidraw2i(xmin+10,ymax-1);  
    	    	    uidraw2i(xmin+13,ymax-1); uidraw2i(xmax,ymin+12);
		    uiendline();

	        uiLtGray();
    	    	    uimove2i(xmin+1,ymin+12); uidraw2i(xmin+11,ymax-1);  
    	    	    uidraw2i(xmin+12,ymax-1); uidraw2i(xmax-1,ymin+12);
		    uiendline();

		uiWhite();
    	    	    uimove2i(xmin+2,ymin+12); uidraw2i(xmin+11,ymax-2);  
    	    	    uidraw2i(xmin+12,ymax-2); uidraw2i(xmax-2,ymin+12);
		    uiendline();
    	    	    uimove2i(xmin+3,ymin+12); uidraw2i(xmin+11,ymax-3);  
    	    	    uidraw2i(xmin+12,ymax-3); uidraw2i(xmax-3,ymin+12);
		    uiendline();

		uiLtGray();
    	    	    uipmv2i(xmin+4,ymin+12); uipdr2i(xmin+11,ymax-4); 
	    	    uipdr2i(xmin+12,ymax-4); uipdr2i(xmax-4,ymin+12); 
	    	    uipdr2i(xmin+12,ymin+5); uipdr2i(xmin+11,ymin+5); uipclos();

		uiLtGray();
    	    	    uimove2i(xmin+3,ymin+11);  uidraw2i(xmin+11,ymin+3);  
    	    	    uidraw2i(xmin+12,ymin+3);  uidraw2i(xmax-3,ymin+11);
		    uiendline();
    	    	    uimove2i(xmin+4,ymin+11);  uidraw2i(xmin+11,ymin+4);  
    	    	    uidraw2i(xmin+12,ymin+4);  uidraw2i(xmax-4,ymin+11);
		    uiendline();

		uiBlack();
    	    	    uimove2i(xmin+2,ymin+11);  uidraw2i(xmin+11,ymin+2);  
    	    	    uidraw2i(xmin+12,ymin+2);  uidraw2i(xmax-2,ymin+11);
		    uiendline();

		uiWhite();
    	    	    uimove2i(xmin,ymin+10);  uidraw2i(xmin+10,ymin);  
    	    	    uidraw2i(xmin+13,ymin);  uidraw2i(xmax,ymin+10);
		    uiendline();
    	    	    uimove2i(xmin,ymin+11);  uidraw2i(xmin+10,ymin+1);  
    	    	    uidraw2i(xmin+13,ymin+1);uidraw2i(xmax,ymin+11);
		    uiendline();
    	    	    uimove2i(xmin+1,ymin+11);uidraw2i(xmin+10,ymin+2);  
    	    	    uimove2i(xmin+13,ymin+2);uidraw2i(xmax-1,ymin+11); 
		    uiendline(); 

	    } else {

	    	uiDkGray();
    	            uipmv2i(xmin,ymin+10); uipdr2i(xmin,ymin+13); 
		    uipdr2i(xmin+10,ymax);
    	            uipdr2i(xmin+13,ymax); uipdr2i(xmax,ymin+13); 
		    uipdr2i(xmax,ymin+10);
    	            uipdr2i(xmin+13,ymin); uipdr2i(xmin+10,ymin); uipclos();
	
	    	uiLtGray();
    	          uipmv2i(xmin+1,ymin+11); uipdr2i(xmin+1,ymin+13); 
	          uipdr2i(xmin+10,ymax-1); uipdr2i(xmin+13,ymax-1); 
	          uipdr2i(xmax-1,ymin+13); uipdr2i(xmax-1,ymin+11); 
    	          uipdr2i(xmin+13,ymin+2); uipdr2i(xmin+10,ymin+2); uipclos();
	
	    	uiWhite();
    	          uimove2i(xmin+1,ymin+12);  uidraw2i(xmin+11,ymax-1);  
    	          uidraw2i(xmin+12,ymax-1);  uidraw2i(xmax-1,ymin+12);
		  uiendline();
	    }
	}

    } else {		/* not enabled */
	uiDkGray();
    	    uipmv2i(xmin,ymin+10); uipdr2i(xmin,ymin+13); uipdr2i(xmin+10,ymax); 
    	    uipdr2i(xmin+13,ymax); uipdr2i(xmax,ymin+13); uipdr2i(xmax,ymin+10); 
    	    uipdr2i(xmin+13,ymin); uipdr2i(xmin+10,ymin); uipclos();

	uiLtGray();
            uipmv2i(xmin+1,ymin+10); uipdr2i(xmin+1,ymin+13); 
	    uipdr2i(xmin+10,ymax-1); uipdr2i(xmin+13,ymax-1); 
	    uipdr2i(xmax-1,ymin+13); uipdr2i(xmax-1,ymin+10); 
    	    uipdr2i(xmin+13,ymin+1); uipdr2i(xmin+10,ymin+1); uipclos();
    }

    if (obj->active) {
	if (muiGetEnable(obj)) {
	    uiSlateBlue();
       	        uipmv2i(xmin+12,ymax-5); uipdr2i(xmax-6,ymin+13); 
	        uipdr2i(xmax-8,ymin+13); uipdr2i(xmin+12,ymax-7); uipclos();

	    uiBlue();
       	        uipmv2i(xmin+12,ymax-8); uipdr2i(xmax-9,ymin+13); 
	        uipdr2i(xmax-9,ymin+12); uipdr2i(xmin+12,ymin+10); uipclos();

	    uiBlack();
       	        uipmv2i(xmin+12,ymin+9); uipdr2i(xmin+15,ymin+12); 
	        uipdr2i(xmax-6,ymin+12); uipdr2i(xmin+13,ymin+8); 
	        uipdr2i(xmin+12,ymin+8); uipclos();

	    uiDkGray();
    	        uimove2i(xmin+13,ymin+7);  uidraw2i(xmax-5,ymin+12);  
		uiendline();
	} else {
	    uiSlateBlue();
       	        uipmv2i(xmin+12,ymax-5); uipdr2i(xmax-6,ymin+13); 
	        uipdr2i(xmax-6,ymin+12); uipdr2i(xmin+13,ymin+8); 
	        uipdr2i(xmin+12,ymin+8); uipclos();

	    uiBlack();
    	        uimove2i(xmin+13,ymin+7);  uidraw2i(xmax-5,ymin+12);  
		uiendline();
	}
    }
    drawbuttonlabel(obj);
}

void	drawtinyradio(muiObject *obj)
{
    int xmin = obj->xmin, xmax = obj->xmax,
	 ymin = obj->ymin, ymax = obj->ymax;
    int sxmin = obj->xmin+4, sxmax = obj->xmin+13, sxmid = obj->xmin+8,
	 symin = obj->ymin+3, symax = obj->ymin+12, symid = obj->ymin+7;

    if (!muiGetVisible(obj)) {
	backgrounddraw(obj->xmin,obj->ymin,obj->xmax,obj->ymax);
	return;
    }

    if (muiGetEnable(obj) && obj->locate) {
	if (obj->select) {
    	    drawedges(xmin++,xmax--,ymin++,ymax--,uiBlack,uiWhite);
    	    drawedges(xmin++,xmax--,ymin++,ymax--,uiDkGray,uiBlack);
    	    drawedges(xmin++,xmax--,ymin++,ymax--,uiWhite,uiMmGray);

    	    uiVyLtGray();
    	        uirectfi(xmin,ymin,xmax,ymax);
	} else {
    	    uiWhite();
    	        uirectfi(xmin,ymin,xmax,ymax);
	}
    } else {
    	uiBackground();
    	    uirectfi(xmin,ymin,xmax,ymax);
    }

	
    uiBlack(); 
        uipmv2i(sxmin, symid); uipdr2i(sxmin++, symid+1); 
	uipdr2i(sxmid, symax); uipdr2i(sxmid+1, symax--); uipdr2i(sxmax, symid+1);
    	uipclos();
	
    uiLtGray(); 
        uipmv2i(sxmax--, symid); uipdr2i(sxmid+1, symin); 
	uipdr2i(sxmid, symin++); uipdr2i(sxmin, symid-1);
    	uipclos();

    if (obj->active) {
	if (!muiGetEnable(obj))
	    uiLtYellow();
        else 
	    uiYellow();
    } else {
	if (!muiGetEnable(obj))
	    uiMmGray();
	else if (obj->locate)
	    uiMmYellow();
	else 
	    uiDkYellow();
    }
    uipmv2i(sxmin, symid); uipdr2i(sxmin, symid+1);
    uipdr2i(sxmid, symax); uipdr2i(sxmid+1, symax); uipdr2i(sxmax, symid+1);
    uipdr2i(sxmax, symid); uipdr2i(sxmid+1, symin); uipdr2i(sxmid, symin);
    uipclos();

    drawbuttonlabel(obj);
}

