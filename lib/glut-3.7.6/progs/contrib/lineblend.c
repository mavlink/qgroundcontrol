
/* lineblend.c
 * 
 * To compile: cc -o lineblend lineblend.c -lGL -lGLU -lX11 -lglut -lXmu
 *
 * Usage: lineblend
 *
 * This is an puffed up version of the first GL assignment we had for my
 * graphics class: write a 2-d openGL program w/ color and interactivity.
 *
 * Left and middle buttons drawn colored lines, right button brings up a menu
 * with a few options.  If you draw for long enough and then hit pick
 * "redraw" (or resize or uncover the window) it takes so long to redraw
 * all the lines it is kind of like a kaleidoscope animation. Or something.
 *
 * Philip Winston - 2/11/95  (modified: 2/12)
 * pwinston@hmc.edu
 * http://www.cs.hmc.edu/people/pwinston
 *        
 */

#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define drand48() (((float) rand())/((float) RAND_MAX))
#endif

typedef enum {MENU_ALPHA_1, MENU_ALPHA_2,  MENU_ALPHA_3, MENU_ALPHA_4,
              MENU_COLOR_1, MENU_COLOR_2,  MENU_COLOR_3, MENU_COLOR_4,
              MENU_ANTI_ON, MENU_ANTI_OFF,
              MENU_ERASE,   MENU_REDRAW,   MENU_QUIT} MenuChoices;

typedef enum {SC1, SC2, SC3, SC4} ColorScheme;

int wwidth, wheight, downbtn = -1, downx, downy;

GLuint DispLists = 1;

GLfloat Alpha = 0.2;
int Color = SC1;

void myglInit(void)
{
  glLineWidth(10.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-0.5,0.5,-0.5,0.5);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glFlush();
} 

