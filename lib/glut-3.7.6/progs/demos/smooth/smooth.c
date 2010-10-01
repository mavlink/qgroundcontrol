/*  
    smooth.c
    Nate Robins, 1998

    Model viewer program.  Exercises the glm library.
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <GL/glut.h>
#include "gltb.h"
#include "glm.h"
#include "dirent32.h"

#define DATA_DIR "data/"

char*      model_file = NULL;		/* name of the obect file */
GLuint     model_list = 0;		/* display list for object */
GLMmodel*  model;			/* glm model data structure */
GLfloat    scale;			/* original scale factor */
GLfloat    smoothing_angle = 90.0;	/* smoothing angle */
GLfloat    weld_distance = 0.00001;	/* epsilon for welding vertices */
GLboolean  facet_normal = GL_FALSE;	/* draw with facet normal? */
GLboolean  bounding_box = GL_FALSE;	/* bounding box on? */
GLboolean  performance = GL_FALSE;	/* performance counter on? */
GLboolean  stats = GL_FALSE;		/* statistics on? */
GLuint     material_mode = 0;		/* 0=none, 1=color, 2=material */
GLint      entries = 0;			/* entries in model menu */
GLdouble   pan_x = 0.0;
GLdouble   pan_y = 0.0;
GLdouble   pan_z = 0.0;

#if defined(_WIN32)
#include <sys/timeb.h>
#else
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#endif
#ifndef CLK_TCK
#define CLK_TCK 1000
#endif
float
elapsed(void)
{
    static long begin = 0;
    static long finish, difference;

#if defined(_WIN32)
    static struct timeb tb;
    ftime(&tb);
    finish = tb.time*1000+tb.millitm;
#else
    static struct tms tb;
    finish = times(&tb);
#endif

    difference = finish - begin;
    begin = finish;

    return (float)difference/(float)CLK_TCK;
}

