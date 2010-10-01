
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* This demo shows off InfiniteReality's "dynamic video resize" (DVR)
   capability.  Dynamic video resizing let's you maintain a constant frame
   rate even when the view becomes limited by your hardware's pixel fill
   rate.  The idea is simple:  Draw the fill-limited frame into a smaller
   area (ie, touch less pixels) and then have the video hardware "zoom" up
   the image to fill the video screen.  This trick does require special
   hardware.

   As written with no command line options, this demo works well on a 1RM
   InfiniteReality.  It keeps a scene containing colored large orbs that
   rotate around the viewer at a constant 60 frames/sec, even though the
   number of viewable orbs is changing (hence the required fullscreen pixel
   fill rate is changing too).  The demo adapts the rendered image size and
   video resizing based on the last frame to keep a constant frame rate. */

/****************************************************************

Command line arguments:

  -window ... run in a window instead of full screen and will not use
     hardware resizing even if the hardware supports it.  Good for
     demos.

  -novidresize ... start without using video resize hardware even though
     the system might really support video resizing.

  -debug ... output frame size to stdout.

  -target # ... sets the target frame rate in increments of 60th of
      a second; the default is 1.

  -twosnaps ... sample the last frame rate as the average of the last
      two frames; the default is to simply use the last frame.

  -nice ... better tesselate the orbs; nice if you have transformation
      rate to burn (ie, use an InfiniteReality).

  -orbs # ... an initial number of orbs to populate the viewing space;
      the default is 80.

Example command lines:

  Because this program's behavior depends on adapting to the speed of the
  system it is running at, a good demonstration of this program may depend
  on the system performance.  Adjust the command line options accordingly,
  particularly -target and -orbs.

  InfiniteReality (1RM) ... "videoresize"

    (For the hardware listed below, you won't get actual video resizing
     since the hardware lacks the support.)

  Indigo^2 XL 200Mhz ... "videoresize -geometry 800x600 -target 5 -orbs 40 -window"

Key controls:

  ESC or q ... exit the demo.

  n ... disable dynamic video resizing.

  r ... enable dynamic video resizing (the default if hardware video
     resizing is detected; otherwise no video resizing is the
     default). 
     
     NOTE: you can enable dynamic video resizing even without the
     hardware and the demo will show you how the rendered area would
     vary as if the feature were supported.

  h ... toggle use of hardware video resizing (only if in fullscreen
     mode and the hardware supports the feature).

  m ... toggle display of the frame meter.  Enabled by default.

  c ... toggle display of the cursor.  Sometimes it is nice to see the
     cursor when video resizing so that you can see the effect of the
     video resizing since the cursor resizes too, but in general a
     resizing cursor is quite distracting.  The cursor is enabled if
     hardware video resizing is to be used.

  Up arrow ... add 10 more orbs.

  Down arrow ... subtract 10 less orbs.

  Spacebar ... regenerate the set of viewing orbs.

  Right arrow ... increase rightward rotation.

  Left arrow ... increase rightward rotation.

Frame meter:

  The frame meter shows how long each frame takes to render.  It
  operates differently depending on if video resizing is being
  demonstrated or not.

  The meter line is BLUE when video resizing is not being demonstrated.

  When video resizing is being demonstrated, a GREEN meter line
  indicates the frame is being shown at full (non-resized) resolution.
  YELLOW indicates the frame is being video resized.  RED indicates the
  frame is being resized so much that dropping a frame is better than
  the resulting poor resize quality (the demo won't zoom anymore than 8
  to 1).  A smaller MAGENTA line shows a calculation of the
  "unadjusted" frame rate for the displayed scene.  When the MAGENTA
  line is longer than the base line, it is showing a relative measure
  of how much resizing was needed to keep the frame at a sustained
  frame rate.

Bugs:

  This program assumes a frame rate of 60 Hz.

  This program should use InfiniteReality's SGIX_instruments
  extension for better fill rate measurement.

*****************************************************************/

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#if !defined(_WIN32)
#include <unistd.h>
#else
#define lrand48() (0)
#define srand48(x) (0)
#define getpid() (0)
#endif

