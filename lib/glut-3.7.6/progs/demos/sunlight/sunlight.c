/* sunlight.c
 * 
 * To compile:
 *	cc -o sunlight sunlight.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm
 *
 * Usage: sunlight
 *
 * Requires: globe.raw
 *
 * This program demonstrates the sun lighting the earth, displayed
 * either as a globe or a flat map.  The time of day and day of year
 * can be adjusted interactively, and the globe can be viewed from
 * any angle.
 *
 * The lighting is done using OpenGL's lighting.  The flat map is
 * lighted by using the normals of globe.  This is a rather unique
 * use of normals, making a flat surface lit as if it were round.
 *
 * The left mouse adjusts the globe orientation, the time of day, or
 * the time of year.  The right mouse activates a menu.
 *
 * Chris Schoeneman - 9/6/98
 * crs23@bigfoot.com
 * http://www.bigfoot.com/~crs23/
 *        
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* menu constants */
#define MENU_SHOW_GLOBE		1
#define MENU_SHOW_MAP		2
#define MENU_ADJUST_GLOBE	3
#define MENU_ADJUST_DAY		4
#define MENU_ADJUST_TIME	5
#define MENU_QUIT		6

static int		mapMode = 0;
static int		adjustMode = 0;
static GLdouble		aspectRatio = 1.0;
static double		timeOfDay = 0.0;
static double		timeOfYear = 0.0;

/*
 * sun direction functions
 */

static const double	radPerDeg = M_PI / 180.0;
static const double	radPerHour = M_PI / 12.0;
static const double	siderealHoursPerHour = 1.002737908;
static const double	epoch = 2415020.0;

static double		getGreenwichSideral(double julianDay)
{
  double jdMidnight;
  double dayFraction;
  double T;		/* time (fractions of a century) */
  double greenwichMidnight;

  /* true position requires sidereal time of midnight at prime meridian.
     get midnight of given julian day (midnight has decimal of .5). */
  jdMidnight = floor(julianDay);
  if (julianDay - jdMidnight >= 0.5) jdMidnight += 0.5;
  else jdMidnight -= 0.5;

  T = (jdMidnight - epoch) / 36525.0;

  /* get fraction of a day */
  dayFraction = (julianDay - jdMidnight) * 24.0;

  /* get Greenwich midnight sidereal time (in hours) */
  greenwichMidnight =
		fmod((0.00002581 * T + 2400.051262) * T + 6.6460656, 24.0);

  /* return Greenwich sidereal time */
  return radPerHour * (greenwichMidnight + dayFraction * siderealHoursPerHour);
}

static void		getTruePosition(double julianDay,
					float latitude, float longitude,
					double sx, double sy, double sz,
					float pos[3])
{
  double tx, ty, tz;
  double localSidereal;

  /* convert to radians */
  latitude *= radPerDeg;
  longitude *= radPerDeg;

  /* get local sidereal time */
  localSidereal = getGreenwichSideral(julianDay) - longitude;

  /* rotate around polar axis (y-axis) by local sidereal time */
  tx = sx * cos(localSidereal) - sz * sin(localSidereal);
  ty = sy;
  tz = sz * cos(localSidereal) + sx * sin(localSidereal);

  /* rotate by latitude to local position */
  pos[0] = (float)tx;
  pos[1] = (float)(ty * cos(latitude) - tz * sin(latitude));
  pos[2] = (float)(tz * cos(latitude) + ty * sin(latitude));
}