void
shadowtext(int x, int y, char* s) 
{
    int lines;
    char* p;

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 
	    0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3ub(0, 0, 0);
    glRasterPos2i(x+1, y-1);
    for(p = s, lines = 0; *p; p++) {
	if (*p == '\n') {
	    lines++;
	    glRasterPos2i(x+1, y-1-(lines*18));
	}
	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glColor3ub(0, 128, 255);
    glRasterPos2i(x, y);
    for(p = s, lines = 0; *p; p++) {
	if (*p == '\n') {
	    lines++;
	    glRasterPos2i(x, y-(lines*18));
	}
	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

void
lists(void)
{
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
    GLfloat specular[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat shininess = 65.0;

    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);

    if (model_list)
	glDeleteLists(model_list, 1);

    /* generate a list */
    if (material_mode == 0) { 
	if (facet_normal)
	    model_list = glmList(model, GLM_FLAT);
	else
	    model_list = glmList(model, GLM_SMOOTH);
    } else if (material_mode == 1) {
	if (facet_normal)
	    model_list = glmList(model, GLM_FLAT | GLM_COLOR);
	else
	    model_list = glmList(model, GLM_SMOOTH | GLM_COLOR);
    } else if (material_mode == 2) {
	if (facet_normal)
	    model_list = glmList(model, GLM_FLAT | GLM_MATERIAL);
	else
	    model_list = glmList(model, GLM_SMOOTH | GLM_MATERIAL);
    }
}

void
init(void)
{
    gltbInit(GLUT_LEFT_BUTTON);
  
    /* read in the model */
    model = glmReadOBJ(model_file);
    scale = glmUnitize(model);
    glmFacetNormals(model);
    glmVertexNormals(model, smoothing_angle);

    if (model->nummaterials > 0)
	material_mode = 2;

    /* create new display lists */
    lists();

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
}

void
reshape(int width, int height)
{
    gltbReshape(width, height);

    glViewport(0, 0, width, height);
  
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -3.0);
}

#define NUM_FRAMES 5
void
display(void)
{
    static char s[256], t[32];
    static char* p;
    static int frames = 0;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    glTranslatef(pan_x, pan_y, 0.0);

    gltbMatrix();

#if 0   /* glmDraw() performance test */
    if (material_mode == 0) { 
	if (facet_normal)
	    glmDraw(model, GLM_FLAT);
	else
	    glmDraw(model, GLM_SMOOTH);
    } else if (material_mode == 1) {
	if (facet_normal)
	    glmDraw(model, GLM_FLAT | GLM_COLOR);
	else
	    glmDraw(model, GLM_SMOOTH | GLM_COLOR);
    } else if (material_mode == 2) {
	if (facet_normal)
	    glmDraw(model, GLM_FLAT | GLM_MATERIAL);
	else
	    glmDraw(model, GLM_SMOOTH | GLM_MATERIAL);
    }
#else
    glCallList(model_list);
#endif

    glDisable(GL_LIGHTING);
    if (bounding_box) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glColor4f(1.0, 0.0, 0.0, 0.25);
	glutSolidCube(2.0);
	glDisable(GL_BLEND);
    }

    glPopMatrix();

    if (stats) {
	/* XXX - this could be done a _whole lot_ faster... */
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	glColor3ub(0, 0, 0);
	sprintf(s, "%s\n%d vertices\n%d triangles\n%d normals\n"
		"%d texcoords\n%d groups\n%d materials",
	        model->pathname, model->numvertices, model->numtriangles, 
		model->numnormals, model->numtexcoords, model->numgroups,
		model->nummaterials);
	shadowtext(5, height-(5+18*1), s);
    }

    /* spit out frame rate. */
    frames++;
    if (frames > NUM_FRAMES) {
	sprintf(t, "%g fps", frames/elapsed());
	frames = 0;
    }
    if (performance) {
	shadowtext(5, 5, t);
    }

    glutSwapBuffers();
    glEnable(GL_LIGHTING);
}

void
keyboard(unsigned char key, int x, int y)
{
    GLint params[2];

    switch (key) {
    case 'h':
	printf("help\n\n");
	printf("w         -  Toggle wireframe/filled\n");
	printf("c         -  Toggle culling\n");
	printf("n         -  Toggle facet/smooth normal\n");
	printf("b         -  Toggle bounding box\n");
	printf("r         -  Reverse polygon winding\n");
	printf("m         -  Toggle color/material/none mode\n");
	printf("p         -  Toggle performance indicator\n");
	printf("s/S       -  Scale model smaller/larger\n");
	printf("t         -  Show model stats\n");
	printf("o         -  Weld vertices in model\n");
	printf("+/-       -  Increase/decrease smoothing angle\n");
	printf("W         -  Write model to file (out.obj)\n");
	printf("q/escape  -  Quit\n\n");
	break;

    case 't':
	stats = !stats;
	break;

    case 'p':
	performance = !performance;
	break;

    case 'm':
	material_mode++;
	if (material_mode > 2)
	    material_mode = 0;
	printf("material_mode = %d\n", material_mode);
	lists();
	break;

    case 'd':
	glmDelete(model);
	init();
	lists();
	break;

    case 'w':
	glGetIntegerv(GL_POLYGON_MODE, params);
	if (params[0] == GL_FILL)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;

    case 'c':
	if (glIsEnabled(GL_CULL_FACE))
	    glDisable(GL_CULL_FACE);
	else
	    glEnable(GL_CULL_FACE);
	break;

    case 'b':
	bounding_box = !bounding_box;
	break;

    case 'n':
	facet_normal = !facet_normal;
	lists();
	break;

    case 'r':
	glmReverseWinding(model);
	lists();
	break;

    case 's':
	glmScale(model, 0.8);
	lists();
	break;

    case 'S':
	glmScale(model, 1.25);
	lists();
	break;

    case 'o':
	printf("Welded %d\n", glmWeld(model, weld_distance));
	glmVertexNormals(model, smoothing_angle);
	lists();
	break;

    case 'O':
	weld_distance += 0.01;
	printf("Weld distance: %.2f\n", weld_distance);
	glmWeld(model, weld_distance);
	glmFacetNormals(model);
	glmVertexNormals(model, smoothing_angle);
	lists();
	break;

    case '-':
	smoothing_angle -= 1.0;
	printf("Smoothing angle: %.1f\n", smoothing_angle);
	glmVertexNormals(model, smoothing_angle);
	lists();
	break;
    
    case '+':
	smoothing_angle += 1.0;
	printf("Smoothing angle: %.1f\n", smoothing_angle);
	glmVertexNormals(model, smoothing_angle);
	lists();
	break;
    
    case 'W':
	glmScale(model, 1.0/scale);
	glmWriteOBJ(model, "out.obj", GLM_SMOOTH | GLM_MATERIAL);
	break;

    case 'R':
	{
	    GLuint i;
	    GLfloat swap;
	    for (i = 1; i <= model->numvertices; i++) {
		swap = model->vertices[3 * i + 1];
		model->vertices[3 * i + 1] = model->vertices[3 * i + 2];
		model->vertices[3 * i + 2] = -swap;
	    }
	    glmFacetNormals(model);
	    lists();
	    break;
	}

    case 27:
	exit(0);
	break;
    }

    glutPostRedisplay();
}

void
menu(int item)
{
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;

    if (item > 0) {
	keyboard((unsigned char)item, 0, 0);
    } else {
	dirp = opendir(DATA_DIR);
	while ((direntp = readdir(dirp)) != NULL) {
	    if (strstr(direntp->d_name, ".obj")) {
		i++;
		if (i == -item)
		    break;
	    }
	}
	if (!direntp)
	    return;
	name = (char*)malloc(strlen(direntp->d_name) + strlen(DATA_DIR) + 1);
	strcpy(name, DATA_DIR);
	strcat(name, direntp->d_name);
	model = glmReadOBJ(name);
	scale = glmUnitize(model);
	glmFacetNormals(model);
	glmVertexNormals(model, smoothing_angle);
	
	if (model->nummaterials > 0)
	    material_mode = 2;
	else
	    material_mode = 0;

	lists();
	free(name);

	glutPostRedisplay();
    }
}

static GLint      mouse_state;
static GLint      mouse_button;

void
mouse(int button, int state, int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];

    /* fix for two-button mice -- left mouse + shift = middle mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    gltbMouse(button, state, x, y);

    mouse_state = state;
    mouse_button = button;

    if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON) {
	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);
	gluProject((GLdouble)x, (GLdouble)y, 0.0,
		   model, proj, view,
		   &pan_x, &pan_y, &pan_z);
	gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
		     model, proj, view,
		     &pan_x, &pan_y, &pan_z);
	pan_y = -pan_y;
    }

    glutPostRedisplay();
}

void
motion(int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];

    gltbMotion(x, y);

    if (mouse_state == GLUT_DOWN && mouse_button == GLUT_MIDDLE_BUTTON) {
	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);
	gluProject((GLdouble)x, (GLdouble)y, 0.0,
		   model, proj, view,
		   &pan_x, &pan_y, &pan_z);
	gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
		     model, proj, view,
		     &pan_x, &pan_y, &pan_z);
	pan_y = -pan_y;
    }

    glutPostRedisplay();
}

int
main(int argc, char** argv)
{
    int buffering = GLUT_DOUBLE;
    struct dirent* direntp;
    DIR* dirp;
    int models;

    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);

    while (--argc) {
	if (strcmp(argv[argc], "-sb") == 0)
	    buffering = GLUT_SINGLE;
	else
	    model_file = argv[argc];
    }

    if (!model_file) {
	model_file = "data/dolphins.obj";
    }

    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | buffering);
    glutCreateWindow("Smooth");
  
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    models = glutCreateMenu(menu);
    dirp = opendir(DATA_DIR);
    if (!dirp) {
	fprintf(stderr, "%s: can't open data directory.\n", argv[0]);
    } else {
	while ((direntp = readdir(dirp)) != NULL) {
	    if (strstr(direntp->d_name, ".obj")) {
		entries++;
		glutAddMenuEntry(direntp->d_name, -entries);
	    }
	}
	closedir(dirp);
    }

    glutCreateMenu(menu);
    glutAddMenuEntry("Smooth", 0);
    glutAddMenuEntry("", 0);
    glutAddSubMenu("Models", models);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[w]   Toggle wireframe/filled", 'w');
    glutAddMenuEntry("[c]   Toggle culling on/off", 'c');
    glutAddMenuEntry("[n]   Toggle face/smooth normals", 'n');
    glutAddMenuEntry("[b]   Toggle bounding box on/off", 'b');
    glutAddMenuEntry("[p]   Toggle frame rate on/off", 'p');
    glutAddMenuEntry("[t]   Toggle model statistics", 't');
    glutAddMenuEntry("[m]   Toggle color/material/none mode", 'm');
    glutAddMenuEntry("[r]   Reverse polygon winding", 'r');
    glutAddMenuEntry("[s]   Scale model smaller", 's');
    glutAddMenuEntry("[S]   Scale model larger", 'S');
    glutAddMenuEntry("[o]   Weld redundant vertices", 'o');
    glutAddMenuEntry("[+]   Increase smoothing angle", '+');
    glutAddMenuEntry("[-]   Decrease smoothing angle", '-');
    glutAddMenuEntry("[W]   Write model to file (out.obj)", 'W');
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
  
    init();
  
    glutMainLoop();
    return 0;
}
