#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture	glBindTextureEXT
#endif

static int the_texture;
static int texture_count;
static int shrink = 1;

void afunc(void) {
    static int state;
    if (state ^= 1)
	glEnable(GL_ALPHA_TEST);
    else
	glDisable(GL_ALPHA_TEST);
}

void bfunc(void) {
    static int state;
    if (state ^= 1)
	glEnable(GL_BLEND);
    else
	glDisable(GL_BLEND);
}

void sfunc(void) {
    shrink ^= 1;
}

void tfunc(void) {
    static int state;
    if (state ^= 1)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    else
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void fourfunc(void) {
    static int state;
    GLenum wrap;
    int i;

    glMatrixMode(GL_TEXTURE);
    if (state ^= 1) {
	wrap = GL_REPEAT;
	glScalef(4.f, 4.f, 1.f);
    } else {
	wrap = GL_CLAMP;
	glLoadIdentity();
    }
    glMatrixMode(GL_MODELVIEW);

    for(i = 0; i < texture_count; i++) {
	glBindTexture(GL_TEXTURE_2D, i+1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    }
}

void help(void) {
    printf("Usage: texmovie image0 ... imagen\n");
    printf("'h'            - help\n");
    printf("'a'            - toggle alpha test\n");
    printf("'b'            - toggle blend\n");
    printf("'s'            - toggle shrink\n");
    printf("'t'            - toggle MODULATE or REPLACE\n");
    printf("'4'            - toggle repeat by 4\n");
}

void init(int argc, char *argv[]) {
    unsigned *image;
    int i, width, height, components;

    glEnable(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (argv[0] == NULL) {
	  char name[256];
	  for (i = 0; i < 32; i++) {
  		sprintf(name, "../data/flame/f%02d", i);
 	    image = read_texture(name, &width, &height, &components);
	    if (image == NULL) {
		  fprintf(stderr, "Error: Can't load image file \"%s\".\n", argv[i]);
		  exit(EXIT_FAILURE);
		} else {
		  printf("%d x %d image loaded\n", width, height);
		}
		glBindTexture(GL_TEXTURE_2D, i+1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, components, width,
					 height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		texture_count++;
	  }
	}

    for(i = 0; i < argc; i++) {
	image = read_texture(argv[i], &width, &height, &components);
	if (image == NULL) {
	    fprintf(stderr, "Error: Can't load image file \"%s\".\n",
		    argv[i]);
	    exit(EXIT_FAILURE);
	} else {
	    printf("%d x %d image loaded\n", width, height);
	}
	glBindTexture(GL_TEXTURE_2D, i+1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	texture_count++;
    }

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
    glClearColor(.25f, .25f, .25f, .25f);

    glAlphaFunc(GL_GREATER, 0.f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
animate(void) {

    the_texture++;
    if (the_texture >= texture_count) the_texture = 0;

    glutPostRedisplay();
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, the_texture+1);
    glPushMatrix();
    if (shrink) glScalef(.5f, .5f, 1.f);
    glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex2f(-1.0, -1.0);
	glTexCoord2f(1.0, 0.0);
	glVertex2f(1.0, -1.0);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(1.0, 1.0);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(-1.0, 1.0);
    glEnd();
    glPopMatrix();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'a': afunc(); break;
    case 'b': bfunc(); break;
    case 'h': help(); break;
    case 's': sfunc(); break;
    case 't': tfunc(); break;
    case '4': fourfunc(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInitWindowSize(256, 256);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    (void)glutCreateWindow(argv[0]);
    init(argc-1, argv+1);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutIdleFunc(animate);
    glutMainLoop();
    return 0;
}