static void		getSunDirection(double julianDay, float latitude,
					float longitude, float pos[3])
{
  double C;
  double T;		/* time (fractions of a century) */
  double sx, sy, sz;	/* sun direction */
  double obliquity;	/* earth's tilt */
  double meanAnomaly;
  double geometricMeanLongitude;
  double trueLongitude;

  T = (julianDay - epoch) / 36525.0;

  obliquity = radPerDeg *
		(23.452294 + (-0.0130125 + (-0.00000164 +
						0.000000503 * T) * T) * T);

  meanAnomaly = radPerDeg * (358.47583 +
		((0.0000033 * T + 0.000150) * T + 35999.04975) * T);

  geometricMeanLongitude = radPerDeg *
		((0.0003025 * T + 36000.76892) * T + 279.69668);

  C = radPerDeg *
		(sin(meanAnomaly) * (1.919460 - (0.004789 + 0.000014 * T) * T) +
		sin(2.0 * meanAnomaly) * (0.020094 - 0.0001 * T) +
		sin(3.0 * meanAnomaly) * 0.000293);

  trueLongitude = geometricMeanLongitude + C;

  /* position of sun if earth didn't rotate: */
  sx = sin(trueLongitude) * cos(obliquity);
  sy = sin(trueLongitude) * sin(obliquity);
  sz = cos(trueLongitude);

  /* get true position */
  getTruePosition(julianDay, latitude, longitude, sx, sy, sz, pos);
}

/*
 * globe/map calculation
 */

#define LAT_GRID	20
#define LON_GRID	40
static GLfloat		sphere[LAT_GRID + 1][LON_GRID + 1][3];
static GLfloat		map[LAT_GRID + 1][LON_GRID + 1][2];
static GLfloat		uv[LAT_GRID + 1][LON_GRID + 1][2];

static void		initSphere(void)
{
  int lat, lon;

  for (lat = 0; lat <= LAT_GRID; lat++) {
    const float y = (float)-cos(M_PI * (double)lat / (double)LAT_GRID);
    const float r = (float)sin(M_PI * (double)lat / (double)LAT_GRID);
    for (lon = 0; lon <= LON_GRID; lon++) {
      sphere[lat][lon][0] = r * (float)sin(-M_PI + 2.0 * M_PI *
					(double)lon / (double)LON_GRID);
      sphere[lat][lon][1] = y;
      sphere[lat][lon][2] = r * (float)cos(-M_PI + 2.0 * M_PI *
					(double)lon / (double)LON_GRID);
    }
  }
}

static void		initMap(void)
{
  int lat, lon;

  for (lat = 0; lat <= LAT_GRID; lat++) {
    const float y = 1.0f * ((float)lat / (float)LAT_GRID - 0.5f);
    for (lon = 0; lon <= LON_GRID; lon++) {
      map[lat][lon][0] = 2.0f * ((float)lon / (float)LON_GRID - 0.5f);
      map[lat][lon][1] = y;
    }
  }
}

static int		isPowerOfTwo(int x)
{
  if (x <= 0) return 0;
  while ((x & 1) == 0) x >>= 1;
  return x == 1;
}

static void		initTexture(const char* filename)
{
  FILE* imageFile;
  int lat, lon;
  unsigned char buffer[4];
  unsigned char* image;
  int dx, dy;

  /* initialize texture coordinates */
  for (lat = 0; lat <= LAT_GRID; lat++) {
    const float y = (float)lat / (float)LAT_GRID;
    for (lon = 0; lon <= LON_GRID; lon++) {
      uv[lat][lon][0] = (float)lon / (float)LON_GRID;
      uv[lat][lon][1] = y;
    }
  }

  /* open image file */
  imageFile = fopen(filename, "rb");
  if (!imageFile) {
    fprintf(stderr, "cannot open image file %s for reading\n", filename);
    exit(1);
  }

  /* read image size */
  fread(buffer, 1, 4, imageFile);
  dx = (int)((buffer[0] << 8) | buffer[1]);
  dy = (int)((buffer[2] << 8) | buffer[3]);
  if (!isPowerOfTwo(dx) || !isPowerOfTwo(dy)) {
    fprintf(stderr, "image %s has an illegal size: %dx%d\n", filename, dx, dy);
    exit(1);
  }

  /* read image */
  image = (unsigned char*)malloc(3 * dx * dy * sizeof(unsigned char));
  fread(image, 3 * dx, dy, imageFile);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, dx, dy, 0,
		GL_RGB, GL_UNSIGNED_BYTE, image);

  /* tidy up */
  free(image);
  fclose(imageFile);
}

/*
 * misc OpenGL stuff
 */

