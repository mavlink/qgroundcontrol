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

/******************************************************************************
 *                                                                            *
 *                Reverse-Polish-Notation (RPN) calculator                    *
 *                                                                            *
 *                Written by Tom Davis 1991                                   *
 *		  Converted to OpenGL/GLUT:  Tom Davis 1997		      *
 *                                                                            *
 *                                                                            *
 * Calc is a Reverse-Polish-Notation (RPN) calculator.  You must enter the    *
 * operands first, then the operation.  For example, to add 3 and 4, press    *
 * [3] [Enter] [4] [+].  If the operation is unary, like sine, it operates    *
 * on the bottom element of the display. To take the sine of .54, do:         *
 *                                                                            *
 * [.] [5] [4] [Sin].                                                         *
 *                                                                            *
 * The last 6 entries of the stack are visible, and you can scroll to see the *
 * rest.  All operations are performed on the element(s) at the bottom of the *
 * stack.  The bottom element is called 'x' and the next element up is called *
 * 'y'.                                                                       *
 *                                                                            *
 * The [+/-] key changes the sign of x.  To find the cosine of -.22, do:      *
 *                                                                            *
 * [.] [2] [2] [+/-] [Cos].                                                   *
 *                                                                            *
 * The [Inv] key changes the operation of some of the other keys so they      *
 * perform the inverse operation.  It is only active for one keystroke.       *
 * Press [Inv] again to cancel the operation.                                 *
 *                                                                            *
 * [Sto] and [Rcl] stores and recalls a single value.                         *
 *                                                                            *
 * [Dup2] duplicates the bottom 2 items on the stack.                         *
 *                                                                            *
 * [Roll] rolls all the stack elements down one, and puts the bottom element  *
 * on the top.                                                                *
 *                                                                            *
 * [Exch] swaps the bottom two elements.                                      *
 *                                                                            *
 * [Int] gives the integer part.                                              *
 *                                                                            *
 * [Inv] [Frac] gives the fractional part.                                    *
 *                                                                            *
 * [Clr] clears the bottom element to zero.  Use this when you get some kind  *
 * of error.                                                                  *
 *                                                                            *
 * [B10] and [B16] put you in base 10 or base 16 mode.  Numbers with          *
 * fractional parts are always displayed in base 10.  In base 16 mode, the    *
 * keys [a] through [f] are used for numeric entry.  They do nothing,         *
 * otherwise.                                                                 *
 *                                                                            *
 * [And], [Or] and [Not] are logical operations on 32 bit integers.  If       *
 * there's a fractional part, they don't do anything.                         *
 *                                                                            *
 * To remember a sequence of keystrokes, press [Prog], then the sequence of   *
 * keystrokes, and then [Prog] again.  For example, if you want to            *
 * calculate x^2 + y^2 repeatedly, where x and y are the two bottom entries   *
 * of the stack, do this:                                                     *
 *                                                                            *
 * [Prog] [Inv] [x^2] [Exch] [Inv] [x^2] [+] [Prog].                          *
 *                                                                            *
 * Then, to calculate 5^2+7^2, do this:                                       *
 *                                                                            *
 * [5] [Enter] [7] [Run].                                                     *
 *                                                                            *
 * The following keys from the computer keyboard are understood by calc:      *
 *                                                                            *
 * [0], [1], [2], [3], [4], [5], [6], [7], [8], [9], [.], [Enter],            *
 * [+], [-], [*], [/], [a], [b], [c], [d], [e], [f].                          *
 *                                                                            *
 ******************************************************************************/

#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mui/mui.h>
#include "calc.h"

/* Many systems lack the trunc routine. */
#ifndef __sgi
#define trunc(x) ((float)((int)x))
/* Alternative trunc macro: ((x > 0.0) ? floor(x) : -floor(-x)) */
#endif

#define Bwidth	    	44
#define Bheight     	25
#define Bhspace     	10
#define Bvspace	    	10
#define Displaylines	6

#define THUMBHEIGHT 20
#define ARROWSPACE 40
#define SLIDERWIDTH 20

#define Stackdepth  200

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void interpclick(int);
void showhelp(void);

char *CS[Stackdepth+Displaylines];

double calcdata[Stackdepth+Displaylines];

int CSP = Displaylines-1;
int invmode = 0;
int base = 10;
float memory = 0.0;
int degreemode = 1;

int savingprog = 0;
int program[5000];
int proglen = 0;

#define WIDTH 6
#define HEIGHT 8

muiObject *tl;

