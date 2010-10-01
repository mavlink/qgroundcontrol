/*  
 *  CS 453 - Final project : An OpenGL version of the pegboard game IQ
 *  Due : June 5, 1997
 *  Author : Kiri Wagstaff
 *
 *  File : gliq.c
 *  Description : Main board display file.
 *
 *  5/22 : Displays the board selection screen, and uses keyboard
 *  input to manipulate it and select.
 *
 */

#include "gliq.h"

/* globals */

#if 0
//GLuint cone;
#endif
int curstate;
int mouse_state=-1;
int mouse_button=-1;
int pegs=0;
int totalpegs=0;
int lastpicked = 0;

/* functions */
void init(void);
void reshape(int width, int height);
void display(void);
void special(int key, int x, int y);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void idle(void);

int main(int argc, char** argv)
{
  glutInit(&argc, argv);

  glutInitWindowSize(512, 512);
  glutInitWindowPosition(0, 0);
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("GLIQ");
  
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutPassiveMotionFunc(passive);
  glutIdleFunc(idle);
  
  init();
  
  glutMainLoop();
  return 0;
}


void init(void)
{
  int i, j;

  /* lighting */
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  /*  glEnable(GL_CULL_FACE);*/

  /* put the identity in the trackball transform */
  tbInit(GLUT_RIGHT_BUTTON);

  glSelectBuffer(SELECT_BUFFER, select_buffer);

  /* make the star cone */
  /*  cone = glGenLists(1);  
  glNewList(cone, GL_COMPILE);
  glPushMatrix();
  for (i=0; i<3; i++)
    {
      glRotatef(45.0, 1.0, 0.0, 0.0);
      glutSolidCone(0.2, 2.0, 8, 8);
    }
  glPopMatrix();
  glEndList();*/

  /* Initialize the state */
  for (i=0; i<BOARDSIZE; i++)
    for (j=0; j<BOARDSIZE; j++)
      filled[i][j] = UNUSED;
  curstate = SELBOARD;
  curboard = 0;
  readboards();
  readscores();

}

void reshape(int width, int height)
{
  GLfloat lightpos[4] = { 1.0, 1.0, 1.0, 1.0 };
  tbReshape(width, height);

  glViewport(0, 0, width, height);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  glTranslatef(0.0, -2.0, -15.0);
}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* draw */
  switch(curstate)
    {
    case SELBOARD:
      selectboard();
      break;
    case PLAY:
      playgame();
      break;
    case HIGHSC:
      highscore();
      break;
    case VIEWSCORES:
      showhighscores();
      break;
    default:
      printf("Unknown state %d, exiting.\n", curstate);
      exit(1);
    }

  glutSwapBuffers();
}

/* ARGSUSED1 */
void special(int key, int x, int y)
{
  switch (key) { 
    case GLUT_KEY_UP:
      break;

    case GLUT_KEY_DOWN:
      break;

    case GLUT_KEY_RIGHT:
      break;

    case GLUT_KEY_LEFT:
      break;
 }

  glutPostRedisplay();
}

