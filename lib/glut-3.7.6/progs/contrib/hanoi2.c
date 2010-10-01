#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <GL/glut.h>

#define MAX_DISKS 6

#define DISK_HEIGHT 0.2
#define CYL_RADIUS 0.1
#define CYL_HEIGHT 1.8
#define OVER_CLICKS 120
#define UP_CLICKS 120

enum {X=0, Y, Z};
enum {DL_POLE=1, DL_FLOOR, DL_DISK};		       /* disk must be last */

/* motion states */
enum {ST_UP=1, ST_TO_1, ST_TO_2, ST_DOWN, ST_READNEXT, ST_IDLE};

int motion = 1;
int spinning = 1;
int click = 0;
int delay = 0;
int direction;
int state=ST_READNEXT;
int c_pole;
int old_pole;
int c_disk;
int engine_fd;
int engine_pid;

float pole_offset[3][2] = { { 0, -0.575 },
			    { 1.15, 0.575 },
                            { -1.15, 0.575 } };
float disk_offset[MAX_DISKS][3] = {{ 0, -0.575, 0},
				   { 0, -0.575, 0},   
				   { 0, -0.575, 0},   
				   { 0, -0.575, 0},   
				   { 0, -0.575, 0},   
				   { 0, -0.575, 0}};
float disk_incr[3] = { 0, 0, 0 };

int num_disks;
struct {
  int num_disks;
  int disks[MAX_DISKS];
} disks_on_poles[3];

/*
 * create display list for disk of outside radius orad
 */
void
diskdlist(int dlist, float orad)
{
    GLUquadricObj *obj;

    obj = gluNewQuadric();
    if (obj == 0) {
	perror("can't alloc quadric");
	exit(1);
    }
    glNewList(dlist, GL_COMPILE);
    glPushMatrix();
      glColor4ub(205, 67, 100, 200);
      gluQuadricDrawStyle(obj, GLU_FILL);

      gluQuadricOrientation(obj, GLU_OUTSIDE);
      gluCylinder(obj, orad, orad, DISK_HEIGHT, 20, 4);

      gluQuadricOrientation(obj, GLU_INSIDE);
      gluCylinder(obj, CYL_RADIUS, CYL_RADIUS, DISK_HEIGHT, 20, 4);

      gluQuadricOrientation(obj, GLU_INSIDE);
      gluDisk(obj, CYL_RADIUS, orad, 20, 4);

      gluQuadricOrientation(obj, GLU_OUTSIDE);
      glPushMatrix();
        glTranslatef(0.0, 0.0, DISK_HEIGHT);
        gluDisk(obj, CYL_RADIUS, orad, 20, 4);
      glPopMatrix();
    glPopMatrix();
    glEndList();
}

/*
 * create display list for pole
 */
void
poledlist(int dlist)
{
    GLUquadricObj *obj;

    obj = gluNewQuadric();
    if (obj == 0) {
	perror("can't alloc quadric");
	exit(1);
    }

    glNewList(dlist, GL_COMPILE);
    glPushMatrix();
      glColor3ub(67, 205, 128);
      gluQuadricDrawStyle(obj, GLU_FILL);
      gluQuadricOrientation(obj, GLU_OUTSIDE);

      gluCylinder(obj, CYL_RADIUS, CYL_RADIUS, CYL_HEIGHT, 12, 3);

      gluQuadricOrientation(obj, GLU_INSIDE);
      gluDisk(obj, 0.0, CYL_RADIUS, 12, 3);

      gluQuadricOrientation(obj, GLU_OUTSIDE);
      glPushMatrix();
        glTranslatef(0.0, 0.0, CYL_HEIGHT);
        gluDisk(obj, 0.0, CYL_RADIUS, 12, 3);
      glPopMatrix();
    glPopMatrix();
    glEndList();

}

/*
 * create display list for floor
 */
