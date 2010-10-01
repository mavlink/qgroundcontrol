#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"
#include <string.h>

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glTexSubImage2D	glTexSubImage2DEXT
#endif

static unsigned *image, *bgdtile;
static int width, height, components;
static int grid, zoom, texture;


#define TSIZE 128
#define TILES 16
#define TILESIZE 32
static struct tile {
    void *data;
} tiles[TILES][TILES];

static int x = TILES*TILESIZE/2, y = TILES*TILESIZE/2;

/*
 * make an rgb tile for the background.
 */
static void
background_tile(void) {
    int i,j,grid;
    unsigned char *ptr;

    bgdtile = (unsigned *) malloc(TILESIZE*TILESIZE*sizeof(unsigned));
    grid = 8;
    ptr = (unsigned char *) bgdtile;
    for (i=0; i<TILESIZE; i++) {
	for(j=0; j<TILESIZE; j++) {
	   if(i%grid == 0 || j%grid == 0) {
		*ptr++ = 0x0; *ptr++ = 0x0; *ptr++ = 0x40; ptr++;
	   } else {
		*ptr++ = 0x40; *ptr++ = 0x40; *ptr++ = 0x40; ptr++;
	   }
        }
    }
}

static void
tile_image(unsigned *image) {
    int i, j;
    int w = width/TILESIZE;
    int h = height/TILESIZE, w2 = w/2, h2 = h/2;
    background_tile();

    for(i = 0; i < TILES; i++) {
	for(j = 0; j < TILES; j++) {
	    if (i >= TILES/2-w2 && i < TILES/2-w2+w && j >= TILES/2-h2 && j < TILES/2-h2+h) {
		/* interior */
		int x, y, k;
		tiles[j][i].data = malloc(TILESIZE*TILESIZE*sizeof(*image));
		x = TILESIZE*(i-TILES/2+w2);
		y = TILESIZE*(j-TILES/2+h2);
		for(k = 0; k < TILESIZE; k++)
		    memcpy((unsigned *)tiles[j][i].data+k*TILESIZE,
			    image+width*(y+k)+x, TILESIZE*sizeof *image);
	    }
	    else
		tiles[j][i].data = bgdtile;
	}
    }
    for(i = 0; i < TILES; i++) {
	for(j = 0; j < TILES; j++) {
	    printf("%d ", tiles[j][i].data != bgdtile);
	}
	printf("\n");
    }
}

#define MAXMESH 64

static float Ml[4*2*(MAXMESH+1)*2 * (MAXMESH+1)];

static void
mesh0(float x0, float x1, float y0, float y1,
          float s0, float s1, float t0, float t1, float z, int nx,int ny)
{
    float y,x,s,t,dx,dy,ds,dt,vb[3],tb[2];
    float *mp = Ml;
	
    dx = (x1-x0)/nx;
    dy = (y1-y0)/ny;
    ds = (s1-s0)/nx;
    dt = (t1-t0)/ny;
    y = y0;
    t = t0;
    vb[2] = z;
    while (y < y1) {
        x = x0;
        s = s0;
        while(x <= x1) {
            tb[0] = s; tb[1] = t;
            vb[0] = x; vb[1] = y;
            vb[2] = 0.0;
            *mp++ = tb[0];	
            *mp++ = tb[1];	
            mp += 2;
            *mp++ = vb[0];	
            *mp++ = vb[1];	
            *mp++ = vb[2];	
            mp++;
            tb[1] = t+dt;
            vb[1] = y+dy;
            vb[2] = 0.0;
            *mp++ = tb[0];	
            *mp++ = tb[1];	
            mp += 2;
            *mp++ = vb[0];	
            *mp++ = vb[1];	
            *mp++ = vb[2];	
            mp++;
            x += dx;
            s += ds;
        }	
        y += dy;
        t += dt;
    }
}

static void
drawmesh(int nx,int ny) {
    float *mp = Ml;
    int i,j;

    glPushMatrix();
    if (zoom) glScalef(1.5f, 1.5f, 1.f);
    glColor4f(1,1,1,1);
    for (i = ny+1; i; i--) {
        glBegin(GL_TRIANGLE_STRIP);
        for (j = nx+1; j; j--) {
            glTexCoord2fv(mp);
            glVertex3fv(mp+4);
            glTexCoord2fv(mp+8);
            glVertex3fv(mp+12);	mp += 16;
        }
        glEnd();
    }
    glPopMatrix();
}

static void
help(void) {
    printf("'h'      - help\n");
    printf("'left'   - pan left\n");
    printf("'right'  - pan right\n");
    printf("'up'     - pan up\n");
    printf("'down'   - pan down\n");
    printf("'t'      - toggle texture memory display\n");
    printf("'g'	     - toggle grid\n");
    printf("'x'	     - toggle auto pan\n");
    printf("'z'      - toggle zoom\n");
}