#define OVERLOAD 0
#define ZOOMED 1
#define UNZOOMED 2
#define STATIC 3
GLfloat state_colors[4][3] =
{
  {1.0, 0.0, 0.0},
  {0.8, 0.8, 0.0},
  {0.0, 0.7, 0.0},
  {0.3, 0.3, 0.8},
};
int state;

extern void sphere(int level);

GLdouble angle, speed = 0.5;

GLfloat light0_ambient[] =
{0.1, 0.1, 0.1, 1.0};
GLfloat light0_diffuse[] =
{0.5, 0.5, 0.5, 1.0};
GLfloat light0_position[] =
{0.0, 0.0, 0.0, 1.0};

typedef struct {
  GLfloat offset;
  GLfloat distance;
  GLfloat size;
  GLfloat *color;
} Orb;

Orb *orbs = NULL;
int num_orbs = 80;      /* Good default for 1 RM InfiniteReality. */
int fullscreen = 1;
int debug = 0;
int video_resizing = 1;
int hw_video_resizing = 1;  /* Assume we have it until told otherwise. */
int hw_exists = 0;
int frame_time = 15;
int sphere_quality = 1;
int max_w, max_h;
GLfloat W, H;
int delta_w_resize = 1, delta_h_resize = 1;
GLfloat target_frame_time = 1000.0 / 60.0;
int show_cursor = 1, show_meter = 1;

#define NUM_SNAPS 2
int snap[NUM_SNAPS] =
{0};
int num_snaps = 1;
int frame = 0;

GLfloat color[][3] =
{
  {0.0, 0.5, 0.0},
  {0.5, 0.5, 0.0},
  {1.0, 0.0, 0.5},
  {0.0, 1.0, 0.5},
  {1.0, 0.0, 0.0},
  {1.0, 0.0, 1.0},
  {0.0, 0.0, 0.8},
  {0.0, 0.2, 0.5},
  {1.0, 1.0, 0.0},
  {1.0, 1.0, 1.0},
  {0.3, 0.3, 0.3},
};
int num_colors = sizeof(color) / (sizeof(GLfloat) * 3);

void
calculate_frame_rate(int delta)
{
  int sum, count, i;

  snap[frame] = delta;
  frame += 1;
  frame %= num_snaps;

  sum = 0;
  count = 0;
  for (i = 0; i < num_snaps; i++) {
    if (snap[i] != 0) {
      sum += snap[i];
      count++;
    }
  }
  frame_time = sum / count;
}

void
advancedAnimation(void)
{
  angle = (GLfloat) fmod(angle + speed, 360.0);
  glutPostRedisplay();
}

void
resize(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50.0, (GLfloat) w / (GLfloat) h, 1.0, 100.0);
  glMatrixMode(GL_MODELVIEW);
}

void
reshape(int w, int h)
{
  max_w = w;
  max_h = h;
  W = w;
  H = h;
  resize(w, h);
}

void
do_video_resize_logic(void)
{
  GLfloat frame_ratio, adjusted_area, dimension_scale;
  GLfloat area;
  int Wint, Hint;

  if (frame_time > (target_frame_time * 0.92)) {
    /* Shrink case. */
    frame_ratio = (target_frame_time * 0.92) / frame_time;
    area = W * H;
    adjusted_area = area * frame_ratio;
    dimension_scale = sqrt(adjusted_area) / sqrt(area);
    W = dimension_scale * W;
    H = dimension_scale * H;
    if (W < max_w / 8 || H < max_h / 8) {
      W = max_w / 8;
      H = max_h / 8;
      state = OVERLOAD;
    } else {
      state = ZOOMED;
    }
    Wint = (((int) W) / delta_w_resize) * delta_w_resize;
    Hint = (((int) H) / delta_h_resize) * delta_h_resize;
    if (video_resizing) {
      if (hw_video_resizing)
        glutVideoResize(0, 0, -Wint, -Hint);
      resize(Wint, Hint);
    }
  } else if (frame_time < (target_frame_time * 0.82)) {
    /* Grow case. */
    frame_ratio = (target_frame_time * 0.82) / frame_time;
    area = W * H;
    adjusted_area = area * frame_ratio;
    dimension_scale = sqrt(adjusted_area) / sqrt(area);

    /* Be a bit conservative about our growth.  As Allan Greenspan would say, 
       "You don't want to overheat the economy; put on the brakes." */
    dimension_scale = (dimension_scale - 1.0) * 0.75 + 1.0;
    if (dimension_scale > 1.5) {
      dimension_scale = 1.5;
    }
    W = dimension_scale * W;
    H = dimension_scale * H;
    if (W > max_w || H > max_h) {
      W = max_w;
      H = max_h;
      state = UNZOOMED;
    } else {
      state = ZOOMED;
    }
    Wint = (((int) W) / delta_w_resize) * delta_w_resize;
    Hint = (((int) H) / delta_h_resize) * delta_h_resize;
    if (video_resizing) {
      if (hw_video_resizing)
        glutVideoResize(0, 0, -Wint, -Hint);
      resize(Wint, Hint);
    }
  } else {
    if (!hw_video_resizing) {
      /* We keep the meter constant size, so we must keep restoring our
         viewport. */
      Wint = (((int) W) / delta_w_resize) * delta_w_resize;
      Hint = (((int) H) / delta_h_resize) * delta_h_resize;
      glViewport(0, 0, Wint, Hint);
    }
  }
  if (debug)
    printf("%gx%g\n\n", W, H);
}