void
floordlist(int dlist)
{
    glNewList(dlist, GL_COMPILE);
    glPushMatrix();
      glColor4ub(90, 100, 230,100);

      /* top/bottom */
      glBegin(GL_TRIANGLE_STRIP);
      glNormal3f(0.0, 0.0, 1.0);
      glVertex3f(-2.0, -2.0, 0);
      glVertex3f(2.0, -2.0, 0);
      glVertex3f(-2.0, 2.0, 0);
      glVertex3f(2.0, 2.0, 0);
      glEnd();
      glPushMatrix();
        glTranslatef(0, 0, -0.2);
        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.0, 0.0, -1.0);
        glVertex3f(2.0, -2.0, 0);
        glVertex3f(-2.0, -2.0, 0);
        glVertex3f(2.0, 2.0, 0);
        glVertex3f(-2.0, 2.0, 0);
        glEnd();
      glPopMatrix();

      /* edges */
      glBegin(GL_TRIANGLE_STRIP);
      glNormal3f(-1.0, 0.0, 0.0);
      glVertex3f(-2.0, -2.0, -0.2);
      glVertex3f(-2.0, -2.0, 0);
      glVertex3f(-2.0, 2.0, -0.2);
      glVertex3f(-2.0, 2.0, 0);
      glNormal3f(0.0, 1.0, 0.0);
      glVertex3f(2.0, 2.0, -0.2);
      glVertex3f(2.0, 2.0, 0);
      glNormal3f(1.0, 0.0, 0.0);
      glVertex3f(2.0, -2.0, -0.2);
      glVertex3f(2.0, -2.0, 0);
      glNormal3f(0.0, -1.0, 0.0);
      glVertex3f(-2.0, -2.0, -0.2);
      glVertex3f(-2.0, -2.0, 0);
      glEnd();
    glPopMatrix();
    glEndList();
}

/*
 * motion state machine -- idle loop
 */
void
idle(void)
{
  static int over_clicks;
  int rc;
  char next_move[3];

  if (spinning)
    click++;

  if (motion) {
      switch(state) {
	case ST_READNEXT:
	  /*
	   * read an instruction from the hanoi engine
	   */
	  rc = read(engine_fd, next_move, 3);
	  if (rc == 3 && next_move[0] == 'M') {
	      /* choose poles/disks to move */
 	      old_pole = next_move[1];
	      c_pole = next_move[2];
	      c_disk = disks_on_poles[old_pole].disks[disks_on_poles[old_pole].num_disks - 1];
	      state = ST_UP;
	      disk_incr[Z] = CYL_HEIGHT / (float)UP_CLICKS;
	  }
	  else if (rc == 3 && next_move[0] == 'D') {
	      state = ST_IDLE;
	  }
	  else if (rc != 0) {
	      fprintf(stderr,"bad read; %d, [%d%d%d]\n",
		      rc, next_move[0], next_move[1], next_move[2]);
	      exit(1);
	  }
	  /* if rc == 0, do nothing this frame */
	  break;

	case ST_UP:
	  disk_offset[c_disk][Z] += disk_incr[Z];
	  if (disk_offset[c_disk][Z] >= (CYL_HEIGHT+0.1)) {
	      state = ST_TO_1;
	      over_clicks = OVER_CLICKS;
	      disk_incr[X] = (pole_offset[c_pole][X] 
			      - pole_offset[old_pole][X]) / (float)(over_clicks-1);
	      disk_incr[Y] = (pole_offset[c_pole][Y] 
			      - pole_offset[old_pole][Y]) / (float)(over_clicks-1);
	      disk_incr[Z] = 0.0;
	  }
	  break;

	case ST_DOWN:
	  disk_offset[c_disk][Z] -= disk_incr[Z];
	  if (disk_offset[c_disk][Z] <= (disks_on_poles[c_pole].num_disks*0.2+disk_incr[Z])) {
	      disk_offset[c_disk][Z] = disks_on_poles[c_pole].num_disks*0.2;
	      disks_on_poles[old_pole].num_disks --;
	      disks_on_poles[c_pole].disks[disks_on_poles[c_pole].num_disks ++] = c_disk;
	      state = ST_READNEXT;
	  }
	  break;

	case ST_TO_1:
	case ST_TO_2:
	  disk_offset[c_disk][X] += disk_incr[X];
	  disk_offset[c_disk][Y] += disk_incr[Y];
	  over_clicks --;
	  if (over_clicks == 0) {
	      state = ST_DOWN;
	      disk_incr[X] = 0.0;
	      disk_incr[Y] = 0.0;
	      disk_incr[Z] = CYL_HEIGHT / (float)UP_CLICKS;
	      disk_offset[c_disk][X] = pole_offset[c_pole][X]; /* paranoia */
	      disk_offset[c_disk][Y] = pole_offset[c_pole][Y];
	  }
	  break;

	case ST_IDLE:
	  break;
      }
  }
  glutPostRedisplay();
}

