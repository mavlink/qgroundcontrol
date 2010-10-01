
C  Copyright (c) Mark J. Kilgard, 1994.

C  This program is freely distributable without licensing fees
C  and is provided without guarantee or warrantee expressed or
C  implied.  This program is -not- in the public domain.

C  GLUT Fortran program to draw red light sphere.

	subroutine display
#include "GL/fgl.h"
	call fglclear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT)
	call fglcalllist(1)
	call glutswapbuffers
	end

	subroutine gfxinit
#include "GL/fgl.h"
	real diffuse(4),pos(4)
	data diffuse /1.0, 0.0, 0.0, 1.0/
	data pos /1.0, 1.0, 1.0, 0.0/
	call fglnewlist(1, GL_COMPILE)
	call glutsolidsphere(dble(1.0), 20, 20)
	call fglendlist
	call fgllightfv(GL_LIGHT0, GL_DIFFUSE, diffuse)
	call fgllightfv(GL_LIGHT0, GL_POSITION, pos)
	call fglenable(GL_LIGHTING)
	call fglenable(GL_LIGHT0)
	call fglenable(GL_DEPTH_TEST)
	call fglmatrixmode(GL_PROJECTION)
	call fgluperspective(dble(40.0), dble(1.0),
     2                       dble(1.0), dble(10.0))
	call fglmatrixmode(GL_MODELVIEW)
	call fglulookat(dble(0.0), dble(0.0), dble(5.0),
     2                  dble(0.0), dble(0.0), dble(0.0),
     3                  dble(0.0), dble(1.0), dble(1.0))
	call fgltranslatef(0.0, 0.0, -1.0)
	end

	program main
#include "GL/fglut.h"
	external display
	external reshape
	external submenu
	external mainmenu
	integer win
	call glutinit
	call glutinitdisplaymode(GLUT_DOUBLE+GLUT_RGB+GLUT_DEPTH)
	win =  glutcreatewindow('Fortran GLUT sphere')
	call gfxinit
	call glutdisplayfunc(display)
	call glutmainloop
	end

