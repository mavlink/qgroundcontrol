
/* hanoi.c - written by Greg Humphreys while an intern at SGI */

#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

double WIDTH = 800;
double HEIGHT = 800;

GLboolean motion = GL_TRUE;
GLboolean back_wall = GL_FALSE;
GLint xangle = 0, yangle = 0;
GLint xlangle = 0, ylangle = 0;

#define other(i,j) (6-(i+j))
#define wallz -(WIDTH/2)
#define DISK_HEIGHT 20
int NUM_DISKS = 11;
#define CONE NUM_DISKS+1
#define WALL CONE + 1
#define HANOI_SOLVE 0
#define HANOI_QUIT 1
#define HANOI_LIGHTING 2
#define HANOI_WALL 3
#define HANOI_FOG 4

GLfloat lightOneDirection[] =
{0, 0, -1};
GLfloat lightOnePosition[] =
{200, 100, 300, 1};
GLfloat lightOneColor[] =
{1.0, 1.0, 0.5, 1.0};

GLfloat lightTwoDirection[] =
{0, 0, -1};
GLfloat lightTwoPosition[] =
{600, 100, 300, 1};
GLfloat lightTwoColor[] =
{1.0, 0.0, 0.3, 1.0};

GLfloat lightZeroPosition[] =
{400, 200, 300, 1};
GLfloat lightZeroColor[] =
{.3, .3, .3, .3};

GLfloat diskColor[] =
{1.0, 1.0, 1.0, .8}, poleColor[] =
{1.0, 0.2, 0.2, .8};

typedef struct stack_node {
  int size;
  struct stack_node *next;
} stack_node;

typedef struct stack {
  struct stack_node *head;
  int depth;
} stack;

stack poles[4];

void 
push(int which, int size)
{
  stack_node *node = malloc(sizeof(stack_node));
  if (!node) {
    fprintf(stderr, "out of memory!\n");
    exit(-1);
  }
  node->size = size;
  node->next = poles[which].head;
  poles[which].head = node;
  poles[which].depth++;
}

int 
pop(int which)
{
  int retval = poles[which].head->size;
  stack_node *temp = poles[which].head;
  poles[which].head = poles[which].head->next;
  poles[which].depth--;
  free(temp);
  return retval;
}

typedef struct move_node {
  int t, f;
  struct move_node *next;
  struct move_node *prev;
} move_node;

typedef struct move_stack {
  int depth;
  struct move_node *head, *tail;
} move_stack;

move_stack moves;

void 
init(void)
{
  int i;
  for (i = 0; i < 4; i++) {
    poles[i].head = NULL;
    poles[i].depth = 0;
  }
  moves.head = NULL;
  moves.tail = NULL;
  moves.depth = 0;

  for (i = 1; i <= NUM_DISKS; i++) {
    glNewList(i, GL_COMPILE);
    {
      glutSolidTorus(DISK_HEIGHT / 2, 5 * i, 15, 15);
    }
    glEndList();
  }
  glNewList(CONE, GL_COMPILE);
  {
    glutSolidCone(10, (NUM_DISKS + 1) * DISK_HEIGHT, 20, 20);
  }
  glEndList();
}

void 
mpop(void)
{
  move_node *temp = moves.head;
  moves.head = moves.head->next;
  free(temp);
  moves.depth--;
}

void 
mpush(int t, int f)
{
  move_node *node = malloc(sizeof(move_node));
  if (!node) {
    fprintf(stderr, "Out of memory!\n");
    exit(-1);
  }
  node->t = t;
  node->f = f;
  node->next = NULL;
  node->prev = moves.tail;
  if (moves.tail)
    moves.tail->next = node;
  moves.tail = node;
  if (!moves.head)
    moves.head = moves.tail;
  moves.depth++;
}

/* ARGSUSED1 */
void 
keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:             /* ESC */
  case 'q':
  case 'Q':
    exit(0);
  }
}

void 
update(void)
{
  glutPostRedisplay();
}

void 
DrawPost(int xcenter)
{
  glPushMatrix();
  {
    glTranslatef(xcenter, 0, 0);
    glRotatef(90, -1, 0, 0);
    glCallList(CONE);
  } glPopMatrix();
}

void 
DrawPosts(void)
{
  glColor3fv(poleColor);
  glLineWidth(10);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, poleColor);
  DrawPost((int) WIDTH / 4);
  DrawPost(2 * (int) WIDTH / 4);
  DrawPost(3 * (int) WIDTH / 4);
}

void 
DrawDisk(int xcenter, int ycenter, int size)
{
  glPushMatrix();
  {
    glTranslatef(xcenter, ycenter, 0);
    glRotatef(90, 1, 0, 0);
    glCallList(size);
  } glPopMatrix();
}

void 
DrawDooDads(void)
{
  int i;
  stack_node *temp;
  int xcenter, ycenter;
  glColor3fv(diskColor);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, diskColor);
  for (i = 1; i <= 3; i++) {
    xcenter = i * WIDTH / 4;
    for (temp = poles[i].head, ycenter = DISK_HEIGHT * poles[i].depth - DISK_HEIGHT / 2; temp; temp = temp->next, ycenter -= DISK_HEIGHT) {
      DrawDisk(xcenter, ycenter, temp->size);
    }
  }
}

#define MOVE(t,f) mpush((t),(f))

static void
mov(int n, int f, int t)
{
  int o;

  if (n == 1) {
    MOVE(t, f);
    return;
  }
  o = other(f, t);
  mov(n - 1, f, o);
  mov(1, f, t);
  mov(n - 1, o, t);
}

GLfloat wallcolor[] =
{0, .3, 1, 1};

