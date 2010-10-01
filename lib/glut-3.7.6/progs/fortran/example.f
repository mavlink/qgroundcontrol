
C  Copyright (c) Mark J. Kilgard, 1994.

C  This program is freely distributable without licensing fees
C  and is provided without guarantee or warrantee expressed or
C  implied.  This program is -not- in the public domain.

C  GLUT Fortran example; touches a reasonable amount of GLUT
C  callback functionality.

	subroutine display
#include "GL/fgl.h"
	call fglclear(GL_COLOR_BUFFER_BIT)
	call fglfinish
	end

	subroutine passive(x,y)
	integer x,y
	print *,'passive motion',x,y
	end

	subroutine submenu(value)
	integer value
	print *,'value is',value
	end

	subroutine mainmenu(value)
	integer value
	print *,'main menu value is',value
	end

	subroutine timer(value)
	integer value
	print *,'timer value',value
	end

	subroutine mouse(btn,state,x,y)
#include "GL/fglut.h"
	external timer
	integer btn,state,x,y
	print *,'mouse',btn,state,x,y
	call gluttimerfunc(1000,timer,25)
	end

	subroutine idle()
#include "GL/fglut.h"
	integer count
	print *,'idle called'
	call glutidlefunc(glutnull)
	end

	subroutine keyboard(key,x,y)
	external idle
	integer key,x,y
	print *,'keyboard',key,x,y
	call glutidlefunc(idle)
	end

	subroutine tablet(x,y)
	integer x,y
	print *,'tablet motion',x,y
	end

	subroutine tbutton(button,state)
	integer button,state
	print *,'tablet button',button,state
	end

	subroutine dials(dial,value)
	integer dial,value
	print *,'dial movement',dial,value
	end

	subroutine box(button,state)
	integer button,state
	print *,'button box',button,state
	end

	program main
#include "GL/fglut.h"
	external display
	external passive
	external submenu
	external mainmenu
	external mouse
	external keyboard
	external tablet
	external tbutton
	external dials
	external box
	call glutinit
	print *,glutcreatewindow('Fortran GLUT program')
	call glutdisplayfunc(display)
	call glutpassivemotionfunc(passive)
	call glutmousefunc(mouse)
	call glutkeyboardfunc(keyboard)
	call gluttabletmotionfunc(tablet)
	call gluttabletbuttonfunc(tbutton)
	call glutdialsfunc(dials)
	call glutbuttonboxfunc(box)
	i = glutcreatemenu(submenu)
	call glutaddmenuentry('something', 4)
	call glutaddmenuentry('another thing', 5)
	j = glutcreatemenu(mainmenu)
	call glutaddsubmenu('submenu', i)
	call glutaddmenuentry('quit', 666)
	call glutattachmenu(2)
	print *,'Number of button box buttons:',
     2    glutdeviceget(GLUT_NUM_BUTTON_BOX_BUTTONS)
	print *,'Number of dials:',glutdeviceget(GLUT_NUM_DIALS)
	print *,'Depth buffer size',glutget(GLUT_WINDOW_DEPTH_SIZE)
	call glutmainloop
	end

