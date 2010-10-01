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
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <mui/browser.h>

#define MAXFILES 400
char	*filelist[MAXFILES];
char	err[80];
char	*dot = ".";
char	*dotdot = "..";
char	directory[300], originaldir[300];
struct stat	d, dd;
struct dirent	*dir;

DIR	*file;
int	off;

extern int mui_singlebuffered;
int selectedfile = -1;
int cd(char *s);
void pwd(void);
void ls(void);

extern void	settlstrings(muiObject *obj, char **s);
void	settltop(muiObject *obj, int top);

muiObject *tl, *vs, *l4;

void writeoutputfile(char *dir, char *file)
{
    FILE *f;
    f = fopen(BROWSEFILE, "w");
    fprintf(f, "D:%s\n", dir);
    if (file)
	fprintf(f, "F:%s\n", file);
    fclose(f);
}

void	controltltop(muiObject *obj, enum muiReturnValue value)
{
    float sliderval;

    if ((value != MUI_SLIDER_RETURN) && (value != MUI_SLIDER_THUMB)) return;
    sliderval = muiGetVSVal(obj);
    muiSetTLTop(tl, sliderval);
}

void	handlefileselection(muiObject *obj, enum muiReturnValue value)
{
    char *fname;
    int len;

    if (value == MUI_TEXTLIST_RETURN_CONFIRM) {
	selectedfile = muiGetTLSelectedItem(obj);
	fname = filelist[selectedfile];
	len = strlen(fname);
	if (fname[len-1] == '/') {
	    fname[len-1] = 0;
	    cd(fname);
	    return;
	} else {
	    writeoutputfile(directory, fname);
	    exit(0);
	}
    }
    if (value != MUI_TEXTLIST_RETURN) return;
    selectedfile = muiGetTLSelectedItem(obj);
    muiSetVSValue(vs, 1.0);
}

void handleaccept(muiObject *obj, enum muiReturnValue value)
{
    char *fname;
    int len;

    if (value != MUI_BUTTON_PRESS) return;
    if (selectedfile == -1) return;
    fname = filelist[selectedfile];
    len = strlen(fname);
    if (fname[len-1] == '/') {
	fname[len-1] = 0;
	cd(fname);
	return;
    } else {
	writeoutputfile(directory, fname);
	exit(0);
    }
    obj = 0;	/* for lint's sake */
}

void handleoriginal(muiObject *obj, enum muiReturnValue value)
{
    if (value != MUI_BUTTON_PRESS) return;
    cd(originaldir);
    obj = 0;	/* for lint's sake */
}

void handleupdir(muiObject *obj, enum muiReturnValue value)
{
    if (value != MUI_BUTTON_PRESS) return;
    cd("..");
    obj = 0;	/* for lint's sake */
}

void handlecancel(muiObject *obj, enum muiReturnValue value)
{
    if (value != MUI_BUTTON_PRESS) return;
    writeoutputfile(directory, 0);
    exit(0);
    obj = 0;	/* for lint's sake */
}

void handletextbox(muiObject *obj, enum muiReturnValue value)
{
    char *s, *slash;

    if (value != MUI_TEXTBOX_RETURN) return;
    s = muiGetTBString(obj);
    if (0 == chdir(s)) {
	pwd();
	ls();
	settlstrings(tl, filelist);
	selectedfile = 0;
	muiChangeLabel(l4, directory);
	muiClearTBString(obj);
	return;
    }
    /* hack up the path, if any */
    slash = strrchr(s, '/');
    if (slash == 0) {
	slash = s-1;	/* to make filename == slash+1 */
    } else {
	if (*s == '/') { /* absolute path */
	    strncpy(directory, s, slash-s);
	    directory[slash-s] = 0;
	} else {
	    strcat(directory, "/");
	    strncat(directory, s, slash-s);
	}
    }
     /* now filename == slash+1 */
    writeoutputfile(directory, slash+1);
    exit(0);
}

#define THUMBHEIGHT 20
#define ARROWSPACE 40

void maketestui(void)
{
    muiObject *l1, *l2, *l3, *b1, *b2, *b3, *b4, *t;
    int xmin, ymin, xmax, ymax;

    muiNewUIList(1);
    l1 = muiNewBoldLabel(10, 475, "Directory:");
    muiAddToUIList(1, l1);
    l4 = muiNewLabel(80, 475, "./");
    muiAddToUIList(1, l4);
    l2 = muiNewBoldLabel(10, 430, "Set directory:");
    muiAddToUIList(1, l2);
    b1 = muiNewButton(10, 100, 390, 415);
    muiLoadButton(b1, "Up");
    muiAddToUIList(1, b1);
    muiSetCallback(b1, handleupdir);
    b2 = muiNewButton(10, 100, 355, 380);
    muiLoadButton(b2, "Original");
    muiAddToUIList(1, b2);
    muiSetCallback(b2, handleoriginal);
    tl = muiNewTextList(120, 80, 370, 22);
    muiAddToUIList(1, tl);
    muiGetObjectSize(tl, &xmin, &ymin, &xmax, &ymax);
    vs = muiNewVSlider(xmax, ymin+2, ymax, 0, THUMBHEIGHT);
    muiSetVSValue(vs, 1.0);
    muiSetVSArrowDelta(vs, 10);
    muiAddToUIList(1, vs);
    t = muiNewTextbox(120, 390, 40);
    muiSetActive(t, 1);
    muiAddToUIList(1, t);
    muiSetCallback(t, handletextbox);
    l3 = muiNewBoldLabel(40, 50, "Open File:");
    muiAddToUIList(1, l3);
    b3 = muiNewButton(130, 230, 9, 34);
    muiLoadButton(b3, "Accept");
    muiSetCallback(b3, handleaccept);
    muiAddToUIList(1, b3);
    b4 = muiNewButton(250, 350, 9, 34);
    muiLoadButton(b4, "Cancel");
    muiSetCallback(b4, handlecancel);
    muiAddToUIList(1, b4);
    muiSetCallback(vs, controltltop);
    muiSetCallback(tl, handlefileselection);
    
    cd(directory);
    strcpy(originaldir, directory);
}