/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 'h':
    printf("gliq help\n\n");
    printf("f            -  Filled\n");
    printf("w            -  Wireframe\n");
    printf("s            -  See high scores\n");
    printf("escape or q  -  Quit\n\n");
    break;

  case 'f':
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;

  case 'w':
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;

  case 's':
    curstate = VIEWSCORES;
    glutIdleFunc(idlescore);
    break;
    
  case 'q':
  case 27:
    exit(0);
    break;
  }

  glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
  int i, j;
  int mid=0;
  
  mouse_state = state;
  mouse_button = button;

  if (state == GLUT_DOWN && button==GLUT_LEFT_BUTTON)
    switch(curstate)
      {
      case SELBOARD:
	switch(picked)
	  {
	  case NONE:
	    break;
	  case LEFTARR:
	    curboard--;
	    totalpegs = 0;
	    if (curboard<0)
	      curboard = numboards-1;
	    /* Set up filled array */
	    for (i=0; i<BOARDSIZE; i++)
	      for (j=0; j<BOARDSIZE; j++)
		{
		  filled[i][j] = boards[curboard][i][j];
		  if (filled[i][j] == FULL)
		    totalpegs++;
		}
	    break;	    
	  case SELECT:
	    totalpegs = 0;
	    /* Set up filled array */
	    for (i=0; i<BOARDSIZE; i++)
	      for (j=0; j<BOARDSIZE; j++)
		{
		  filled[i][j] = boards[curboard][i][j];
		  if (filled[i][j] == FULL)
		    totalpegs++;
		}
	    curstate = PLAY;
	    playdone = 0;
	    pegs = totalpegs;
	    glutIdleFunc(NULL);
	    break;
	  case RIGHTARR:
	    curboard++;
	    totalpegs = 0;
	    if (curboard>=numboards)
	      curboard = 0;
	    /* Set up filled array */
	    for (i=0; i<BOARDSIZE; i++)
	      for (j=0; j<BOARDSIZE; j++)
		{
		  filled[i][j] = boards[curboard][i][j];
		  if (filled[i][j] == FULL)
		    totalpegs++;
		}
	    break;
	  case QUIT:
	    exit(0);
	  default:
	    printf("picked is %d.\n", picked);
	  }
	break;
      case PLAY:
#if 0
//	printf("picked is %d\n", picked);
#endif
	if (picked == 0)
	  break;
	if (picked == QUIT)
	  {
	    if (pegs < minscore || (pegs==minscore && totalpegs > minpegs))
	      {
		curstate = HIGHSC;
		numentered = 0;
		written = 0;
		glutKeyboardFunc(keyscores);
		glutIdleFunc(idlescore);
		break;
	      }
	    curstate = SELBOARD;
	    glutIdleFunc(idle);
	    totalpegs = 0;
	    for (i=0; i<BOARDSIZE; i++)
	      for (j=0; j<BOARDSIZE; j++)
		{
		  filled[i][j] = boards[curboard][i][j];
		  if (filled[i][j] == FULL)
		    totalpegs++;
		}
	    break;
	  }
	if (filled[(picked-1)/BOARDSIZE][(picked-1)%BOARDSIZE] == FULL)
	  {
	    if (canmove(picked))
	      filled[(picked-1)/BOARDSIZE][(picked-1)%BOARDSIZE] = CANMOVE;
	    else
	      {
		filled[(picked-1)/BOARDSIZE][(picked-1)%BOARDSIZE] = CANTMOVE;
#if 0
		Beep(1000, 40);
		PlaySound("SPR_OUCH.WAV", NULL, SND_FILENAME);
#endif
	      }
	    lastpicked = picked;
	  }
	else
	  lastpicked = 0;
	break;
      case HIGHSC:
	if (written)
	  {
	    curstate = VIEWSCORES;
	    glutKeyboardFunc(NULL);
	    glutIdleFunc(idlescore);
	  }
	break;
      case VIEWSCORES:
	curstate = SELBOARD;
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	totalpegs = 0;
	for (i=0; i<BOARDSIZE; i++)
	  for (j=0; j<BOARDSIZE; j++)
	    {
	      filled[i][j] = boards[curboard][i][j];
	      if (filled[i][j] == FULL)
		totalpegs++;
	    }
	break;
      default:
	printf("Unknown state %d, exiting.\n", curstate);
	exit(1);
      }

  /* Release a button, reset the array */
  else if (state==GLUT_UP && button==GLUT_LEFT_BUTTON)
    switch (curstate)
      {
      case SELBOARD:
	break;
      case PLAY:
	if (picked <= 0)
	  break;
	if ((mid = legalmove()))
	  {
#if 0
	    //	    printf("Erasing (%d,%d).", (mid-1)/BOARDSIZE, (mid-1)%BOARDSIZE);
#endif
	    filled[(lastpicked-1)/BOARDSIZE][(lastpicked-1)%BOARDSIZE] = EMPTY;
	    filled[(mid-1)/BOARDSIZE][(mid-1)%BOARDSIZE] = EMPTY;
	    filled[(picked-1)/BOARDSIZE][(picked-1)%BOARDSIZE] = FULL;
	    pegs--;
	    /* Check for any legal moves left */
	    if (!movesexist())
	      {
		/* Display score & "no moves left" */
		printf("No moves remaining.  You finished with %d pegs left.\n",
		       pegs);
		playdone = 1;
#if 0
		//		exit(0);
#endif
	      }
	  }
	else if (lastpicked != 0)
	  filled[(lastpicked-1)/BOARDSIZE][(lastpicked-1)%BOARDSIZE] = FULL;
	break;
      case HIGHSC:
	break;
      case VIEWSCORES:
	break;
      default:
	printf("Unknown state %d, exiting.\n", curstate);
	exit(1);
      }

  else
    tbMouse(button, state, x, y);

#if 0
  else if (state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON)
    tbStartMotion(x, y, button, glutGet(GLUT_ELAPSED_TIME));
  else if (state == GLUT_UP && button == GLUT_RIGHT_BUTTON)
    tbStopMotion(button, glutGet(GLUT_ELAPSED_TIME));
#endif

  glutPostRedisplay();

}

void motion(int x, int y)
{
  tbMotion(x, y);
  if (mouse_button == GLUT_LEFT_BUTTON)
    picked = pick(x,y);
  
  glutPostRedisplay();
}


void idle(void)
{
  display();
}
  
