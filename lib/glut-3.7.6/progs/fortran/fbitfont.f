
C  Copyright (c) Mark J. Kilgard, 1994.

C  This program is freely distributable without licensing fees
C  and is provided without guarantee or warrantee expressed or
C  implied.  This program is -not- in the public domain.

C  GLUT Fortran example demonstrating use of bitmap fonts.

	subroutine output(x,y,s)
	real x,y
	character s*(*)
	character c
#include "GL/fgl.h"
#include "GL/fglut.h"

C  XXX Stroke and font names must be explicitly declared as
C  external instead of relying on "GL/fglut.h" because
C  the IRIX Fortran compiler does not know to only
C  link in used external data symbols.
	external GLUT_BITMAP_TIMES_ROMAN_24

	call fglrasterpos2f(x,y)
	lenc = len(s)
	do 10, i=1,lenc
	  c = s(i:i)
	  call glutbitmapcharacter(GLUT_BITMAP_TIMES_ROMAN_24,
     2      ichar(c))
10	continue
	end

	subroutine display
#include "GL/fgl.h"
#include "GL/fglut.h"
	call fglclear(GL_COLOR_BUFFER_BIT)
	call output(0.0,24.0,
     2    'This is written in a GLUT bitmap font.')
	call output(100.0,100.0,'ABCDEFGabcdefg')
	call output(50.0,145.0,
     2    '(positioned in pixels with upper-left origin)')
	end

	subroutine reshape(w,h)
	integer w, h
#include "GL/fgl.h"
#include "GL/fglu.h"
	call fglviewport(0, 0, w, h)
	call fglmatrixmode(GL_PROJECTION)
	call fglloadidentity
	call fgluortho2d(dble(0.0), dble(w), dble(0.0), dble(h))
	call fglscalef(1.0, -1.0, 1.0)
	call fgltranslatef(real(0.0), real(-h), real(0.0))
	call fglmatrixmode(GL_MODELVIEW)
	end

	program main
#include "GL/fglut.h"
	external display
	external reshape
	integer win
	call glutinitdisplaymode(GLUT_RGB + GLUT_SINGLE)
	call glutinitwindowsize(500, 150)
	call glutinit
	win = glutcreatewindow('Fortran GLUT bitmap A')
	call fglclearcolor(0.0, 0.0, 0.0, 1.0)
	call glutdisplayfunc(display)
	call glutreshapefunc(reshape)
	call glutmainloop
	end