void 
DrawWall(void)
{
  int i;
  int j;
  glColor3fv(wallcolor);
  for (i = 0; i < WIDTH; i += 10) {
    for (j = 0; j < HEIGHT; j += 10) {
      glBegin(GL_POLYGON);
      {
        glNormal3f(0, 0, 1);
        glVertex3f(i + 10, j, wallz);
        glVertex3f(i + 10, j + 10, wallz);
        glVertex3f(i, j + 10, wallz);
        glVertex3f(i, j, wallz);
      }
      glEnd();
    }
  }
}

void 
draw(void)
{
  int t, f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (back_wall) {
    glMaterialfv(GL_FRONT, GL_DIFFUSE, wallcolor);
    DrawWall();
  }
  glPushMatrix();
  {
    glTranslatef(WIDTH / 2, HEIGHT / 2, 0);
    glRotatef(xlangle, 0, 1, 0);
    glRotatef(ylangle, 1, 0, 0);
    glTranslatef(-WIDTH / 2, -HEIGHT / 2, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
  }
  glPopMatrix();

  glPushMatrix();
  {
    glTranslatef(WIDTH / 2, HEIGHT / 2, 0);
    glRotatef(xangle, 0, 1, 0);
    glRotatef(yangle, 1, 0, 0);
    glTranslatef(-WIDTH / 2, -HEIGHT / 2, 0);
    DrawPosts();
    DrawDooDads();
  }
  glPopMatrix();
  if (motion && moves.depth) {
    t = moves.head->t;
    f = moves.head->f;
    push(t, pop(f));
    mpop();
  }
  glutSwapBuffers();
}

void 
hanoi_menu(int value)
{
  switch (value) {
  case HANOI_SOLVE:
    motion = !motion;
    if(motion) {
      glutIdleFunc(update);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case HANOI_LIGHTING:
    if (glIsEnabled(GL_LIGHTING))
      glDisable(GL_LIGHTING);
    else
      glEnable(GL_LIGHTING);
    break;
  case HANOI_WALL:
    back_wall = !back_wall;
    break;
  case HANOI_FOG:
    if (glIsEnabled(GL_FOG))
      glDisable(GL_FOG);
    else {
      glEnable(GL_FOG);
      glFogi(GL_FOG_MODE, GL_EXP);
      glFogf(GL_FOG_DENSITY, .01);
    }
    break;
  case HANOI_QUIT:
    exit(0);
    break;
  }
  glutPostRedisplay();
}

int oldx, oldy;

GLboolean leftb = GL_FALSE, middleb = GL_FALSE;

void 
hanoi_mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    oldx = x;
    oldy = y;
    if (state == GLUT_DOWN)
      leftb = GL_TRUE;
    else
      leftb = GL_FALSE;
  }
  if (button == GLUT_MIDDLE_BUTTON) {
    oldx = x;
    oldy = y;
    if (state == GLUT_DOWN)
      middleb = GL_TRUE;
    else
      middleb = GL_FALSE;
  }
}

void 
hanoi_visibility(int state)
{
  if (state == GLUT_VISIBLE && motion) {
    glutIdleFunc(update);
  } else {
    glutIdleFunc(NULL);
  }
}

void 
hanoi_motion(int x, int y)
{
  if (leftb) {
    xangle -= (x - oldx);
    yangle -= (y - oldy);
  }
  if (middleb) {
    xlangle -= (x - oldx);
    ylangle -= (y - oldy);
  }
  oldx = x;
  oldy = y;
  glutPostRedisplay();
}

int 
main(int argc, char *argv[])
{
  int i;

  glutInit(&argc, argv);
  for(i=1; i<argc; i++) {
    if(!strcmp("-n", argv[i])) {
      i++;
      if(i >= argc) {
	printf("hanoi: number after -n is required\n");
	exit(1);
      }
      NUM_DISKS = atoi(argv[i]);
    }
  }

  glutInitWindowSize((int) WIDTH, (int) HEIGHT);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

  glutCreateWindow("Hanoi");

  glutDisplayFunc(draw);
  glutKeyboardFunc(keyboard);

  glViewport(0, 0, (GLsizei) WIDTH, (GLsizei) HEIGHT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, WIDTH, 0, HEIGHT, -10000, 10000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClearColor(0, 0, 0, 0);
  glClearDepth(1.0);

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

/*  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);  */

  glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOneColor);
  glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 10);
  glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, lightOneDirection);
  glEnable(GL_LIGHT1);

  glLightfv(GL_LIGHT2, GL_POSITION, lightTwoPosition);
  glLightfv(GL_LIGHT2, GL_DIFFUSE, lightTwoColor);
/*  glLightf(GL_LIGHT2,GL_LINEAR_ATTENUATION,.005); */
  glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 10);
  glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, lightTwoDirection);
  glEnable(GL_LIGHT2);

  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
  glEnable(GL_LIGHT0);

  glEnable(GL_LIGHTING);

  glutMouseFunc(hanoi_mouse);
  glutMotionFunc(hanoi_motion);
  glutVisibilityFunc(hanoi_visibility);

  glutCreateMenu(hanoi_menu);
  glutAddMenuEntry("Solve", HANOI_SOLVE);
  glutAddMenuEntry("Lighting", HANOI_LIGHTING);
  glutAddMenuEntry("Back Wall", HANOI_WALL);
  glutAddMenuEntry("Fog", HANOI_FOG);
  glutAddMenuEntry("Quit", HANOI_QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  init();

  for (i = 0; i < NUM_DISKS; i++)
    push(1, NUM_DISKS - i);
  mov(NUM_DISKS, 1, 3);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