void
display(void)
{
  int i, start, stop;

  if (video_resizing)
    do_video_resize_logic();

  start = glutGet(GLUT_ELAPSED_TIME);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, 0.0, 1.0,
    0.0, 0.0, 0.0,
    0.0, 1.0, 0.0);
  glTranslatef(0.0, 0.0, -1.0);

  for (i = 0; i < num_orbs; i++) {
    glPushMatrix();
    glRotatef(angle + orbs[i].offset, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, orbs[i].distance);
    glScalef(orbs[i].size, orbs[i].size, orbs[i].size);
    glColor3fv(orbs[i].color);
    glCallList(1);
    glPopMatrix();
  }

  if (show_meter) {
    if (!hw_video_resizing) {
      /* Make sure the meter stays constant size in window mode. */
      glViewport(0, 0, max_w, max_h);
    }
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 3000, 0, 3000);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glColor3fv(state_colors[state]);
    glRecti(100, 100, 100 + frame_time * 8, 200);
    if (video_resizing) {
      float render_area = W * H, full_area = max_w * max_h;
      float advantage = sqrt(full_area / render_area);

      glColor3f(1.0, 0.0, 1.0);
      glRecti(100, 140, 100 + frame_time * advantage * 8, 160);
    }
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    for (i = 0; i < 21; i++) {
      glVertex2f(100 + i * 16.6 * 8, 80);
      glVertex2f(100 + i * 16.6 * 8, 220);
    }
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
  }
  glFinish();
  stop = glutGet(GLUT_ELAPSED_TIME);

  glutSwapBuffers();

  calculate_frame_rate(stop - start);
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(advancedAnimation);
  else
    glutIdleFunc(NULL);
}

void
set_cursor(int on)
{
  if (on) {
    glutSetCursor(GLUT_CURSOR_INHERIT);
  } else {
    glutSetCursor(GLUT_CURSOR_NONE);
  }
}

void
generate_orbs(void)
{
  int i, r, angle, chance, delta, d;

  if (orbs)
    free(orbs);
  orbs = (Orb *) malloc(num_orbs * sizeof(Orb));

  i = 0;
  angle = 0;
  while (i < num_orbs) {
    angle += 37;
    angle %= 360;
    r = (int) lrand48();
    chance = abs(r % 360 - 180);
    delta = chance - abs(angle - 180);
    if (delta > 0) {
      orbs[i].color = color[lrand48() % num_colors];
      orbs[i].offset = angle;
      d = (r % 20) + (delta / 180.0) * 10;
      orbs[i].distance = d + 3.0;
      orbs[i].size = 1.4 * exp(d / 8.0) / exp(1.0);
      i++;
    }
  }
}

