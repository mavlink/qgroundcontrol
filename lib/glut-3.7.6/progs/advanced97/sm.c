#include "stdlib.h"
#include "math.h"
#include <GL/glut.h>
#include "sm.h"

#ifdef _WIN32
#define drand48() ((double)rand()/RAND_MAX)
#define srand48(x) (srand((x)))
#endif

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture	glBindTextureEXT
#endif

typedef struct elem {
    float x, y, z;	/* current position */
    float dx, dy, dz;	/* displacement */
    float size;		/* scale factor */
    float ts;		/* time stamp */
    float opacity;	/* alpha value */
} elem_t;

typedef struct smoke {
    float ox, oy, oz;	/* origin */
    float dx, dy, dz;	/* drift */
    int elems;
    float intensity;
    float min_size;
    float max_size;
    unsigned texture;
    elem_t *elem;
} smoke_t;

void *
new_smoke(float x, float y, float z, float dx, float dy, float dz,
	int elems, float intensity, unsigned texture) {
    int i;
    smoke_t *s = malloc(sizeof(smoke_t));

    s->ox = x; s->oy = y, s->oz = z;
    s->dx = dx; s->dy = dy; s->dz = dz;
    s->min_size = .1f;
    s->max_size = 1.0;
    s->elems = elems;
    s->elem = malloc(sizeof(elem_t)*elems);
    for(i = 0; i < elems; i++) {
	s->elem[i].ts = (float)i/elems;;
	s->elem[i].dx = -drand48()*1.5f;
	s->elem[i].dy = drand48()*1.5f;
	s->elem[i].dz = drand48()*1.5f;
    }
    s->intensity = intensity;
    s->texture = texture;
    return s;
}

void
delete_smoke(void *smoke) {
    smoke_t *s = smoke;
    free(s->elem);
    free(s);
}

void
draw_smoke(void *smoke) {
    smoke_t *s = smoke;
    int i;

    glEnable(GL_BLEND);
    glDepthMask(0);
#if 1
    glEnable(GL_TEXTURE_2D);
#else
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
    glBindTexture(GL_TEXTURE_2D, s->texture);
    for(i = 0; i < s->elems; i++) {
	elem_t *e = s->elem+i;
	glPushMatrix();
	glTranslatef(e->x, e->y, e->z);
	glScalef(e->size, e->size, 1.);
	glColor4f(s->intensity,s->intensity,s->intensity,e->opacity);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-1., -1., -0.);
	glTexCoord2f(0, 1); glVertex3f(-1., 1.,  0.);
	glTexCoord2f(1, 1); glVertex3f( 1., 1.,  0.);
	glTexCoord2f(1, 0); glVertex3f( 1., -1., -0.);
	glEnd();
	glPopMatrix();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(1);
    glDisable(GL_BLEND);
}

void
update_smoke(void *smoke, float tick) {
    smoke_t *s = smoke;
    int i;

    for(i = 0; i < s->elems; i++) {
	elem_t *e = s->elem+i;
	e->ts += tick;
	if (e->ts > 1.0) e->ts = 0;
	e->x = s->ox + s->dx*e->ts + e->dx*e->ts;
	e->y = s->oy + s->dy*e->ts + e->dy*e->ts;
	e->z = s->oz + s->dz*e->ts + e->dz*e->ts;
	e->size = s->min_size + e->ts*s->max_size;
	e->opacity = (1.0-e->ts);
    }
}
