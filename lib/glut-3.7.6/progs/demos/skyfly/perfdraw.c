
/*
 * perfdraw.c - $Revision: 1.4 $
 */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include "skyfly.h"

#if !defined(GL_VERSION_1_1)
#if defined(GL_EXT_texture_object)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#define glGenTextures(A,B)     glGenTexturesEXT(A,B)
#define glDeleteTextures(A,B)  glDeleteTexturesEXT(A,B)
#else
#define glBindTexture(A,B)
#define glGenTextures(A,B)
#define glDeleteTextures(A,B)
#endif
#endif

/* static routine decls */

extern int clouds;
static void drawlitmesh_11(float *objdata);
static void drawcolrtexmesh_10(float *objdata);
static void drawclouds(float *objdata);

void drawperfobj(perfobj_t *perfobj)
{
    float     *vdata_ptr =(float *) perfobj->vdata;
    extern void texenv(int), lightpos(void);

    unsigned int *flagsptr = perfobj->flags;
    float     *dp;

    for (;;) {
        switch (*flagsptr) {
		/*
		 * A paper plane is a single tmesh folded on itself so the orientations
		 * of some triangles in the mesh are incorrect with respect to
		 * their normals. This is solved by drawing the tmesh twice;
		 * first draw only backfaces, then only frontfaces.
		 */
		case PD_DRAW_PAPER_PLANE:
			flagsptr += 1;
			glCullFace(GL_FRONT);
			drawlitmesh_11(vdata_ptr);
			glCullFace(GL_BACK);
			drawlitmesh_11((float *)((perfobj_vert_t *) vdata_ptr + 11));
			glPopMatrix();
			break;

		case PD_DRAW_TERRAIN_CELL:
			dp = *(float **) (flagsptr + 1);
			flagsptr += 2;
			drawcolrtexmesh_10(dp);
			break;

		case PD_DRAW_CLOUDS:
                        if (rgbmode) {
#if 0
                            glColor3ub(0x30, 0x40, 0xb0);
#else
                            glColor3f(1.0f, 1.0f, 1.0f);
#endif
                        }
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			/*texenv(2);*/
			drawclouds(vdata_ptr);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			flagsptr += 1;
			break;

		case PD_PAPER_PLANE_MODE:
			switch (*(flagsptr + 1)) {
			case PLANES_START:
                            glShadeModel(GL_FLAT);
                            glEnable(GL_LIGHTING);
                            glDisable(GL_TEXTURE_2D);
                            if (fog && !rgbmode)
                                glDisable(GL_FOG);
                            break;
			case PLANES_END:
                            glShadeModel(GL_SMOOTH);
                            glDisable(GL_LIGHTING);
                            if (fog && !rgbmode && FOG_LEVELS > 1)
                                glEnable(GL_FOG);
                            break;
                        }	
			flagsptr += 2;
			break;

		case PD_PAPER_PLANE_POS:        /* contains the pushmatrix */
			glPushMatrix();
			glTranslatef(*(vdata_ptr), *(vdata_ptr + 1), *(vdata_ptr + 2));
			glRotatef(*(vdata_ptr + 3), 0.0, 0.0, 1.0);
			glRotatef(*(vdata_ptr + 4), 0.0, 1.0, 0.0);
			glRotatef(*(vdata_ptr + 5), 1.0, 0.0, 0.0);
			flagsptr += 1;
			break;

		case PD_VIEWER_POS:
			glLoadIdentity();
			glRotatef(-90., 1.0, 0., 0.);
			glRotatef(*(vdata_ptr + 3) * RAD_TO_DEG, 0.0, 0.0, 1.0); /* yaw */
			lightpos();
			glTranslatef(-*(vdata_ptr), -*(vdata_ptr + 1), -*(vdata_ptr + 2));
			flagsptr += 1;
			break;

		case PD_TEXTURE_BIND:
			glBindTexture(GL_TEXTURE_2D, *(flagsptr + 1));
			texenv(*(flagsptr + 1));
			glEnable(GL_TEXTURE_2D);
			flagsptr += 2;
			break;

		case PD_END:
			return;

		default:
			fprintf(stderr, "Bad PD primitive %d\n", *flagsptr);
			flagsptr++;
			break;
		}
	}
}

/*
 * Notice how the following routines unwind loops and pre-compute indexes
 * at compile time. This is crucial in obtaining the maximum data transfer 
 * from cpu to the graphics pipe.
 */