void
draw_scene(void)
{
  int i;
  glPushMatrix();
    glRotatef(click, 0, 0, 1);
    glRotatef(click/5.0, 1, 0, 0);
    for (i=0; i<3; i++) {
      glPushMatrix();
	glTranslatef(pole_offset[i][X], pole_offset[i][Y], 0);
        glCallList(DL_POLE);
      glPopMatrix();
    }
    for(i=0; i<num_disks; i++) {
      glPushMatrix();
	glTranslatef(disk_offset[i][X], disk_offset[i][Y], disk_offset[i][Z]);
	glCallList(DL_DISK+i);
      glPopMatrix();
    }
#ifndef TOOSLOW
    glCallList(DL_FLOOR);
#endif
  glPopMatrix();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw_scene();

  glutSwapBuffers();
}

void
decide_animation(void)
{
  if(motion || spinning) {
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}

void
visible(int state)
{
  if (state == GLUT_VISIBLE) {
    decide_animation();
  } else {
    glutIdleFunc(NULL);
  }
}

void
menu(int value)
{
  switch (value) {
  case 2:
    motion = 1 - motion;
    decide_animation();
    break;
  case 3:
    spinning = 1 - spinning;
    decide_animation();
    break;
  case 666:
    kill(engine_pid, SIGTERM);
    exit(0);
  }
}

int
main(int argc, char *argv[])
{
  int c;
  int i;

  GLfloat mat_specular[] = { 0.7, 0.7, 0.7, 1.0 };
  GLfloat mat_shininess[] = { 40.0 };
  GLfloat light_position[] = { 4.5, 0.0, 4.5, 0.0 };

  glutInit(&argc, argv);

  num_disks = MAX_DISKS;
  while((c = getopt(argc, argv, "n:s:m:")) != -1) {
      switch (c) {
	case 'n':
	  num_disks = atoi(optarg);
	  if (num_disks < 1 || num_disks > MAX_DISKS) {
	      num_disks = MAX_DISKS;
	  }
	  break;
	case 's':
	  spinning = atoi(optarg) ? 1 : 0;
	  break;
	case 'm':
	  motion = atoi(optarg) ? 1 : 0;
	  break;
	default:
	  break;
      }
  }

  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("Hanoi");
  glutDisplayFunc(display);
  glutVisibilityFunc(visible);
  glMatrixMode(GL_PROJECTION);
  gluPerspective(40.0, 1.0, 0.1, 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0, 5.5, 3.5,
    0, 0, 0,
    0, 0, 1);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
#ifndef TOOSLOW
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
  glEnable(GL_COLOR_MATERIAL);
#endif
#ifndef TOOSLOW
  glShadeModel(GL_SMOOTH);
#endif
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
#ifndef TOOSLOW
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
#endif
  glDepthFunc(GL_LEQUAL);
  glClearColor(0.3, 0.3, 0.3, 0.0);
#ifndef TOOSLOW
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

  glPolygonMode(GL_FRONT, GL_FILL);

  glutCreateMenu(menu);
  glutAddMenuEntry("Toggle motion", 2);
  glutAddMenuEntry("Toggle spinning", 3);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
#if defined(GL_POLYGON_OFFSET_EXT)
  if (glutExtensionSupported("GL_EXT_polygon_offset")) {
    glPolygonOffsetEXT(0.5, 0.0);
    glEnable(GL_POLYGON_OFFSET_EXT);
  }
#endif

  poledlist(DL_POLE);
  floordlist(DL_FLOOR);
  
  disks_on_poles[0].num_disks = num_disks;
  for (i=0; i<num_disks; i++) {
      diskdlist(DL_DISK+i, 0.3 + i*0.1);
      disks_on_poles[0].disks[num_disks-i-1] = i;
      disk_offset[i][Z] = 0.2*(num_disks-i-1);
  }

  /*
   * start hanoi instruction engine
   */
  {
      int engine_args[2];
      extern void engine(int *);
      int p[2];

      prctl(PR_SETEXITSIG, SIGTERM);
      if (-1 == pipe(p)) {
	  perror("can't pipe");
	  exit(1);
      }
      engine_args[0] = num_disks;
      engine_args[1] = p[1];
      engine_fd = p[0];
      engine_pid = sproc((void(*)(void *))engine, PR_SALL, (void *)engine_args);
      if (engine_pid == -1) {
	  perror("can't sproc");
	  exit(1);
      }
  }

  glutMainLoop();
  /*NOTREACHED*/
  return 0;             /* ANSI C requires main to return int. */
}