static void
gfunc(void) {
    grid ^= 1;
}

static void
tfunc(void) {
    texture ^= 1;
}

static void anim(void);

static void
xfunc(void) {
    static int state;
    glutIdleFunc((state ^= 1) ? anim : NULL);
}

static void
zfunc(void) {
    zoom ^= 1;
}

#define CLAMP(v)	{ int w = TSIZE/2; \
			    if (v < w) v = w; \
			    else if (v > TILES*TILESIZE-w) v = TILES*TILESIZE-w; }
static void
up(void) {
    y += 8;
    CLAMP(y);
}

static void
pfunc(void) {
    static int delta = -1;
    int xx = x + delta;
    x += delta; CLAMP(x);
    y += delta; CLAMP(y);
    if (x != xx) delta = -delta;
}

static void
down(void) {
    y -= 8;
    CLAMP(y);
}

static void
right(void) {
    x += 8;
    CLAMP(x);
}

static void
left(void) {
    x -= 8;
    CLAMP(x);
}

static void
anim(void) {
    static int delta = -1;
    int xx = x + delta;
    x += delta;
    CLAMP(x);
    y += delta;
    CLAMP(y);
    if (x != xx) delta = -delta;
    glutPostRedisplay();
}


static void
loadtiles(void) {
    int lx, rx, ty, by;	/* image bounding box */
    static int ox = TILES*TILESIZE/2, oy = TILES*TILESIZE/2;  /* image origin */
    static int ot = 0, os = 0;
    int dx = 0, dy = 0, nx = -1, ny = -1;
    float trx, try;
#define S_TSIZE	(TSIZE-TILESIZE)	/* visible portion of texture = TSIZE less one tile for slop */

    /* calculate tile #'s at corners of visible region */
    lx = x - S_TSIZE/2;
    rx = lx + S_TSIZE;
    by = y - S_TSIZE/2;
    ty = by + S_TSIZE;
    lx /= TILESIZE; rx /= TILESIZE;
    by /= TILESIZE; ty /= TILESIZE;

    dx = ((x - S_TSIZE/2)/TILESIZE) - ((ox - S_TSIZE/2)/TILESIZE);
    
    nx = lx; ny = by;
    if (dx < 0) {
	/* add on left */
	os -= TILESIZE;
	if (os < 0) os += TSIZE;
	nx = lx;
    } else if (dx > 0) {
	nx = rx;
    }

    dy = ((y - S_TSIZE/2) / TILESIZE) - ((oy - S_TSIZE/2) / TILESIZE);
    if (dy > 0) {
	/* add on bottom */
	ny = ty;
    } else if (dy < 0) {
	/* add on top */
	ot -= TILESIZE;
	if (ot < 0) ot += TSIZE;
	ny = by;
    }
if (dx || dy) printf("dx %d dy %d   lx %d rx %d   by %d ty %d   nx %d ny %d   os %d ot %d\n", dx, dy, lx, rx, by, ty, nx, ny, os, ot);
    if (dx) {
	int t;
	for(t = 0; t < TSIZE; t += TILESIZE) {
	    glTexSubImage2D(GL_TEXTURE_2D, 0, os, (t+ot) % TSIZE, TILESIZE,
                 TILESIZE, GL_RGBA, GL_UNSIGNED_BYTE,
                 tiles[ny+t/TILESIZE][nx].data);
printf("load %d %d  %d %d\n", nx, ny+t/TILESIZE, os, (t+ot) % TSIZE);
	}
    }

    if (dy) {
	int s;
	for(s = 0; s < TSIZE; s += TILESIZE) {
	    glTexSubImage2D(GL_TEXTURE_2D, 0, (s+os) % TSIZE, ot, TILESIZE,
                 TILESIZE, GL_RGBA, GL_UNSIGNED_BYTE,
                 tiles[ny][nx+s/TILESIZE].data);
printf("load %d %d  %d %d\n", nx+s/TILESIZE, ny, (s+os) % TSIZE, ot);
	}
    }
    if (dx > 0) {
	os += TILESIZE;
	if (os >= TSIZE) os -= TSIZE;
    }
    if (dy > 0) {
	ot += TILESIZE;
	if (ot >= TSIZE) ot -= TSIZE;
    }
    ox = x; oy = y;
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    trx = (float)((x-TILES*TILESIZE/2) % TSIZE)/TSIZE;
    try = (float)((y-TILES*TILESIZE/2) % TSIZE)/TSIZE;
    glTranslatef(trx, try, 0.f);
    glMatrixMode(GL_MODELVIEW);
}

