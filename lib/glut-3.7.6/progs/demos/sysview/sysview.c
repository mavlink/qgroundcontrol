
/*
 * Systems statistics viewing application
 *  (C) Javier Velasco	'97 (almost '98)
 *	fjvelasco@sinix.net
 *
 *  This application was developed on an INDIGO2 Extreme and makes use of
 *  a SGI system call (sginap) that sleeps the process for a given number of
 *  clock ticks. For other UNIX systems, this call should be substituted for 
 *  another proper call.
 *  The default number of ticks between samples is 20. This can be changed
 *  through the mouse right button menu.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysmp.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <unistd.h>

/* GL includes */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>


/*
 * Macros
 */
#define GRID		0x22
#define ZGRID		0x23
#define XGRID		0x24
#define YGRID		0x25

float lastx=0;	/* mouse track */
float lasty=0;

void *font1 = GLUT_BITMAP_9_BY_15; /* used fonts */
void *font2 = GLUT_BITMAP_8_BY_13;

long nPeriod=20;	    /* default sampling rate in ticks */
GLsizei nWidth, nHeight;    /* current window size */
GLboolean bMotion=False;

struct sysinfo SysInfo, LastSysInfo;	/* system information */

/*
 * Mouse motion track
 */
void MouseMove(int x, int y)
{
    if(bMotion)
	{
	lastx = x;
	lasty = y;
	glutPostRedisplay();    
	}
}

/*
 * 3D Perspective projection setup
 */
void Make3DLook(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 4.0/3.0, 0.0, 500.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();    
}

/*
 * 2D Orthographic projections setup
 */
void Make2DLook(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, nWidth, nHeight, 0);
  glMatrixMode(GL_MODELVIEW);
}

/*
 * Toggle line antialias 
 */
void ToggleAAlias(void)
{
    if(glIsEnabled(GL_LINE_SMOOTH))
	{
	glDisable(GL_LINE_SMOOTH);	    
	glDisable(GL_BLEND);	    
	}
    else
	{
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	}
}


/*
 * Draw a string
 */
void glPuts(GLint x, GLint y, char *string, void *font)
{
  int len, i;

  glRasterPos2i(x, y);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(font, string[i]);
  }
}

/*
 * Draw the 3d grid
 */
void DrawGrid(void)
{
    glCallList(GRID);
}

/*
 * Display the legend
 */
void Legend(void)
{
    glPuts(5, 40, "User", font2);    
    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glTranslatef(10.0, 50.0, 0.0);
    glColor3f(0.0, 0.0, 1.0);
    glRecti(0, 0, 20, 20);
    glPopAttrib();
    glPopMatrix();
    
    glPuts(5, 90, "Kernel", font2);    
    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glTranslatef(10.0, 100.0, 0.0);
    glColor3f(1.0, 0.0, 0.0);
    glRecti(0, 0, 20, 20);
    glPopAttrib();
    glPopMatrix();

    glPuts(5, 140, "Intr", font2);    
    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glTranslatef(10.0, 150.0, 0.0);
    glColor3f(1.0, 1.0, 0.0);
    glRecti(0, 0, 20, 20);
    glPopAttrib();
    glPopMatrix();

    glPuts(5, 190, "Idle", font2);    
    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glTranslatef(10.0, 200.0, 0.0);
    glColor3f(0.0, 1.0, 0.0);
    glRecti(0, 0, 20, 20);
    glPopAttrib();
    glPopMatrix();
    
    glPuts(5, 240, "Wait", font2);    
    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glTranslatef(10.0, 250.0, 0.0);
    glColor3f(0.0, 1.0, 1.0);
    glRecti(0, 0, 20, 20);
    glPopAttrib();
    glPopMatrix();
    
    glPuts(nWidth-30, nHeight-30, "0", font1); 
    glPuts(nWidth-45, 45, "100", font1); 
}



/*
 * Construct the grid display list
 */
void MakeGrid(void)
{
    int i;
    
    /* let's divide the grid in three pieces */
    glNewList(ZGRID, GL_COMPILE_AND_EXECUTE);
    glBegin(GL_LINES);
    for(i=0;i<=30;i+=3)
	{
	glVertex2d(i, 0);
	glVertex2d(i, 30);
	glVertex2d(0, i);
	glVertex2d(30, i);
	}
    glEnd();
    glEndList();
    
        
    glNewList(XGRID, GL_COMPILE_AND_EXECUTE);
    glPushMatrix();
    glRotatef(90.0, 1.0, 0.0, 0.0);    
    glBegin(GL_LINES);
    for(i=0;i<=6;i+=3)
	{
	glVertex2d(0, i);
	glVertex2d(30, i);
	}
    for(i=0;i<=30;i+=3)
	{
	glVertex2d(i, 0);
	glVertex2d(i, 6);
	} 
    glEnd();
    glPopMatrix();
    glEndList();

    glNewList(YGRID, GL_COMPILE_AND_EXECUTE);
    glPushMatrix();
    glRotatef(-90.0, 0.0, 1.0, 0.0);    
    glBegin(GL_LINES);
    for(i=0;i<=30;i+=3)
	{
	glVertex2d(0, i);
	glVertex2d(6, i);
	}
    for(i=0;i<=6;i+=3)
	{
	glVertex2d(i, 0);
	glVertex2d(i, 30);
	} 
    glEnd();
    glPopMatrix();
    glEndList();

    /* now call them all */
    glNewList(GRID, GL_COMPILE_AND_EXECUTE);
	glLineWidth(2.0);
	glColor3f(0.7, 0.7, 0.7);
	glCallList(ZGRID);
	glCallList(XGRID);
	glCallList(YGRID);
	glLineWidth(1.0);
    glEndList();
}

