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

#include <stdio.h>
#include <GL/glut.h>
#include <mui/mui.h>

extern int mui_singlebuffered;

char *strs[] = { "Line 1", "Line 2", "Longer line 3", "Very, very, very long line number four", 
	    "Line 5", "Sixth line", "Seventh line", "Line number 8",  "9", "10", "The eleventh line", 
	    "line 12", "Line number 13", "Quite long line 14", "15",
	    "Line 1", "Line 2", "Longer line 3", "Very, very, very long line number four", 
	    "Line 5", "Sixth line", "Seventh line", "Line number 8",  "9", "10", "The eleventh line", 
	    "line 12", "Line number 13", "Quite long line 14", "15", 0, };

muiObject *b1, *b2, *b3, *rb1, *rb2, *rb3, *t, *t1, *l, *l1, *vs, *tl;
muiObject *trb1, *trb2, *trb3, *pd, *hs;

int M1, M2, M3;	/* menus */

void	controltltop(muiObject *obj, enum muiReturnValue value)
{
    float sliderval;

    if ((value != MUI_SLIDER_RETURN) && (value != MUI_SLIDER_THUMB)) return;
    sliderval = muiGetVSVal(obj);
    muiSetTLTop(tl, sliderval);
}

#define THUMBHEIGHT 20
#define ARROWSPACE 40

void menucallback(int x)
{
    printf("menu callback: %d\n", x);
}

void maketestmenus(void)
{
    M1 = glutCreateMenu(menucallback);
    glutAddMenuEntry("Open", 1);
    glutAddMenuEntry("Close", 2);
    glutAddMenuEntry("Read", 3);

    M2 = glutCreateMenu(menucallback);
    glutAddMenuEntry("Edit", 4);
    glutAddMenuEntry("Cut", 5);
    glutAddMenuEntry("Paste", 6);

    M3 = glutCreateMenu(menucallback);
    glutAddMenuEntry("Help1", 7);
    glutAddMenuEntry("Help2", 8);
    glutAddMenuEntry("Help3", 9);

}

void bcallback(muiObject *obj, enum muiReturnValue r)
{
    obj = obj; r = r; /* for lint's sake */
    printf("Test 3 callback\n");
}

void maketestui(void)
{
    int xmin, ymin, xmax, ymax;

    maketestmenus();
    muiNewUIList(1);	/* makes an MUI display list (number 1) */
    b1 = muiNewButton(10, 100, 10, 35);
    b2 = muiNewButton(10, 100, 40, 65);
    b3 = muiNewButton(10, 100, 70, 95);
    rb1 = muiNewRadioButton(10, 380);
    muiLoadButton(rb1, "Radio1");
    rb2 = muiNewRadioButton(10, 350);
    muiLoadButton(rb2, "Radio2");
    rb3 = muiNewRadioButton(10, 320);
    muiLoadButton(rb3, "Radio3");
    muiLinkButtons(rb1, rb2);
    muiLinkButtons(rb2, rb3);
    trb1 = muiNewTinyRadioButton(10, 450);
    muiLoadButton(trb1, "TRadio1");
    trb2 = muiNewTinyRadioButton(10, 430);
    muiLoadButton(trb2, "TRadio2");
    trb3 = muiNewTinyRadioButton(10, 410);
    muiLoadButton(trb3, "TRadio3");
    muiLinkButtons(trb1, trb2);
    muiLinkButtons(trb2, trb3);
    t = muiNewTextbox(110, 250, 50);
    muiSetActive(t, 1);
    t1 = muiNewTextbox(110, 270, 20);
    l = muiNewLabel(110, 85, "Label");
    l1 = muiNewBoldLabel(110, 110, "Bold Label");
    tl = muiNewTextList(20, 120, 200, 9);
    muiGetObjectSize(tl, &xmin, &ymin, &xmax, &ymax);
    vs = muiNewVSlider(xmax, ymin+2, ymax, 0, THUMBHEIGHT);
    hs = muiNewHSlider(20, 290, 280, 0, THUMBHEIGHT);
    muiSetVSValue(vs, 1.0);
    muiSetVSArrowDelta(vs, (ymax-ymin-10-THUMBHEIGHT-ARROWSPACE)/((sizeof strs)/(sizeof (char *))-9));
    muiLoadButton(b1, "Test");
    muiLoadButton(b2, "Test22");
    muiLoadButton(b3, "Test3");
    muiSetCallback(b3, bcallback);
    pd = muiNewPulldown();
    muiAddPulldownEntry(pd, "File", M1, 0);
    muiAddPulldownEntry(pd, "Edit", M2, 0);
    muiAddPulldownEntry(pd, "Help", M3, 1);
    muiSetTLStrings(tl, strs);
    muiSetCallback(vs, controltltop);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    if (argc > 1) mui_singlebuffered = 1;
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(300, 500);
    if (mui_singlebuffered)
	glutInitDisplayMode( GLUT_RGB | GLUT_SINGLE );
    else
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
    glutCreateWindow("test");
    maketestui();
    muiInit();
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