/* ARGSUSED */
void
keyboard(unsigned char k, int x, int y)
{
  switch (k) {
  case 27:
  case 'q':
  case 'Q':
    exit(0);
    break;
  case 'R':
  case 'r':
    video_resizing = 1;
    break;
  case 'H':
  case 'h':
    if (hw_exists) {
      hw_video_resizing = 1 - hw_video_resizing;
      if (!hw_video_resizing) {
        glutVideoResize(0, 0, max_w, max_h);
        resize(max_w, max_h);
      }
    }
    break;
  case 'N':
  case 'n':
    video_resizing = 0;
    if (hw_video_resizing)
      glutVideoResize(0, 0, max_w, max_h);
    resize(max_w, max_h);
    state = STATIC;
    break;
  case 'D':
  case 'd':
    debug = 1;
    break;
  case 'M':
  case 'm':
    show_meter = 1 - show_meter;
    break;
  case ' ':
    generate_orbs();
    break;
  case 'C':
  case 'c':
    show_cursor = 1 - show_cursor;
    set_cursor(show_cursor);
    break;
  }
}

/* ARGSUSED */
void
special(int k, int x, int y)
{
  switch (k) {
  case GLUT_KEY_RIGHT:
    speed -= 0.25;
    break;
  case GLUT_KEY_LEFT:
    speed += 0.25;
    break;
  case GLUT_KEY_UP:
    num_orbs += 10;
    generate_orbs();
    break;
  case GLUT_KEY_DOWN:
    num_orbs -= 10;
    generate_orbs();
    break;
  }
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-orbs")) {
      i++;
      if (i >= argc) {
        fprintf(stderr, "videoresize: need number of orbs\n");
        exit(1);
      }
      num_orbs = atoi(argv[i]);
      if (num_orbs < 1) {
        fprintf(stderr, "videoresize: number of orbs must be 1 or more\n");
        exit(1);
      }
    } else if (!strcmp(argv[i], "-target")) {
      i++;
      if (i >= argc) {
        fprintf(stderr, "videoresize: need target number of frames\n");
        exit(1);
      }
      target_frame_time = atoi(argv[i]) * 1000.0 / 60.0;
      if (target_frame_time <= 0.0) {
        fprintf(stderr, "videoresize: target frames must be 1 or more\n");
        exit(1);
      }
    } else if (!strcmp(argv[i], "-novidresize")) {
      hw_video_resizing = 0;
    } else if (!strcmp(argv[i], "-debug")) {
      debug = 1;
    } else if (!strcmp(argv[i], "-window")) {
      fullscreen = 0;
    } else if (!strcmp(argv[i], "-twosnaps")) {
      num_snaps = 2;
    } else if (!strcmp(argv[i], "-nice")) {
      sphere_quality = 2;
    } else {
      printf("usage: videoresize [-window] [-twosnaps] [-nice] [-target #] [-orbs #]\n");
      printf("  -target #    = target number of frame intervals to stay under (default=%g)\n", target_frame_time);
      printf("  -orbs #      = number of orbs to randomly position (default=%d)\n", num_orbs);
      printf("  -window      = do not go fullscreen\n");
      printf("  -nice        = use better tesselated spheres\n");
      printf("  -novidresize = even if videoresizing is supported, don't use it\n");
      printf("  -twosnaps    = averages over last two frames instead of just estimating\n");
      printf("                 based on the last frame\n");
      exit(1);
    }
  }
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  glutCreateWindow("Dynamic video resizing for constant frame rates");
  if (fullscreen) {
    glutFullScreen();
    if (glutVideoResizeGet(GLUT_VIDEO_RESIZE_POSSIBLE)) {
      delta_w_resize = glutVideoResizeGet(GLUT_VIDEO_RESIZE_WIDTH_DELTA);
      delta_h_resize = glutVideoResizeGet(GLUT_VIDEO_RESIZE_HEIGHT_DELTA);
      show_cursor = 0;
      glutSetupVideoResizing();
      hw_exists = 1;
    } else {
      hw_video_resizing = 0;
    }
  } else {
    hw_video_resizing = 0;
  }
  if (video_resizing) {
    state = UNZOOMED;
  } else {
    state = STATIC;
  }
  glutReshapeFunc(reshape);
  glDisable(GL_CULL_FACE);  /* Makes us more fill limited. */
  glEnable(GL_LIGHT0);
  glColorMaterial(GL_FRONT, GL_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_NORMALIZE);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

  glNewList(1, GL_COMPILE);
  sphere(sphere_quality);
  glEndList();

  srand48(getpid() * 16 + glutGet(GLUT_ELAPSED_TIME));
  generate_orbs();

  glutDisplayFunc(display);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);
  set_cursor(show_cursor);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