static const GLfloat	sunColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat		sunDirection[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
static const GLfloat	surfaceColor[] = { 0.8f, 0.8f, 0.8f, 1.0f };

static void		initProjection(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  if (mapMode) {
    /* orthographic for map */
    if (aspectRatio < 2.0)
      glOrtho(-1.0, 1.0, -1.0 / aspectRatio, 1.0 / aspectRatio, 2.0, 4.0);
    else
      glOrtho(-0.5 * aspectRatio, 0.5 * aspectRatio, -0.5, 0.5, 2.0, 4.0);
  }

  else {
    /* prespective for globe */
    gluPerspective(40.0, aspectRatio, 1.0, 5.0);
  }

  glMatrixMode(GL_MODELVIEW);
}

static void		initSunlight(void)
{
  getSunDirection(epoch + timeOfDay + 365.0 * timeOfYear,
					0.0f, 0.0f, sunDirection);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, sunColor);
  glLightfv(GL_LIGHT0, GL_POSITION, sunDirection);
  glEnable(GL_LIGHT0);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, surfaceColor);
}

/*
 * glut callbacks
 */

static GLfloat		angle = 0;	/* in degrees */
static GLfloat		angle2 = 0;	/* in degrees */
static int		moving = 0, startx, starty;

static void		reshapeCB(GLsizei w, GLsizei h)
{
  aspectRatio = (GLdouble)w / (GLdouble)h;
  glViewport(0, 0, w, h);
  initProjection();
  glutPostRedisplay();
}

static void		redrawCB(void)
{
  glClear(GL_COLOR_BUFFER_BIT);

  glPushMatrix();

    /* Perform scene rotations based on user mouse input. */
    if (!mapMode) {
      glRotatef(angle2, 1.0f, 0.0f, 0.0f);
      glRotatef(angle, 0.0f, 1.0f, 0.0f);
    }

    glLightfv(GL_LIGHT0, GL_POSITION, sunDirection);

    if (mapMode) {
      /* use the normals for a sphere on a flat surface to get
       * the lighting as if we unwrapped a lighted sphere. */

      int lat, lon;
      for (lat = 0; lat < LAT_GRID; lat++) {
	glBegin(GL_TRIANGLE_STRIP);

	  for (lon = 0; lon <= LON_GRID; lon++) {
	    glTexCoord2fv(uv[lat + 1][lon]);
	    glNormal3fv(sphere[lat + 1][lon]);
	    glVertex2fv(map[lat + 1][lon]);

	    glTexCoord2fv(uv[lat][lon]);
	    glNormal3fv(sphere[lat][lon]);
	    glVertex2fv(map[lat][lon]);
	  }

	glEnd();
      }
    }

    else {
      /* draw a sphere */
      int lat, lon;
      for (lat = 0; lat < LAT_GRID; lat++) {
	glBegin(GL_TRIANGLE_STRIP);

	  for (lon = 0; lon <= LON_GRID; lon++) {
	    glTexCoord2fv(uv[lat + 1][lon]);
	    glNormal3fv(sphere[lat + 1][lon]);
	    glVertex3fv(sphere[lat + 1][lon]);

	    glTexCoord2fv(uv[lat][lon]);
	    glNormal3fv(sphere[lat][lon]);
	    glVertex3fv(sphere[lat][lon]);
	  }

	glEnd();
      }
    }

  glPopMatrix();

  glutSwapBuffers();
}

static void		mouseCB(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      moving = 1;
      startx = x;
      starty = y;
    }
    else if (state == GLUT_UP) {
      moving = 0;
    }
  }
}