void myreshape(GLsizei w, GLsizei h)
{
  wwidth = w; wheight = h;
  glViewport(0,0,w,h);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void mydisplay(void)
{
  GLuint i;

  glClear(GL_COLOR_BUFFER_BIT);

  for (i = 0; i < DispLists; i++)
    glCallList(i);

  glFlush();
}

void drawline(int x, int y)
{
  GLfloat fx =   (float)(x - downx)/wwidth,
          fy = -((float)(y - downy)/wheight),
          r1, g1, b1, r2, g2, b2;

  switch(Color) {      /* four different ways to pick colors */
    case SC1:          /* just kind of random formulas...    */
      r1 = fy+1; g1 = 0;    b1 = (float)x/wwidth;
      r2 = 0;    g2 = fy+1; b2 = 1-(float)x/wwidth;
      break;
    case SC2:
      r1 = fx+fy+1; g1 = 0;       b1 = 0;
      r2 = 0;       g2 = fx+fy+1; b2 = 0;
      break;
    case SC3:
      r1 = 0;   g1 = (float)downx/wwidth; b1 = (float)x/wwidth;
      r2 = 0;   g2 = (float)y/wheight; b2 = (float)downy/wheight;
      break;
    case SC4:
      r1 = drand48(); g1 = drand48(); b1 = (float)downx/(wwidth*1.5);
      r2 = drand48(); g2 = (float)downy/(wheight*1.5); b2 = drand48();
      break;
  }


  glBegin(GL_LINES);
        glColor4f(r1, g1, b1, Alpha);
    switch(downbtn) {
      case GLUT_LEFT_BUTTON:   glVertex2f(0,0);     break;        
      case GLUT_MIDDLE_BUTTON: glVertex2f(-fx,-fy); break;
      }
    glColor4f(r2, g2, b2, Alpha); glVertex2f(fx,fy);
  glEnd();

  glFlush();
}

void mousebutton(int btn, int state, int x, int y)
{
  GLfloat fx =  (float)x/wwidth - 0.5,
          fy = -(float)y/wheight + 0.5;

  if (state == GLUT_DOWN && downbtn == -1) {
    glNewList(DispLists++, GL_COMPILE_AND_EXECUTE);
    glPushMatrix();
    glTranslatef(fx, fy, 0);
    downbtn = btn;
    downx = x;
    downy = y;
    drawline(x, y);
  } else if (state == GLUT_UP && btn == downbtn) {
    glPopMatrix();
    glEndList();
    downbtn = -1;
  }
}

/*
 * For some reason I felt like doing these check boxes -- it is kind of
 * lame the way I did it, though.  Probably a smarter and easier way.
 */

void handlealphamenu(int value)
{
  glutChangeToMenuEntry(1,"[   ] 0.05", MENU_ALPHA_1);  
  glutChangeToMenuEntry(2,"[   ] 0.20", MENU_ALPHA_2);
  glutChangeToMenuEntry(3,"[   ] 0.50", MENU_ALPHA_3);
  glutChangeToMenuEntry(4,"[   ] 1.00", MENU_ALPHA_4);
  switch (value) {
    case MENU_ALPHA_1:
      glutChangeToMenuEntry(1, "[ * ] 0.05", MENU_ALPHA_1);  
      Alpha = 0.05; break;
    case MENU_ALPHA_2:
      glutChangeToMenuEntry(2, "[ * ] 0.20", MENU_ALPHA_2);
      Alpha = 0.2; break;
    case MENU_ALPHA_3:
      glutChangeToMenuEntry(3, "[ * ] 0.50", MENU_ALPHA_3);
      Alpha = 0.5; break;
    case MENU_ALPHA_4:
      glutChangeToMenuEntry(4, "[ * ] 1.00", MENU_ALPHA_4);
      Alpha = 1.0; break;
    }
}

void handlecolormenu(int value)
{
  glutChangeToMenuEntry(1,"[   ] Various", MENU_COLOR_1);  
  glutChangeToMenuEntry(2,"[   ] Red/Green  ", MENU_COLOR_2);
  glutChangeToMenuEntry(3,"[   ] Blue/Green", MENU_COLOR_3);
  glutChangeToMenuEntry(4,"[   ] Random", MENU_COLOR_4);
  switch (value) {
    case MENU_COLOR_1:
      glutChangeToMenuEntry(1, "[ * ] Various", MENU_COLOR_1);  
      Color = SC1; break;
    case MENU_COLOR_2:
      glutChangeToMenuEntry(2, "[ * ] Red/Green", MENU_COLOR_2);
      Color = SC2; break;
    case MENU_COLOR_3:
      glutChangeToMenuEntry(3, "[ * ] Blue/Green", MENU_COLOR_3);
      Color = SC3; break;
    case MENU_COLOR_4:
      glutChangeToMenuEntry(4, "[ * ] Random", MENU_COLOR_4);
      Color = SC4; break;
    }
}

void handlemenu(int value)
{
  switch (value) {
    case MENU_ANTI_OFF:
      glDisable(GL_LINE_SMOOTH);
      glutChangeToMenuEntry(3, "anti-aliasing [NO]", MENU_ANTI_ON);
      break;
    case MENU_ANTI_ON:
      glEnable(GL_LINE_SMOOTH);
      glutChangeToMenuEntry(3, "anti-aliasing [YES]", MENU_ANTI_OFF);
      break;
    case MENU_REDRAW:
      glutPostRedisplay();
      break;
    case MENU_ERASE:
      glDeleteLists(0, DispLists);
      DispLists = 1;
      mydisplay();
      break;
    case MENU_QUIT:
      exit(0);
      break;
    }
}

void myMenuInit(void)
{
  int sub1,sub2,sub3;

  sub3 = glutCreateMenu(handlealphamenu);
  glutAddMenuEntry("[   ] 0.05", MENU_ALPHA_1);  
  glutAddMenuEntry("[ * ] 0.20", MENU_ALPHA_2);
  glutAddMenuEntry("[   ] 0.50", MENU_ALPHA_3);
  glutAddMenuEntry("[   ] 1.00", MENU_ALPHA_4);
  sub2 = glutCreateMenu(handlecolormenu);
  glutAddMenuEntry("[ * ] Various", MENU_COLOR_1);  
  glutAddMenuEntry("[   ] Red/Green", MENU_COLOR_2);
  glutAddMenuEntry("[   ] Blue/Green", MENU_COLOR_3);
  glutAddMenuEntry("[   ] Random", MENU_COLOR_4);
  sub1 = glutCreateMenu(handlemenu);
  glutAddSubMenu("Colors", sub2);
  glutAddSubMenu("Alpha", sub3);
  glutAddMenuEntry("anti-aliasing [NO]", MENU_ANTI_ON);
  glutCreateMenu(handlemenu);
  glutAddSubMenu("Lines", sub1);
  glutAddMenuEntry("Erase", MENU_ERASE);
  glutAddMenuEntry("Redraw", MENU_REDRAW);
  glutAddMenuEntry("Quit", MENU_QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutCreateWindow("lineblend");

  myglInit(); 
  myMenuInit();

  glutReshapeFunc(myreshape);
  glutMouseFunc(mousebutton);
  glutMotionFunc(drawline);
  glutDisplayFunc(mydisplay);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