static void drawlitmesh_11(float *op)
{
    glBegin(GL_TRIANGLE_STRIP);
    /* one */
    glNormal3fv((op + PD_V_NORMAL));
    glVertex3fv((op + PD_V_POINT));
    /* two */
    glNormal3fv((op + (PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (PD_V_SIZE + PD_V_POINT)));
    /* three */
    glNormal3fv((op + (2 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (2 * PD_V_SIZE + PD_V_POINT)));
    /* four */
    glNormal3fv((op + (3 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (3 * PD_V_SIZE + PD_V_POINT)));
    /* five */
    glNormal3fv((op + (4 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (4 * PD_V_SIZE + PD_V_POINT)));
    /* six */
    glNormal3fv((op + (5 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (5 * PD_V_SIZE + PD_V_POINT)));
    /* seven */
    glNormal3fv((op + (6 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (6 * PD_V_SIZE + PD_V_POINT)));
    /* eight */
    glNormal3fv((op + (7 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (7 * PD_V_SIZE + PD_V_POINT)));
    /* nine */
    glNormal3fv((op + (8 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (8 * PD_V_SIZE + PD_V_POINT)));
    /* ten */
    glNormal3fv((op + (9 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (9 * PD_V_SIZE + PD_V_POINT)));
    /* eleven */
    glNormal3fv((op + (10 * PD_V_SIZE + PD_V_NORMAL)));
    glVertex3fv((op + (10 * PD_V_SIZE + PD_V_POINT)));

	glEnd();

}

static void drawcolrtexmesh_10(float *op)
{
    glBegin(GL_TRIANGLE_STRIP);
	/* one */
    glTexCoord2fv((op + PD_V_TEX));
    glColor3fv((op + PD_V_COLOR));
    glVertex3fv((op + PD_V_POINT));
    /* two */
    glTexCoord2fv((op + (PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (PD_V_SIZE + PD_V_POINT)));
    /* three */
    glTexCoord2fv((op + (2 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (2 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (2 * PD_V_SIZE + PD_V_POINT)));
    /* four */
    glTexCoord2fv((op + (3 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (3 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (3 * PD_V_SIZE + PD_V_POINT)));
    /* five */
    glTexCoord2fv((op + (4 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (4 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (4 * PD_V_SIZE + PD_V_POINT)));
    /* six */
    glTexCoord2fv((op + (5 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (5 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (5 * PD_V_SIZE + PD_V_POINT)));
    /* seven */
    glTexCoord2fv((op + (6 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (6 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (6 * PD_V_SIZE + PD_V_POINT)));
    /* eight */
    glTexCoord2fv((op + (7 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (7 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (7 * PD_V_SIZE + PD_V_POINT)));
    /* nine */
    glTexCoord2fv((op + (8 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (8 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (8 * PD_V_SIZE + PD_V_POINT)));
    /* ten */
    glTexCoord2fv((op + (9 * PD_V_SIZE + PD_V_TEX)));
    glColor3fv((op + (9 * PD_V_SIZE + PD_V_COLOR)));
    glVertex3fv((op + (9 * PD_V_SIZE + PD_V_POINT)));

    glEnd();
}

static void drawclouds(float *op)
{
#define SKY_STRIPS 24

	/* Break into quad strips so cheap fog works better */
	if (0 == clouds) {

        GLfloat *vc0, *vc1, *vc2;
        GLfloat *tc0, *tc1, *tc2;
        GLfloat sky_s0, sky_s1, sky_t0, sky_t1;
        GLfloat sky_x0, sky_x1, sky_y0, sky_y1, sky_z;
        int ii, jj;
        GLfloat s0, s1, t0, t1;
        GLfloat x0, x1, y0, y1, z0;

        vc0 = op + PD_V_POINT;
        vc1 = op + PD_V_POINT + PD_V_SIZE;
        vc2 = op + PD_V_POINT + PD_V_SIZE * 2;

        tc0 = op + PD_V_TEX;
        tc1 = op + PD_V_TEX + PD_V_SIZE;
        tc2 = op + PD_V_TEX + PD_V_SIZE * 2;

        sky_s0 = tc0[0];
        sky_s1 = tc1[0];
        sky_t0 = tc0[1];
        sky_t1 = tc2[1];

        sky_x0 = vc0[0];
        sky_x1 = vc1[0];
        sky_y0 = vc0[1];
        sky_y1 = vc2[1];
        sky_z  = vc0[2];

        clouds = glGenLists(1);

        glNewList(clouds, GL_COMPILE);

        s1 = (1.0f / SKY_STRIPS) * (sky_s1 - sky_s0);
        t1 = (1.0f / SKY_STRIPS) * (sky_t1 - sky_t0);
        x1 = (1.0f / SKY_STRIPS) * (sky_x1 - sky_x0);
        y1 = (1.0f / SKY_STRIPS) * (sky_y1 - sky_y0);

        z0 = sky_z;
        s0 = sky_s0;
        x0 = sky_x0;

        for (ii = 0; ii < SKY_STRIPS; ii++, s0 += s1, x0 += x1) {

            t0 = sky_t0;
            y0 = sky_y0;

            glBegin(GL_QUAD_STRIP);

            glTexCoord2f(s0, t0);
            glVertex3f(x0, y0, z0);

            glTexCoord2f(s0 + s1, t0);
            glVertex3f(x0 + x1, y0, z0);

            for (jj = 0; jj < SKY_STRIPS; jj++, t0 += t1, y0 += y1) {

                glTexCoord2f(s0 + s1, t0 + t1);
                glVertex3f(x0 + x1, y0 + y1, z0);

                glTexCoord2f(s0, t0 + t1);
                glVertex3f(x0, y0 + y1, z0);
            }

            glEnd();

        }

        glEndList();
    }

	glCallList(clouds);
}

void putv3fdata(float *v, perfobj_vert_t *ptr)
{
  ptr->vert[0] = v[0];
  ptr->vert[1] = v[1];
  ptr->vert[2] = v[2];
}

void putc3fdata(float *c, perfobj_vert_t *ptr)
{
  ptr->color[0] = c[0];
  ptr->color[1] = c[1];
  ptr->color[2] = c[2];
}

void putn3fdata(float *n, perfobj_vert_t *ptr)
{
  ptr->normal[0] = n[0];
  ptr->normal[1] = n[1];
  ptr->normal[2] = n[2];
}

void putt2fdata(float *t, perfobj_vert_t *ptr)
{
  ptr->texture[0] = t[0];
  ptr->texture[1] = t[1];
}