struct calcbutton {
    char *label, *invlabel;
    muiObject *b;
    int type;
} keypad[HEIGHT][WIDTH];


void butcallback(muiObject *obj, enum muiReturnValue r)
{
    int but = muiGetID(obj);
    interpclick(but);
    r = r;
}

void loadcb(int row, int col, char *lab, char *invlab, int type)
{
    struct calcbutton *cb = &keypad[row][col];
    cb->label = lab;
    cb->invlabel = invlab;
    cb->type = type;
    cb->b = muiNewButton(Bhspace+col*(Bhspace+Bwidth), (col+1)*(Bhspace+Bwidth),
			    Bvspace+row*(Bvspace+Bheight), (row+1)*(Bvspace+Bheight));
    muiLoadButton(cb->b, lab);
    muiSetCallback(cb->b, butcallback);
    muiSetID(cb->b, type);
}

void initkeypad(void)
{
    int xmin, xmax, ymin, ymax;

    loadcb(0, 0, "Exch", "Exch", Exchkey);
    loadcb(0, 1, "0", "0", Zerokey);
    loadcb(0, 2, ".", ".", Dotkey);
    loadcb(0, 3, "+/-", "+/-", Flipsignkey);
    loadcb(0, 4, "/", "/", Dividekey);
    loadcb(0, 5, "Deg", "Deg", Radkey);
    loadcb(1, 0, "Roll", "Roll", Rollkey);
    loadcb(1, 1, "1", "1", Onekey);
    loadcb(1, 2, "2", "2", Twokey);
    loadcb(1, 3, "3", "3", Threekey);
    loadcb(1, 4, "*", "*", Timeskey);
    loadcb(1, 5, "Clr", "Clr", Clearkey);
    loadcb(2, 0, "Dup2", "Dup2", Dup2key);
    loadcb(2, 1, "4", "4", Fourkey);
    loadcb(2, 2, "5", "5", Fivekey);
    loadcb(2, 3, "6", "6", Sixkey);
    loadcb(2, 4, "-", "-", Minuskey);
    loadcb(2, 5, "And", "And", Andkey);
    loadcb(3, 0, "Enter", "Enter", Enterkey);
    loadcb(3, 1, "7", "7", Sevenkey);
    loadcb(3, 2, "8", "8", Eightkey);
    loadcb(3, 3, "9", "9", Ninekey);
    loadcb(3, 4, "+", "+", Pluskey);
    loadcb(3, 5, "Or", "Or", Orkey);
    loadcb(4, 0, "Sin", "Asin", Sinkey);
    loadcb(4, 1, "Cos", "Acos", Coskey);
    loadcb(4, 2, "Tan", "Atan", Tankey);
    loadcb(4, 3, "Int", "Frac", Intkey);
    loadcb(4, 4, "Help", "Help", Helpkey);
    loadcb(4, 5, "Not", "Not", Notkey);
    loadcb(5, 0, "e^x", "Ln", Expkey);
    loadcb(5, 1, "10^x", "Log", Tentoxkey);
    loadcb(5, 2, "Sqrt", "x^2", Sqrtkey);
    loadcb(5, 3, "y^x", "y^(1/x)", Xtoykey);
    loadcb(5, 4, "Run", "Run", Runkey);
    loadcb(5, 5, "B16", "B16", Base16key);
    loadcb(6, 0, "Inv", "Inv", Invkey);
    loadcb(6, 1, "Rcl", "Rcl", Recallkey);
    loadcb(6, 2, "Sto", "Sto", Storekey);
    loadcb(6, 3, "1/x", "1/x", Oneoverkey);
    loadcb(6, 4, "Prog", "Prog", Progkey);
    loadcb(6, 5, "B10", "B10", Base10key);
    loadcb(7, 0, "a", "a", Akey);
    loadcb(7, 1, "b", "b", Bkey);
    loadcb(7, 2, "c", "c", Ckey);
    loadcb(7, 3, "d", "d", Dkey);
    loadcb(7, 4, "e", "e", Ekey);
    loadcb(7, 5, "f", "f", Fkey);
    tl = muiNewTextList(Bhspace, HEIGHT*Bheight + (HEIGHT+1)*Bvspace,
				    WIDTH*(Bhspace+Bwidth), Displaylines);
    muiGetObjectSize(tl, &xmin, &ymin, &xmax, &ymax);
    muiSetTLStrings(tl, CS);
}