static void		motionCB(int x, int y)
{
  if (moving) {
    switch (adjustMode) {
      case 0:
	/* move globe */
	if (!mapMode) {
	  angle = angle + (x - startx);
	  angle2 = angle2 + (y - starty);
	  glutPostRedisplay();
	}
	break;

      case 1:
	/* change day of year */
	timeOfYear = timeOfYear + (y - starty) / 365.0;
	while (timeOfYear < 0) timeOfYear += 1.0;
	while (timeOfYear >= 1.0) timeOfYear -= 1.0;
	getSunDirection(epoch + timeOfDay + 365.0 * timeOfYear,
					0.0f, 0.0f, sunDirection);
	glutPostRedisplay();
	break;

      case 2:
	/* change time of day (by 4 minute increments (24 hrs / 360)) */
	timeOfDay = timeOfDay + (y - starty) / 360.0;
	while (timeOfDay < 0) timeOfDay += 1.0;
	while (timeOfDay >= 1.0) timeOfDay -= 1.0;
	getSunDirection(epoch + timeOfDay + 365.0 * timeOfYear,
					0.0f, 0.0f, sunDirection);
	glutPostRedisplay();
	break;
    }

    startx = x;
    starty = y;
  }
}

static void		keyCB(unsigned char c, int x, int y)
{
  if (c == 27) {
    exit(0);
  }
  glutPostRedisplay();
}


/*
 * menu handling
 */

static void		handleMenu(int value)
{
  switch (value) {
    default:
      break;

    case MENU_SHOW_GLOBE:
      mapMode = 0;
      initProjection();
      glutPostRedisplay();
      glutChangeToMenuEntry(2, "Show Globe *", MENU_SHOW_GLOBE);
      glutChangeToMenuEntry(3, "Show Map", MENU_SHOW_MAP);
      break;

    case MENU_SHOW_MAP:
      mapMode = 1;
      initProjection();
      glutPostRedisplay();
      glutChangeToMenuEntry(2, "Show Globe", MENU_SHOW_GLOBE);
      glutChangeToMenuEntry(3, "Show Map *", MENU_SHOW_MAP);
      break;

    case MENU_ADJUST_GLOBE:
      adjustMode = 0;
      glutChangeToMenuEntry(4, "Adjust Globe *", MENU_ADJUST_GLOBE);
      glutChangeToMenuEntry(5, "Adjust Day", MENU_ADJUST_DAY);
      glutChangeToMenuEntry(6, "Adjust Time", MENU_ADJUST_TIME);
      break;

    case MENU_ADJUST_DAY:
      adjustMode = 1;
      glutChangeToMenuEntry(4, "Adjust Globe", MENU_ADJUST_GLOBE);
      glutChangeToMenuEntry(5, "Adjust Day *", MENU_ADJUST_DAY);
      glutChangeToMenuEntry(6, "Adjust Time", MENU_ADJUST_TIME);
      break;

    case MENU_ADJUST_TIME:
      adjustMode = 2;
      glutChangeToMenuEntry(4, "Adjust Globe", MENU_ADJUST_GLOBE);
      glutChangeToMenuEntry(5, "Adjust Day", MENU_ADJUST_DAY);
      glutChangeToMenuEntry(6, "Adjust Time *", MENU_ADJUST_TIME);
      break;

    case MENU_QUIT:
      exit(0);
  }
}

/*
 * main
 */

int			main(int argc, char** argv)
{
  /*
   * initialize GLUT and open a window
   */
  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutCreateWindow("Sunlight");
  glutDisplayFunc(redrawCB);
  glutReshapeFunc(reshapeCB);
  glutMouseFunc(mouseCB);
  glutMotionFunc(motionCB);
  glutKeyboardFunc(keyCB);

  /*
   * make the menu
   */
  glutCreateMenu(handleMenu);
  glutAddMenuEntry("SUNLIGHT", 0);
  glutAddMenuEntry("Show Globe *", MENU_SHOW_GLOBE);
  glutAddMenuEntry("Show Map", MENU_SHOW_MAP);
  glutAddMenuEntry("Adjust Globe *", MENU_ADJUST_GLOBE);
  glutAddMenuEntry("Adjust Day", MENU_ADJUST_DAY);
  glutAddMenuEntry("Adjust Time", MENU_ADJUST_TIME);
  glutAddMenuEntry("Quit", MENU_QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /*
   * initialize GL
   */
  initProjection();
  gluLookAt(0.0, 0.0, 3.0,
	    0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0);
  initSunlight();
  glEnable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);

  /*
   * initialize data structures
   */
  initSphere();
  initMap();
  initTexture("globe.raw");

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