void main(int argc, char **argv)
{
    FILE *f;

    f = fopen(BROWSEFILE, "r");
    parsebrowsefile(f);
    fclose(f);
    strcpy(directory, currentdirectoryname);
    maketestui();
    glutInit(&argc, argv);
    if (argc > 1) mui_singlebuffered = 1;
    glutInitWindowPosition(xcenter-200, ycenter-250);
    glutInitWindowSize(400, 500);
    if (mui_singlebuffered)
	glutInitDisplayMode( GLUT_RGB | GLUT_SINGLE );
    else
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
    glutCreateWindow("browser");
    muiInit();
    glutMainLoop();
}

void errormsg(char *s)
{
    fprintf(stderr, "%s\n", s);
}

void prname(void)
{
	directory[0] = '/';
	if (off == 0)
		off = 1;
	directory[off] = 0;
}

int dirlevels(char *s)
{
    int levels;

    for (levels = 0; *s; s++)
	if (*s == '/')
	    levels++;
    return(levels);
}

int cat(void)
{
	register i, j;
	char *name = directory + 1;	/* I love C */

	i = -1;
	while (dir->d_name[++i] != 0) 
	if ((off+i+2) > MAXNAMLEN - 1) {
		prname();
		return 1;
	}
	for(j=off+1; j>=0; --j)
		name[j+i+1] = name[j];
	off=i+off+1;
	name[i] = '/';
	for(--i; i>=0; --i)
		name[i] = dir->d_name[i];
	return 0;
}

/* get the current working directory (the following 3 routines are from pwd.c) */
void pwd(void)
{
	for(off = 0;;) {
		if(stat(dot, &d) < 0) {
			fprintf(stderr, "pwd: cannot stat .!\n");
			exit(2);
		}
		if ((file = opendir(dotdot)) == NULL) {
			fprintf(stderr,"pwd: cannot open ..\n");
			exit(2);
		}
		if(fstat(file->dd_fd, &dd) < 0) {
			fprintf(stderr, "pwd: cannot stat ..!\n");
			exit(2);
		}
		if(chdir(dotdot) < 0) {
			fprintf(stderr, "pwd: cannot chdir to ..\n");
			exit(2);
		}
		if(d.st_dev == dd.st_dev) {
			if(d.st_ino == dd.st_ino) {
				prname();
				chdir(directory);
				return;
			}
			do
				if ((dir = readdir(file)) == NULL) {
					fprintf(stderr, "pwd: read error in ..\n");
					exit(2);
				}
			while (dir->d_ino != d.st_ino);
		}
		else do {
				if((dir = readdir(file)) == NULL) {
					fprintf(stderr, "pwd: read error in ..\n");
					exit(2);
				}
				stat(dir->d_name, &dd);
		} while(dd.st_ino != d.st_ino || dd.st_dev != d.st_dev);
		(void)closedir(file);
		if (cat()) {
			chdir(directory);
			return;
		}
	}
}

void freels(void)
{
    char **p;

    p = filelist;
    while (*p != 0) {
	free(*p);
	*p = 0;
	p++;
    }
}

int mystrcmp(char **s1, char **s2)
{
    return strcmp(*s1,*s2);
}

void ls(void)
{
    DIR			*dirp;
    int			i = 0;
    int			len;
    struct dirent	*dir;
    struct stat		statbuf;
    

    if ((dirp = opendir(directory)) == NULL) {
	errormsg("bad directory\n");
	return;
    }
    freels();
    chdir(directory);
    while ((dir = readdir(dirp)) != NULL) {
	if (dir->d_name[0] == '.')
	    continue;
	/*f = open(dir->d_name, O_RDONLY);
	if (!f) 
	    continue;
	if (!okfiletype(getfiletype(f)))
	    continue;
	close(f);*/
	stat(dir->d_name,&statbuf);
	len = strlen(dir->d_name) + 1 + (statbuf.st_mode & S_IFDIR? 1 : 0);
	filelist[i] = (char *)malloc(len);
	strcpy(filelist[i], dir->d_name);
	if (statbuf.st_mode & S_IFDIR) {
	    filelist[i][len-2] = '/'; filelist[i][len-1] = 0;
	}
	i++;
    }
    filelist[i] = 0;
    qsort(&filelist[0], i, sizeof (char *), (int (*)(const void *, const void *))mystrcmp);
    closedir(dirp);
}

int cd(char *s)
{
    if(chdir(s) < 0) {
	fprintf(stderr,"cannot open %s\n",s);
	return -1;
    }
    pwd();
    ls();
    settlstrings(tl, filelist);
    muiChangeLabel(l4, directory);
    selectedfile = 0;
    return 0;
}