/*
 * System statistics collection routine
 */
void ComputeStatistics(void)
{
    memcpy(&LastSysInfo, &SysInfo, sizeof(SysInfo));
    sysmp(MP_SAGET, MPSA_SINFO, &SysInfo, sizeof(SysInfo));
    sginap(nPeriod);
    glutPostRedisplay();
}

/*
 * Draw a coloured cube for each one of the monitored parameters
 */
void Draw3DStatistics(void)
{
    GLdouble fSize;

    glPushMatrix();
    glTranslatef(0.0, 0.0, 1.5);
    /* Time in user mode */
    glPushMatrix();
    fSize = SysInfo.cpu[CPU_USER] - LastSysInfo.cpu[CPU_USER] ;
    fSize = fSize*30/nPeriod;
    glTranslatef(3.0, fSize/2, 0.0);
    glScalef(6.0, fSize, 3.0);
    glColor3f(0.0, 0.0, 1.0);
    glutSolidCube(1.0);
    glColor3f(0.0, 0.0, 0.0);
    glutWireCube(1.0);
    glPopMatrix();
    
    /* Time in kernel mode */
    glPushMatrix();
    fSize = SysInfo.cpu[CPU_KERNEL] - LastSysInfo.cpu[CPU_KERNEL] ;
    fSize = fSize*30/nPeriod;
    glTranslatef(9.0, fSize/2, 0.0);
    glScalef(6.0, fSize, 3.0);
    glColor3f(1.0, 0.0, 0.0);
    glutSolidCube(1.0);
    glColor3f(0.0, 0.0, 0.0);
    glutWireCube(1.0);
    glPopMatrix();

   /* Time in interrupt mode */
    glPushMatrix();
    fSize = SysInfo.cpu[CPU_INTR] - LastSysInfo.cpu[CPU_INTR] ;
    fSize = fSize*30/nPeriod;
    glTranslatef(15.0, fSize/2, 0.0);
    glScalef(6.0, fSize, 3.0);
    glColor3f(1.0, 1.0, 0.0);
    glutSolidCube(1.0);
    glColor3f(0.0, 0.0, 0.0);
    glutWireCube(1.0);
    glPopMatrix();
  
   /* Time in idle */
    glPushMatrix();
    fSize = SysInfo.cpu[CPU_IDLE] - LastSysInfo.cpu[CPU_IDLE] ;
    fSize = fSize*30/nPeriod;
    glTranslatef(21.0, fSize/2, 0.0);
    glScalef(6.0, fSize, 3.0);
    glColor3f(0.0, 1.0, 0.0);
    glutSolidCube(1.0);
    glColor3f(0.0, 0.0, 0.0);
    glutWireCube(1.0);
    glPopMatrix();
  
   /* Time in wait mode */
    glPushMatrix();
    fSize = SysInfo.cpu[CPU_WAIT] - LastSysInfo.cpu[CPU_WAIT] ;
    fSize = fSize*30/nPeriod;
    glTranslatef(27.0, fSize/2, 0.0);
    glScalef(6.0, fSize, 3.0);
    glColor3f(0.0, 1.0, 1.0);
    glutSolidCube(1.0);
    glColor3f(0.0, 0.0, 0.0);
    glutWireCube(1.0);
    glPopMatrix();
        
    glPopMatrix();
}


/*
 * Resize routine
 */
void Resize3DWindow(int newWidth, int newHeight)
{
    glViewport(0, 0, (GLint)newWidth, (GLint)newHeight);
    nWidth = newWidth;
    nHeight = newHeight;
    Make3DLook();
    glClear(GL_COLOR_BUFFER_BIT);
}


/*
 * Display routine
 */
void Doit3D(void)
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Make2DLook();
    glColor3f(1.0, 1.0, 0.0);
    glPuts(15, 15, "CPU Activity", font1);
    Legend();
    Make3DLook();
    glPushMatrix();
    glTranslatef(-15.0, -15.0, -50.0);
    glRotatef (lastx, 0.0, 1.0, 0.0);
    glRotatef (lasty, 1.0, 0.0, 0.0);
    DrawGrid();
    Draw3DStatistics();
    glFlush();
    glPopMatrix();
    glutSwapBuffers();
}

/*
 * Menus routines
 */
void SelectSampleRate(int pick)
{
    nPeriod = pick;
}

void MainMenu(int pick)
{
    switch(pick)  
	{
	case 0:
	    bMotion = !bMotion;
	    break;
	case 1:
	    ToggleAAlias();
	    break;	    
	case 2:
	    exit(0);
	    break;	    
	}
}

/*
 * Menu creation
 */
void SetUpMenu(void)
{
    int SampleMenu;
    
    SampleMenu = glutCreateMenu(SelectSampleRate);
    glutAddMenuEntry("1", 1);
    glutAddMenuEntry("5", 5);
    glutAddMenuEntry("10", 10);
    glutAddMenuEntry("20", 20);
    glutCreateMenu(MainMenu);
    glutAddMenuEntry("Mouse Motion", 0);
    glutAddMenuEntry("Line antialias", 1);
    glutAddSubMenu("Sample rate", SampleMenu);
    glutAddMenuEntry("Quit", 2);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

}

/*
 * MAIN STUFF
 */
int main(int argc,  char **argv)
{
    GLenum type;
    
    type = GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH;
    glutInit(&argc, argv);
    glutInitDisplayMode(type);
    glutCreateWindow(argv[0]);    
    SetUpMenu();
    
    MakeGrid();

    glutReshapeFunc(Resize3DWindow);
    glutDisplayFunc(Doit3D);
    glutMotionFunc(MouseMove);
    glutIdleFunc(ComputeStatistics);
    glutMainLoop();
    return(0);
}

