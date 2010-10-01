
C  Copyright (c) Mark J. Kilgard, 1994.

C  This program is freely distributable without licensing fees
C  and is provided without guarantee or warrantee expressed or
C  implied.  This program is -not- in the public domain.

C  GLUT Fortran program to render simple red scene.

	subroutine display
#include "GL/fgl.h"
	call fglclear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT)
	call fglpushmatrix
	call fglscalef(1.3, 1.3, 1.3)
	call fglrotatef(20.0, 1.0, 0.0, 0.0)

	call fglpushmatrix
	call fgltranslatef(-0.75, 0.5, 0.0)
	call fglrotatef(90.0, 1.0, 0.0, 0.0)
	call glutsolidtorus(dble(0.275), dble(0.85), 10, 15)
	call fglpopmatrix

	call fglpushmatrix
	call fgltranslatef(-0.75, -0.5, 0.0)
	call fglrotatef(270.0, 1.0, 0.0, 0.0)
	call glutsolidtetrahedron
	call fglpopmatrix

	call fglpushmatrix
	call fgltranslatef(0.75, 0.0, -1.0)
	call glutsolidicosahedron
	call fglpopmatrix

	call fglpopmatrix
	call fglflush
	end

	subroutine reshape(w,h)
#include "GL/fgl.h"
	integer w,h
	real wr,hr
	real*8 d
	call fglviewport(0, 0, w, h)
	call fglmatrixmode(GL_PROJECTION)
	call fglloadidentity
	wr = w
	hr = h
	d = 1.0
	if ( w .le. h ) then
	   call fglortho(dble(-2.5), dble(2.5),
     2       dble(-2.5 * hr/wr), dble(2.5 * hr/wr),
     3       dble(-10.0), dble(10.0))
	else
	   call fglortho(dble(-2.5 * hr/wr), dble(2.5 * hr/wr),
     2       dble(-2.5), dble(2.5), dble(-10.0), dble(10.0))
	end if
	call fglmatrixmode(GL_MODELVIEW)
	end
	
	subroutine submenu(value)
#include "GL/fgl.h"
	integer value
	if ( value .eq. 1 ) then
	  call fglenable(GL_DEPTH_TEST)
	  call fglenable(GL_LIGHTING)
	  call fgldisable(GL_BLEND)
	  call fglpolygonmode(GL_FRONT_AND_BACK, GL_FILL)
        else
	  call fgldisable(GL_DEPTH_TEST)
	  call fgldisable(GL_LIGHTING)
	  call fglcolor3f(1.0, 1.0, 1.0)
	  call fglpolygonmode(GL_FRONT_AND_BACK, GL_LINE)
	  call fglenable(GL_LINE_SMOOTH)
	  call fglenable(GL_BLEND)
	  call fglblendfunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
	end if
	call glutpostredisplay
	end

	subroutine mainmenu(value)
	integer value
	call exit(1)
	end

	subroutine myinit
#include "GL/fgl.h"
	real lambient(4), ldiffuse(4), lspecular(4), lposition(4)
	data lambient /0.0, 0.0, 0.0, 1.0/
	data ldiffuse /1.0, 0.0, 0.0, 1.0/
	data lspecular /1.0, 1.0, 1.0, 1.0/
	data lposition /1.0, 1.0, 1.0, 0.0/

	call fgllightfv(GL_LIGHT0, GL_AMBIENT, lambient)
	call fgllightfv(GL_LIGHT0, GL_DIFFUSE, ldiffuse)
	call fgllightfv(GL_LIGHT0, GL_SPECULAR, lspecular)
	call fgllightfv(GL_LIGHT0, GL_POSITION, lposition)
	call fglenable(GL_LIGHT0)
	call fgldepthfunc(GL_LESS)
	call fglenable(GL_DEPTH_TEST)
	call fglenable(GL_LIGHTING)
	end

	program main
#include "GL/fglut.h"
	external display
	external reshape
	external submenu
	external mainmenu
	integer win, menu
	call glutinitwindowposition(500,500)
	call glutinitwindowsize(500,500)
	call glutinit
	win =  glutcreatewindow('Fortran GLUT program')
	call myinit
	call glutdisplayfunc(display)
	call glutreshapefunc(reshape)
	i = glutcreatemenu(submenu)
	call glutaddmenuentry('Filled', 1)
	call glutaddmenuentry('Outline', 2)
	menu = glutcreatemenu(mainmenu)
	call glutaddsubmenu('Polygon mode', i)
	call glutaddmenuentry('Quit', 666)
	call glutattachmenu(GLUT_RIGHT_BUTTON)
	call glutmainloop
	end