/*
void drawdisplay(void)
{
    settltop(cdisplay, CSP - Displaylines + 1);
    cdisplay->count = CSP+1;
    adjustslider(cdisplay, cdisplay->vs);
    setvsarrowdelta(cdisplay->vs, 1);
    drawtl(cdisplay); swapbuffers();
}

void drawkeypad(void)
{
    int i, j;

    backgroundclear();
    for (i = 0; i < HEIGHT; i++)
    	for (j = 0; j < WIDTH; j++)
	    drawbut(keypad[i][j].b);
    drawdisplay();
}
*/

void loadbuttons(int inv)
{
    int i, j;

    for (i = 0; i < HEIGHT; i++)
    	for (j = 0; j < WIDTH; j++) {
	    if (inv)
	    	muiLoadButton(keypad[i][j].b, keypad[i][j].invlabel);
	    else
	    	muiLoadButton(keypad[i][j].b, keypad[i][j].label);
	}
    glutPostRedisplay();
}

void initcalc(void)
{
    int i;
    
    glutInitWindowSize(WIDTH*Bwidth+(WIDTH+1)*Bhspace,
    	HEIGHT*Bheight+(HEIGHT+2)*Bvspace+Displaylines*20+7);
    glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow("RPN Calc");
    muiInit();
    muiNewUIList(1);
    for (i = 0; i < Stackdepth+Displaylines; i++) {
    	CS[i] = (char *)malloc(50);
	*CS[i] = ' ';
	*(CS[i]+1) = 0;
    }
    initkeypad();
}

void formatentry(int s)
{
    if (calcdata[s] - trunc(calcdata[s]) == 0 &&
    	    (calcdata[s] < 1.0e9 && -1.0e9 < calcdata[s])) {
	if (base == 10) {
	    sprintf(CS[s], "%15d", (int)calcdata[s]);
	} else {
	    sprintf(CS[s], "0x%13x", (int)calcdata[s]);
	}
    } else {
        if (calcdata[s] < 1.0e9 && -1.0e9 < calcdata[s] &&
	    (calcdata[s] > 1.0e-5 || calcdata[s] < -1.0e-5))
    	    sprintf(CS[s], "%15.12f", calcdata[s]);
	else
	    sprintf(CS[s], "%15.12e", calcdata[s]);
    }
}

/* ARGSUSED1 */
void kbd(unsigned char c, int x, int y)
{
    switch (c) {
	case 'A': case 'a': interpclick(Akey); break;
	case 'B': case 'b': interpclick(Bkey); break;
	case 'C': case 'c': interpclick(Ckey); break;
	case 'D': case 'd': interpclick(Dkey); break;
	case 'E': case 'e': interpclick(Ekey); break;
	case 'F': case 'f': interpclick(Fkey); break;
	case '0': interpclick(Zerokey); break;
	case '1': interpclick(Onekey); break;
	case '2': interpclick(Twokey); break;
	case '3': interpclick(Threekey); break;
	case '4': interpclick(Fourkey); break;
	case '5': interpclick(Fivekey); break;
	case '6': interpclick(Sixkey); break;
	case '7': interpclick(Sevenkey); break;
	case '8': interpclick(Eightkey); break;
	case '9': interpclick(Ninekey); break;
	case '.': interpclick(Dotkey); break;
	case '+': interpclick(Pluskey); break;
	case '-': interpclick(Minuskey); break;
	case '*': interpclick(Timeskey); break;
	case '/': interpclick(Dividekey); break;
	case '\n': case '\r': interpclick(Enterkey); break;
    }
    
}

void main(int argc, char **argv)
{
    glutInit(&argc, argv);
    initcalc();
    formatentry(Displaylines-1);
    glutKeyboardFunc(kbd);  /* overrides mui cmd */
    glutMainLoop();
}

int inputptr = 0;
int enablepush = 0;

void push(void)
{
    CSP++;
    inputptr = 0;
    *CS[CSP] = ' ';
    *(CS[CSP]+1) = 0;
    muiSetTLTopInt(tl, CSP - Displaylines + 1);
}

void doenter(void)
{
    int isdot = 0;
    
    char *s = CS[CSP];
    while (*s) {if (*s++ == '.') isdot=1; }
    if (enablepush) {
	strcpy(CS[CSP+1], CS[CSP]);
	calcdata[CSP+1] = calcdata[CSP];
	CSP++;
    } else {
        if (isdot || base == 10)
	    calcdata[CSP] = atof(CS[CSP]);
	else
	    calcdata[CSP] = (double) strtol(CS[CSP], 0, 16);
	enablepush = 1;
	inputptr = 0;
    }
    formatentry(CSP);
    muiSetTLTopInt(tl, CSP - Displaylines + 1);
    glutPostRedisplay();
}

