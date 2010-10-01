#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

int winWidth = 512;
int winHeight = 512;
GLboolean smooth = GL_FALSE;
GLboolean dblbuf = GL_TRUE;
GLfloat objangle[2] = {0.f, 0.f};
GLfloat scale = 1.f;
int active;

enum {X, Y, Z};
enum {OBJ_ANGLE, OBJ_SCALE};
enum {NOLIST, PLANE}; /* display lists */

/* load data structure from file */
enum {VERTS, END};


void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
}

void
motion(int x, int y)
{

    switch(active)
    {
    case OBJ_ANGLE:
	objangle[X] = (x - winWidth/2) * 360./winWidth;
	objangle[Y] = (y - winHeight/2) * 360./winHeight;
	glutPostRedisplay();
	break;
    case OBJ_SCALE:
	scale = x * 5./winWidth;
	objangle[Y] = (y - winHeight/2) * 360./winHeight;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* rotate the object */
	    active = OBJ_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* scale the object */
	    active = OBJ_SCALE;
	    motion(x, y);
	    break;
	}
}


/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 's': /* toggle line smoothing */
	if(smooth)
	{
	    glDisable(GL_LINE_SMOOTH);
	    glDisable(GL_BLEND);
	    smooth = GL_FALSE;
	    printf("Turn off OpenGL line smoothing\n");
	}
	else
	{
	    glEnable(GL_LINE_SMOOTH);
	    glEnable(GL_BLEND);
	    printf("Turn on OpenGL line smoothing\n");
	    smooth = GL_TRUE;
	}
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
	break;
   case '?':
   case 'h':
   case 'H':
    default:
	fprintf(stderr, "Keyboard commands:\n\n"
		"s - toggle smooth line mode\n");
	break;
    }

}


void
loader(char *fname)
{
    FILE *fp;
    GLfloat x, y, z;
    int state = END;
    int read;

    fp = fopen(fname, "r");
    if (!fp) {
        printf("can't open file %s\n", fname);
	exit(1);
    }

    glNewList(PLANE, GL_COMPILE);
    while(!feof(fp))
    {
	switch(state)
	{
	case END:
	    read = fscanf(fp, " v");
	    if(read < 0) /* hit eof */
		break;
	    state = VERTS;
	    glBegin(GL_LINE_STRIP);
	    break;
	case VERTS:
	    read = fscanf(fp, " %f %f %f", &x, &y, &z);
	    if(read == 3)
		glVertex3f(x, y, z);
	    else
	    {
		fscanf(fp, " e");
		glEnd();
		state = END;
	    }
	    break;
	}
    }
    glEndList();
    fclose(fp);
}

void redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    glScalef(scale, scale, scale);

    glCallList(PLANE);

    glPopMatrix();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done)
	{
	    switch(*args)
	    {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = GL_FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = GL_TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA);

    (void)glutCreateWindow("load and draw a wireframe image");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    glutMotionFunc(motion);
    glutMouseFunc(mouse);

    glMatrixMode(GL_PROJECTION);
    glOrtho(-1., 1., -1., 1., -5., 5.);
    glMatrixMode(GL_MODELVIEW);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    loader("../data/f15.data");

    CHECK_ERROR("OpenGL Error in main()");

    key('?', 0, 0); /* print usage message */
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
