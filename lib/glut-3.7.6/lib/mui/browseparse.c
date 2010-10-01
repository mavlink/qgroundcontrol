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
#include <string.h>
#include <stdlib.h>

char currentdirectoryname[200];
char currentfilename[200];
char browseprompt[200];

int xcenter = 300, ycenter = 300;

void nukecr(char *s)
{
    while (*s && *s != '\n') s++;
    *s = 0;
}

void parsebrowsefile(FILE *f)
{
    char buffer[300];

    while (0 != fgets(buffer, 299, f))
	switch(buffer[0]) {
	    case 'D':
		strcpy(currentdirectoryname, &buffer[2]);
		nukecr(currentdirectoryname);
		break;
	    case 'F':
		strcpy(currentfilename, &buffer[2]);
		nukecr(currentfilename);
		break;
	    case 'P':
		strcpy(browseprompt, &buffer[2]);
		nukecr(browseprompt);
		break;
	    case 'X':
		xcenter = atoi(&buffer[2]);
		break;
	    case 'Y':
		ycenter = atoi(&buffer[2]);
		break;
	}
}

void setcurrentfilename(char *s)
{
    int len = strlen(s);
    char *sptr;
    
    sptr = &s[len-1];
    while (sptr != s) {
	if (*sptr == '/') {
	    strcpy(currentfilename, sptr+1);
	    *sptr = 0;
	    strcpy(currentdirectoryname, s);
	    return;
	}
	sptr--;
    }
    strcpy(currentfilename, s);
}