void binop(char c)
{
    int i, j;

    if (enablepush == 0) doenter();
    if (CSP < Displaylines) return;
    switch (c) {
	case '+':
	    calcdata[CSP-1] = calcdata[CSP-1] + calcdata[CSP];
	    break;
	case '-':
	    calcdata[CSP-1] = calcdata[CSP-1] - calcdata[CSP];
	    break;
	case '*':
	    calcdata[CSP-1] = calcdata[CSP-1] * calcdata[CSP];
	    break;
	case '/':
	    calcdata[CSP-1] = calcdata[CSP-1] / calcdata[CSP];
	    break;
	case '^':
	    if (invmode == 0)
	    	calcdata[CSP-1] = pow(calcdata[CSP-1], calcdata[CSP]);
	    else
	    	calcdata[CSP-1] = pow(calcdata[CSP-1], 1.0/calcdata[CSP]);
	    break;
	case '&':
	    if (calcdata[CSP-1] - trunc(calcdata[CSP-1]) != 0) return;
	    if (calcdata[CSP] - trunc(calcdata[CSP]) != 0) return;
	    i = calcdata[CSP-1]; j = calcdata[CSP];
	    calcdata[CSP-1] = (int)(i&j);
	    break;
	case '|':
	    if (calcdata[CSP-1] - trunc(calcdata[CSP-1]) != 0) return;
	    if (calcdata[CSP] - trunc(calcdata[CSP]) != 0) return;
	    i = calcdata[CSP-1]; j = calcdata[CSP];
	    calcdata[CSP-1] = (int)(i|j);
	    break;
    }
    CS[CSP][0] = 0;
    CSP--;
    formatentry(CSP);
    muiSetTLTopInt(tl, CSP - Displaylines + 1);
    glutPostRedisplay();
    enablepush = 1;
}

void unop(int c)
{
    int i;

    if (enablepush == 0) doenter();
    switch (c) {
    	case Storekey:
	    memory = calcdata[CSP];
	    break;
	case Recallkey:
	    doenter();
	    calcdata[CSP] = memory;
	    break;
	case Flipsignkey:
	    calcdata[CSP] = -calcdata[CSP];
	    break;
	case Oneoverkey:
	    calcdata[CSP] = 1.0/calcdata[CSP];
	    break;
	case Clearkey:
	    calcdata[CSP] = 0.0;
	    enablepush = 0;
	    formatentry(CSP);
	    glutPostRedisplay();
	    return;
	case Sinkey:
	    if (invmode) {
	    	calcdata[CSP] = asin(calcdata[CSP]);
		if (degreemode) calcdata[CSP] *= 180.0/M_PI;
	    } else {
		if (degreemode) calcdata[CSP] *= M_PI/180.0;
	    	calcdata[CSP] = sin(calcdata[CSP]);
	    }
	    break;
	case Coskey:
	    if (invmode) {
	    	calcdata[CSP] = acos(calcdata[CSP]);
		if (degreemode) calcdata[CSP] *= 180.0/M_PI;
	    } else {
		if (degreemode) calcdata[CSP] *= M_PI/180.0;
	    	calcdata[CSP] = cos(calcdata[CSP]);
	    }
	    break;
	case Tankey:
	    if (invmode) {
	    	calcdata[CSP] = atan(calcdata[CSP]);
		if (degreemode) calcdata[CSP] *= 180.0/M_PI;
	    } else {
		if (degreemode) calcdata[CSP] *= M_PI/180.0;
	    	calcdata[CSP] = tan(calcdata[CSP]);
	    }
	    break;
	case Tentoxkey:
	    if (invmode)
	    	calcdata[CSP] = log10(calcdata[CSP]);
	    else
	    	calcdata[CSP] = pow(10, calcdata[CSP]);
	    break;
	case Expkey:
	    if (invmode)
	    	calcdata[CSP] = log(calcdata[CSP]);
	    else
	    	calcdata[CSP] = exp(calcdata[CSP]);
	    break;
	case Intkey:
	    if (invmode)
	    	calcdata[CSP] = calcdata[CSP] - trunc(calcdata[CSP]);
	    else
	    	calcdata[CSP] = trunc(calcdata[CSP]);
	    break;
	case Sqrtkey:
	    if (invmode==0)
	        calcdata[CSP] = sqrt(calcdata[CSP]);
	    else
	        calcdata[CSP] = calcdata[CSP]*calcdata[CSP];
	    break;
	case Notkey:
	    if (calcdata[CSP] - trunc(calcdata[CSP]) != 0) return;
	    i = calcdata[CSP];
	    calcdata[CSP] = (int)(~i);
	    break;
    }
    formatentry(CSP);
    muiSetTLTopInt(tl, CSP - Displaylines + 1);
    glutPostRedisplay();
    enablepush = 1;
}