static void
init(char *filename) {
    int i;

    mesh0(-1.f,1.f,-1.f,1.f,0.f,1.f,0.f,1.f,0.f,64,64);
    if (filename) {
	image = read_texture(filename, &width, &height, &components);
	if (image == NULL) {
	    fprintf(stderr, "Error: Can't load image file \"%s\".\n",
		    filename);
	    exit(EXIT_FAILURE);
	} else {
	    printf("%d x %d image loaded\n", width, height);
	}
	if (components < 3 || components > 4) {
	    printf("must be RGB or RGBA image\n");
	    exit(EXIT_FAILURE);
	}
    } else {
	int i, j;
	components = 4; width = height = TSIZE;
	image = (unsigned *) malloc(width*height*sizeof(unsigned));
	for (j = 0; j < height; j++)
	    for (i = 0; i < width; i++) {
		if (i & 1)
		    image[i+j*width] = 0xff;
		else
		    image[i+j*width] = 0xff00;
		if (j&1)
		    image[i+j*width] |= 0xff0000;
	    }

    }
    if (width % TILESIZE || height % TILESIZE) {
#define TXSIZE 192
	unsigned *newimage = malloc(TXSIZE*TXSIZE*sizeof *newimage);
	gluScaleImage(GL_RGBA, width, height, GL_UNSIGNED_BYTE, image,
		TXSIZE, TXSIZE, GL_UNSIGNED_BYTE, newimage);
	free(image);
	image = newimage; width = height = TXSIZE; components = 4;
    }
    tile_image(image);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TSIZE,
                 TSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    for(i = 0; i < TILES; i++) {
	int j;
	for(j = 0; j < TILES; j++) {
	    glTexSubImage2D(GL_TEXTURE_2D, 0, i*TILESIZE, j*TILESIZE, TILESIZE,
		 TILESIZE, GL_RGBA, GL_UNSIGNED_BYTE, 
		 tiles[(TILES-TSIZE/TILESIZE)/2+j][(TILES-TSIZE/TILESIZE)/2+i].data);
	}
    }
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.,1.,.1,10.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-1.0);
    glLineWidth(3.0);
    glClearColor(.25, .25, .25, .25);

    /* start at center of image */
    x = TILES*TILESIZE/2;
    y = TILES*TILESIZE/2;
}

void
showgrid(void) {
    GLfloat mat[16];
    int i;

    glPushMatrix();
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();
    glColor3f(0.f, 0.f, 0.f);
    if (!zoom) glScalef(1.f/1.5f, 1.f/1.5f, 1.f);
    glBegin(GL_LINE_LOOP);
	glVertex2f(-1.f,-1.f);
	glVertex2f(-1.f, 1.f);
	glVertex2f( 1.f, 1.f);
	glVertex2f( 1.f,-1.f);
    glEnd();
    glPopMatrix();

    glGetFloatv(GL_TEXTURE_MATRIX,mat);
    glPushMatrix();
    if (zoom) glScalef(1.5f,1.5f,1.f);
    glTranslatef(-1.f,-1.f,-1.f);
    glScalef(2.f,2.f,1.f);
    glTranslatef(-mat[12], -mat[13], 1.0f);
#if 1
    glColor3f(1.f,1.f,1.f);
#else
    glColor3f(1.f,0.f,0.f);
#endif
    glBegin(GL_LINES);
    for(i = -TSIZE; i <= 2*TSIZE; i+=TILESIZE) {
	GLfloat x = (GLfloat)i/(GLfloat)TSIZE;
	glVertex2f(-1.f,x);
	glVertex2f(2.f,x);
	glVertex2f(x,-1.f);
	glVertex2f(x,2.f);
    }
    glEnd();
    glPopMatrix();
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

static void
drawtexture(void) {
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(1.f,1.f,1.f);
    glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f); glVertex2f(-1.f, -1.f);
	glTexCoord2f(0.f, 1.f); glVertex2f(-1.f,  1.f);
	glTexCoord2f(1.f, 1.f); glVertex2f( 1.f,  1.f);
	glTexCoord2f(1.f, 0.f); glVertex2f( 1.f, -1.f);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void
display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    loadtiles();
    if (texture) {
	drawtexture();
    } else {
	drawmesh(64,64);
	if (grid) showgrid();
    }
    glutSwapBuffers();
}

static void
reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/*ARGSUSED1*/
static void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'h': help(); break;
    case 'g': gfunc(); break;
    case 't': tfunc(); break;
    case 'z': zfunc(); break;
    case 'x': xfunc(); break;
    case 'p': pfunc(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

/*ARGSUSED1*/
static void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:	up(); break;
    case GLUT_KEY_DOWN:	down(); break;
    case GLUT_KEY_LEFT:	left(); break;
    case GLUT_KEY_RIGHT:right(); break;
    }
    glutPostRedisplay();
}

int main(int argc, char* argv[]) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    (void)glutCreateWindow(argv[0]);
    init(argv[1] ? argv[1] : "../data/fendi.rgb");
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutReshapeFunc(reshape);
    glutMainLoop();
    return 0;
}