void interpclick(int x)
{
    int i, rcount;
    float f;
    char *c;
    
    if (x == 0) return;
    if (savingprog && x != Progkey && x != Runkey)
        program[proglen++] = x;
    if ((Zerokey <= x && x <= Ninekey) || x == Dotkey ||
    	((base == 16) && (Akey <= x && x <= Fkey))) {
        if (enablepush) push();
	enablepush = 0;
	if (x == Dotkey)
	    CS[CSP][inputptr++] = '.';
	else if (Akey <= x && x <= Fkey)
	    CS[CSP][inputptr++] = 'a' + x - Akey;
	else
	    CS[CSP][inputptr++] = '0'+x-Zerokey;
	CS[CSP][inputptr] = 0;
	muiSetTLTopInt(tl, CSP - Displaylines + 1);
	glutPostRedisplay();
    } else switch (x) {
	case Pluskey:
	    binop('+'); break;
	case Minuskey:
	    binop('-'); break;
	case Timeskey:
	    binop('*'); break;
	case Dividekey:
	    binop('/'); break;
	case Xtoykey:
	    binop('^'); break;
	case Helpkey:
	    showhelp(); break;
	case Clearkey:
	    unop(Clearkey); break;
	case Flipsignkey:
	    unop(Flipsignkey); break;
	case Enterkey:
	    doenter();
	    break;
	case Exchkey:
	    if (CSP < Displaylines) break;
	    if (enablepush == 0) doenter();
	    f = calcdata[CSP];
	    calcdata[CSP] = calcdata[CSP-1];
	    calcdata[CSP-1] = f;
	    c = CS[CSP];
	    CS[CSP] = CS[CSP-1];
	    CS[CSP-1] = c;
	    glutPostRedisplay();
	    break;
	case Rollkey:
	    if (CSP < Displaylines) break;
	    rcount = CSP - Displaylines+1;
	    if (enablepush == 0) doenter();
	    f = calcdata[CSP];
	    c = CS[CSP];
	    for (i = 0; i < rcount; i++) {
		calcdata[CSP-i] = calcdata[CSP-i-1];
		CS[CSP-i] = CS[CSP-i-1];
	    }
	    calcdata[CSP-rcount] = f;
	    CS[CSP-rcount] = c;
	    glutPostRedisplay();
	    break;
	case Dup2key:
	    if (CSP < Displaylines) break;
	    if (enablepush == 0) doenter();
	    strcpy(CS[CSP+2], CS[CSP]);
	    calcdata[CSP+2] = calcdata[CSP];
	    strcpy(CS[CSP+1], CS[CSP-1]);
	    calcdata[CSP+1] = calcdata[CSP-1];
	    CSP += 2;
	    muiSetTLTopInt(tl, CSP - Displaylines + 1);
	    glutPostRedisplay();
	    break;
	case Radkey:
	    if (degreemode) {
	        keypad[0][5].label = "Rad";
		keypad[0][5].invlabel = "Rad";
	    } else {
	        keypad[0][5].label = "Deg";
		keypad[0][5].invlabel = "Deg";
	    }
	    muiLoadButton(keypad[0][5].b, keypad[0][5].label);
	    degreemode = 1 - degreemode;
	    loadbuttons(invmode);
	    break;
	case Invkey:
	    if (invmode) {
		invmode = 0;
		loadbuttons(0);
	    } else {
		invmode = 2;
		loadbuttons(1);
	    }
	    break;
	case Progkey:
	    if (savingprog == 0) {
		proglen = 0;
	    }
	    savingprog = 1 - savingprog;
	    break;
	case Runkey:
	    if (savingprog) break;
	    for (i = 0; i < proglen; i++)
	    	interpclick(program[i]);
	    break;
	case Storekey:
	    unop(Storekey); break;
	case Recallkey:
	    unop(Recallkey); break;
	case Oneoverkey:
	    unop(Oneoverkey); break;
	case Sinkey:
	    unop(Sinkey); break;
	case Coskey:
	    unop(Coskey); break;
	case Tankey:
	    unop(Tankey); break;
	case Expkey:
	    unop(Expkey); break;
	case Tentoxkey:
	    unop(Tentoxkey); break;
	case Intkey:
	    unop(Intkey); break;
	case Andkey:
	    binop('&'); break;
	case Orkey:
	    binop('|'); break;
	case Notkey:
	    unop(Notkey); break;
	case Base10key:
	    if (enablepush == 0) doenter();
	    base = 10;
	    for (i = Displaylines-1; i <= CSP; i++)
	    	formatentry(i);
	    glutPostRedisplay();
	    break;
	case Base16key:
	    if (enablepush == 0) doenter();
	    base = 16;
	    for (i = Displaylines-1; i <= CSP; i++)
	    	formatentry(i);
	    glutPostRedisplay();
	    break;
	case Sqrtkey:
	    unop(Sqrtkey); break;
    }
    if (invmode == 1) {
	loadbuttons(0);
    }
    if (invmode > 0) invmode--;
}

void showhelp(void)
{
printf("\n\n------------------------------------------\n\n");
printf("Calc is a Reverse-Polish-Notation (RPN) calculator.  You\n");
printf("must enter the operands first, then the operation.  For\n");
printf("example, to add 3 and 4, press [3] [Enter] [4] [+].  If\n");
printf("the operation is unary, like sine, it operates on the bottom\n");
printf("element of the display.  To take the sine of .54, do:\n");
printf("\n");
printf("[.] [5] [4] [Sin].\n");
printf("\n");
printf("The last 6 entries of the stack are visible, and you can\n");
printf("scroll to see the rest.  All operations are performed on\n");
printf("the element(s) at the bottom of the stack.  The bottom\n");
printf("element is called 'x' and the next element up is called 'y'.\n");
printf("\n");
printf("The [+/-] key changes the sign of x.  To find the cosine of\n");
printf("-.22, do:\n");
printf("\n");
printf("[.] [2] [2] [+/-] [Cos].\n");
printf("\n");
printf("The [Inv] key changes the operation of some of the other keys\n");
printf("so they perform the inverse operation.  It is only active for\n");
printf("one keystroke.  Press [Inv] again to cancel the operation.\n");
printf("\n");
printf("[Sto] and [Rcl] stores and recalls a single value.\n");
printf("\n");
printf("[Dup2] duplicates the bottom 2 items on the stack.\n");
printf("\n");
printf("[Roll] rolls all the stack elements down one, and puts the\n");
printf("bottom element on the top.\n");
printf("\n");
printf("[Exch] swaps the bottom two elements.\n");
printf("\n");
printf("[Int] gives the integer part.\n");
printf("\n");
printf("[Inv] [Frac] gives the fractional part.\n");
printf("\n");
printf("[Clr] clears the bottom element to zero.  Use this when you\n");
printf("get some kind of error\n");
printf("\n");
printf("[B10] and [B16] put you in base 10 or base 16 mode.  Numbers\n");
printf("with fractional parts are always displayed in base 10.  In\n");
printf("base 16 mode, the keys [a] through [f] are used for numeric\n");
printf("entry.  They do nothing, otherwise.\n");
printf("\n");
printf("[And], [Or] and [Not] are logical operations on 32 bit\n");
printf("integers.  If there's a fractional part, they don't do\n");
printf("anything.\n");
printf("\n");
printf("To remember a sequence of keystrokes, press [Prog], then the\n");
printf("sequence of keystrokes, and then [Prog] again.  For example,\n");
printf("if you want to calculate x^2 + y^2 repeatedly, where x and y\n");
printf("are the two bottom entries of the stack, do this:\n");
printf("\n");
printf("[Prog] [Inv] [x^2] [Exch] [Inv] [x^2] [+] [Prog].\n");
printf("\n");
printf("Then, to calculate 5^2+7^2, do this:\n");
printf("\n");
printf("[5] [Enter] [7] [Run].\n");
printf("\n");
printf("The following keys from the computer keyboard are understood\n");
printf("by calc:\n");
printf("\n");
printf("[0], [1], [2], [3], [4], [5], [6], [7], [8], [9], [.],\n");
printf("[Enter], [+], [-], [*], [/], [a], [b], [c], [d], [e], [f].\n");
printf("\n");
printf("The [Deg]/[Rad] key shows the current angle mode.  Press\n");
printf("it to get the other angle mode.\n");
printf("\n------------------------------------------\n\n");
}
