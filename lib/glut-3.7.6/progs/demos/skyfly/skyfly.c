
/*
 * skyfly.c     $Revision: 1.5 $
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <GL/glut.h>
#include <math.h>
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
#if defined(GL_EXT_texture)
#define GL_RGB5 GL_RGB5_EXT
#else
#define GL_RGB5 GL_RGB
#endif
#endif

#define ERR_WARNING                     0x1
#define ERR_FATAL                       0x2
#define ERR_SYSERR                      0x4

#define AMALLOC(a, type, num, func)     {                     \
	if((int)(a = (type*)malloc(sizeof(type)*(num))) <= 0)     \
		err_msg(ERR_FATAL, func, "amalloc failed");           \
}

float   ScaleZ   = 2.3;     /* Terrain height scale factor */
int     CellDim = 4;        /* Terrain cell is CellDim X Celldim quads */
int     NumCells = 36;      /* Terrain grid is NumCells X NumCells cells */
int     GridDim;            /* Terrain grid is GridDim X GridDim quads */
float   XYScale;            /* Conversion from world-space to grid index */
float   CellSize;           /* World-space size of cell */

int     Init_pos;           /* if true, set initial position and kbd mode */
float   Init_x, Init_y, Init_z, Init_azimuth;
int     rgbmode = GL_TRUE;

/* Color index ramp info */
int sky_base, terr_base;
int plane_colors[3];

/*
 * Data that changes from frame to frame needs to be double-buffered because
 * two processes may be working on two different frames at the same time.
 */
typedef struct buffered_data_struct {

        /* objects */
        perfobj_t       paper_plane_pos_obj[NUM_PLANES];
        perfobj_t       viewer_pos_obj;

        /* flags */
        unsigned int   paper_plane_pos_flags[2];
        unsigned int   viewer_pos_flags[2];

        /* data */
        float  paper_plane_position[NUM_PLANES][6];
        float  viewer_position[4];

} buffered_data;

/*
 * This is the per-pipe data structure which holds pipe id, semaphores,
 * and variable data buffers. Both gfxpipe and buffer structures live in
 * shared memory so the sim can communicate with its forked children.
 */
typedef struct gfxpipe_data_struct {
        int             gfxpipenum;
		buffered_data   **buffers;

} gfxpipe_data;

static gfxpipe_data     *gfxpipe;       /* A processes' gfxpipe struct */
static gfxpipe_data     *gfxpipes[1];   /* Maximum of 1 graphics pipe */
static int              num_pipes;
float           		fog_params[4];  /* Fog and clear color */
static float            fog_density = 0.025*2;
float           		far_cull  = 31.;    /* Far clip distance from eye */
int			mipmap = 0;
static int              texmode = GL_NEAREST;

static int              threecomp = 1;

int 	dither = GL_TRUE, fog = GL_TRUE;
int     Wxsize = 320, Wysize = 240; /* Default to 320x240 window */

/*
 * All non-variable data like geometry is stored in shared memory. This way
 * forked processes avoid duplicating data unnecessarily.
 */
shared_data     *SharedData;

/* //////////////////////////////////////////////////////////////////////// */

void sim_proc(void);
void sim_cyclops(void);
void sim_dualchannel(void);
void sim_singlechannel(void);
void cull_proc(void);
void draw_proc(void);
void sim_exit(void);
void init_misc(void); 
void init_shmem(void); 
void init_terrain(void);
void init_clouds(void);
void init_paper_planes(void);
void init_positions(void);
void init_gfxpipes(void);
void init_gl(int gfxpipenum);
void err_msg(int type, char* func, char* error);
void fly(perfobj_t *viewer_pos);
void fly_paper_planes(perfobj_t *paper_plane_pos);
float terrain_height(void);

void init_skyfly(void)
{
    init_shmem();
    init_gfxpipes();
    init_clouds();
    init_terrain();
    init_paper_planes();
    init_positions();

    gfxpipe = gfxpipes[0];
    init_gl(gfxpipe->gfxpipenum);
}

/*
 * This is a single-channel version of the dual-channel simulation
 * described above.
 */
void sim_singlechannel(void)
{
	buffered_data  **buffered = gfxpipes[0]->buffers;

	fly(&(buffered[0]->viewer_pos_obj));
	fly_paper_planes(buffered[0]->paper_plane_pos_obj);
}

/*-------------------------------------- Cull ------------------------------*/

/*
 *   The cull and draw processes operate in a classic producer/consumer, 
 * write/read configuration using a ring buffer. The ring consists of pointers
 * to perfobj's instead of actual geometric data. This is important because
 * you want to minimize the amount of data 'shared' between two processes that
 * run on different processors in order to reduce cache invalidations.
 *   enter_in_ring and get_from_ring spin on ring full and ring empty 
 * conditions respectively.
 *   Since cull/draw are shared group processes(sproc), the ring buffer is
 * in the virtual address space of both processes and shared memory is not
 * necessary.
*/

#define RING_SIZE   1000    /* Size of ring */

typedef struct render_ring_struct {
    volatile unsigned int      head, tail;
    perfobj_t                   **ring;
} render_ring;

render_ring ringbuffer;

void        enter_in_ring(perfobj_t *perfobj);
perfobj_t*  get_from_ring(void);

void cull_proc(void)
{

    static struct cull {
        perfobj_t       **cells;
        perfobj_t       viewer_pos_obj[2];
        unsigned int   viewer_pos_flags[4];
        float           viewer_position[2][4];
        float           fovx, side, farr, epsilon, plane_epsilon;
    } cull;

    static int init = 0;

    if (!init) {
        int             x, y;

        cull.fovx = FOV *(float) Wxsize /(float) Wysize;
        cull.side = far_cull / cosf(cull.fovx / 2.);
        cull.farr = 2.* cull.side * sinf(cull.fovx / 2.);
        cull.epsilon = sqrtf(2.) * CellSize / 2.;
        cull.plane_epsilon = .5;

        cull.cells = (perfobj_t **) malloc(NumCells * NumCells * sizeof(perfobj_t *));
        for (x = 0; x < NumCells; x++)
            for (y = 0; y < NumCells; y++)
                cull.cells[x * NumCells + y] =
                    &(SharedData->terrain_cells[x * NumCells + y]);

        ringbuffer.ring = malloc(RING_SIZE * sizeof(perfobj_t *));
        ringbuffer.head = ringbuffer.tail = 0;

        cull.viewer_pos_obj[0].flags = cull.viewer_pos_flags;
        cull.viewer_pos_obj[0].vdata = cull.viewer_position[0];
        cull.viewer_pos_obj[1].flags = cull.viewer_pos_flags;
        cull.viewer_pos_obj[1].vdata = cull.viewer_position[1];

        *(cull.viewer_pos_flags) = PD_VIEWER_POS;
        *(cull.viewer_pos_flags + 1) = PD_END;
        init = 1;
    }

	{
	float           *viewer;
	float           vX, vY, vazimuth, px, py;
	float           left_area, right_area;
	float           left_dx, left_dy, right_dx, right_dy;
	float           ax, ay, bx, by, cx, cy;
	float           minx, maxx, miny, maxy;
	int             i, buffer = 0;
	int             x, y, x0, y0, x1, y1;
	perfobj_t      *viewer_pos, *paper_plane_pos;
	buffered_data  *buffered;
	perfobj_t      *terrain_texture = &(SharedData->terrain_texture_obj);
	perfobj_t      *paper_plane = &(SharedData->paper_plane_obj);
	perfobj_t      *paper_plane_start = &(SharedData->paper_plane_start_obj);
	perfobj_t      *paper_plane_end = &(SharedData->paper_plane_end_obj);
	perfobj_t      *clouds_texture = &(SharedData->clouds_texture_obj);
	perfobj_t      *clouds = &(SharedData->clouds_obj);

	buffered = gfxpipe->buffers[buffer];

	viewer_pos = &(buffered->viewer_pos_obj);
	paper_plane_pos = buffered->paper_plane_pos_obj;

	vX = *((float *) viewer_pos->vdata + 0);
	vY = *((float *) viewer_pos->vdata + 1);
	vazimuth = *((float *) viewer_pos->vdata + 3);

	viewer = cull.viewer_position[buffer];

	viewer[0] = vX;
	viewer[1] = vY;
	viewer[2] = *((float *) viewer_pos->vdata + 2);
	viewer[3] = vazimuth;

	/*
	 * Begin cull to viewing frustrum
	 */
	ax = (vX - sinf(-vazimuth + cull.fovx *.5) * cull.side);
	ay = (vY + cosf(-vazimuth + cull.fovx *.5) * cull.side);
	bx = vX;
	by = vY;
	cx = (vX + sinf(vazimuth + cull.fovx *.5) * cull.side);
	cy = (vY + cosf(vazimuth + cull.fovx *.5) * cull.side);

	minx = MIN(MIN(ax, bx), cx);
	miny = MIN(MIN(ay, by), cy);
	maxx = MAX(MAX(ax, bx), cx);
	maxy = MAX(MAX(ay, by), cy);

	x0 = MAX((int) (minx / CellSize), 0);
	x1 = MIN((int) (maxx / CellSize) + 1, NumCells);
	y0 = MAX((int) (miny / CellSize), 0);
	y1 = MIN((int) (maxy / CellSize) + 1, NumCells);

	left_dx = ax - bx;
	left_dy = ay - by;
	right_dx = cx - bx;
	right_dy = cy - by;

	enter_in_ring(&cull.viewer_pos_obj[buffer]);

	if (viewer[2]<SKY_HIGH) {
	   /* draw clouds first */
	   enter_in_ring(clouds_texture);
	   enter_in_ring(clouds);
	}

	enter_in_ring(terrain_texture);
	/*
	 * Add visible cells to ring buffer 
	 */
	for (x = x0; x < x1; x++) {
		for (y = y0; y < y1; y++) {
			float           cntrx =(x +.5) * CellSize;
			float           cntry =(y +.5) * CellSize;

			left_area = left_dx * (cntry - by) - left_dy * (cntrx - bx);
			right_area = right_dx * (cntry - by) - right_dy * (cntrx - bx);

			if ((left_area < cull.epsilon * cull.side && right_area > -cull.epsilon * cull.side)) {
					enter_in_ring(cull.cells[x * NumCells + y]);
			}
		}
	}

	enter_in_ring(paper_plane_start);
	/*
	 * Add visible planes to ring buffer
	*/
	for (i = 0; i < NUM_PLANES; i++) {

		px = *((float *) paper_plane_pos[i].vdata + 0);
		py = *((float *) paper_plane_pos[i].vdata + 1);
		left_area = left_dx * (py - by) - left_dy * (px - bx);
		right_area = right_dx * (py - by) - right_dy * (px - bx);

		if (left_area < cull.plane_epsilon * cull.side && right_area > -cull.plane_epsilon * cull.side) {
			enter_in_ring(&paper_plane_pos[i]);
			enter_in_ring(paper_plane);
		}
	}

	enter_in_ring(paper_plane_end);

	if (viewer[2]>SKY_HIGH) {
	   /* draw clouds after everything else */
	   enter_in_ring(clouds_texture);
	   enter_in_ring(clouds);
	}

	enter_in_ring((perfobj_t *) 0);     /* 0 indicates end of frame */

	buffer = !buffer;
	}
}

void enter_in_ring(perfobj_t *perfobj)
{
	while (ringbuffer.head == RING_SIZE+ringbuffer.tail-1) {}
	ringbuffer.ring[ringbuffer.head % RING_SIZE] = perfobj;
	ringbuffer.head++;
}

perfobj_t* get_from_ring(void)
{
    static perfobj_t *pobj;

    while(ringbuffer.tail == ringbuffer.head) {}
    pobj = ringbuffer.ring[ringbuffer.tail % RING_SIZE];
    ringbuffer.tail++;
    return pobj;        
}

/*-------------------------------------- Draw ------------------------------*/

void draw_proc(void)
{
	perfobj_t      *too_draw;

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	while ((too_draw = get_from_ring())) {
		drawperfobj(too_draw);
	}
}


/*------------------------------- Init -----------------------------------*/

void init_texture_and_lighting(void);
void init_buffered_data(buffered_data *buffered);

void init_misc(void)
{
	float   density;

	threecomp = rgbmode;

	/*
	 * Compute fog and clear color to be linear interpolation between blue
	 * and white.
	 */
	density = 1.- expf(-5.5 * fog_density * fog_density *
							  far_cull * far_cull);
	density = MAX(MIN(density, 1.), 0.);

	fog_params[0] = .23 + density *.57;
	fog_params[1] = .35 + density *.45;
	fog_params[2] = .78 + density *.22;
	fog_params[3] = 1.0f;
}

void init_shmem(void)
{
	int			   i;
    unsigned int  *flagsptr;
    perfobj_vert_t *vertsptr;
    int             nflags, nverts;

    AMALLOC(SharedData, shared_data, 1, "init_shmem");
    AMALLOC(SharedData->terrain_cells, perfobj_t,
            NumCells * NumCells, "init_shmem");
    AMALLOC(SharedData->terrain_cell_flags, unsigned int *,
            NumCells * NumCells, "init_shmem");
    AMALLOC(SharedData->terrain_cell_verts, perfobj_vert_t *,
            NumCells * NumCells, "init_shmem");

    /*
     * Allocate the flags and vertices of all terrain cells in 2 big chunks
     * to improve data locality and consequently, cache hits 
     */
    nflags = 2 * CellDim + 1;
	AMALLOC(flagsptr, unsigned int, nflags * NumCells * NumCells, "init_shmem");
	nverts = (CellDim + 1) * 2 * CellDim;
	AMALLOC(vertsptr, perfobj_vert_t, nverts * NumCells * NumCells, "init_shmem");

	for (i = 0; i < NumCells * NumCells; i++) {
		SharedData->terrain_cell_flags[i] = flagsptr;
		flagsptr += nflags;
		SharedData->terrain_cell_verts[i] = vertsptr;
		vertsptr += nverts;
	}
}

/*
 * Initialize gfxpipe data structures. There is one set of semaphores
 * per pipe.
 */
void init_gfxpipes(void)
{
	int             i, j;

	num_pipes = 1;

	for (i = 0; i < num_pipes; i++) {

		AMALLOC(gfxpipes[i], gfxpipe_data, 1, "initgfxpipes");
		AMALLOC(gfxpipes[i]->buffers, buffered_data *, NBUFFERS,
				"init_gfxpipes");
		gfxpipes[i]->gfxpipenum = i;
	}

	for (j = 0; j < NBUFFERS; j++) {
		AMALLOC(gfxpipes[0]->buffers[j], buffered_data, 1,
				"init_gfxpipes");
		init_buffered_data(gfxpipes[0]->buffers[j]);
		}
}

void init_buffered_data(buffered_data *buffered)
{
    int             i;
    perfobj_t      *pobj;

    pobj = &(buffered->viewer_pos_obj);
    pobj->flags = buffered->viewer_pos_flags;
    pobj->vdata = buffered->viewer_position;

    *(buffered->viewer_pos_flags) = PD_VIEWER_POS;
    *(buffered->viewer_pos_flags + 1) = PD_END;

    for (i = 0; i < NUM_PLANES; i++) {
        pobj = &(buffered->paper_plane_pos_obj[i]);
        pobj->flags = buffered->paper_plane_pos_flags;
        pobj->vdata = buffered->paper_plane_position[i];
    }
    *(buffered->paper_plane_pos_flags) = PD_PAPER_PLANE_POS;
    *(buffered->paper_plane_pos_flags + 1) = PD_END;
}

/* ARGSUSED */
void init_gl(int gfxpipenum)
{
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
	if (!rgbmode)
		glIndexi(0);

	set_fog(fog);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

    set_dither(dither);

	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

	init_texture_and_lighting();

	glMatrixMode(GL_PROJECTION);
	gluPerspective(FOV * RAD_TO_DEG, (float)Wxsize/(float)Wysize,
														 .1, far_cull *.95);
	glMatrixMode(GL_MODELVIEW);
	glHint(GL_FOG_HINT, GL_FASTEST);
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogf(GL_FOG_DENSITY, fog_density);

	if (rgbmode) {
		glFogfv(GL_FOG_COLOR, fog_params);
		if (fog && fog_density > 0)
			glEnable(GL_FOG);
	} else if (FOG_LEVELS > 1) {
		glFogi(GL_FOG_INDEX, FOG_LEVELS-1);
		if (fog)
			glEnable(GL_FOG);
	}
}

unsigned char* read_bwimage(char *name, int *w, int *h);

void init_texture_and_lighting(void)
{

    unsigned char  *bwimage256, *bwimage128;
    int             w, h;

    if(!(bwimage256 = (unsigned char*) read_bwimage("terrain.bw", &w, &h)))
       if(!(bwimage256 = (unsigned char *) 
                read_bwimage("/usr/demos/data/textures/terrain.bw", &w, &h)))
		err_msg(ERR_FATAL, "init_texture_and_lighting()",
										"Can't open terrain.bw");

	if(w != 256 || h != 256)
		err_msg(ERR_FATAL, "init_texture_and_lighting()",
										"terrain.bw must be 256x256");

	if (!(bwimage128 = (unsigned char *) read_bwimage("clouds.bw", &w, &h)))
		if (!(bwimage128 = (unsigned char *)
			  read_bwimage("/usr/demos/data/textures/clouds.bw", &w, &h)))
			err_msg(ERR_FATAL, "init_misc()", "Can't open clouds.bw");

	if (w != 128 || h != 128)
		err_msg(ERR_FATAL, "init_misc()", "clouds.bw must be 128x128");

	if (mipmap)
		texmode = GL_LINEAR_MIPMAP_LINEAR;
	else
		texmode = GL_NEAREST;

	if (!threecomp) {
		/*
		 * 1 and 2-component textures offer the highest performance on SkyWriter
		 * so they are the most recommended.
		 */
		glBindTexture(GL_TEXTURE_2D, 1);
		if (!mipmap)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texmode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texmode);
		if (mipmap)
			gluBuild2DMipmaps(GL_TEXTURE_2D, /*0,*/ 1, 256, 256, /*0,*/ GL_LUMINANCE, GL_UNSIGNED_BYTE, bwimage256);
		else if (rgbmode)
			glTexImage2D(GL_TEXTURE_2D, 0, 1, 256, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bwimage256);
		else {
#define TXSIZE 128
			GLubyte buf[TXSIZE*TXSIZE];
			int ii;
			gluScaleImage(GL_LUMINANCE, 256, 256, GL_UNSIGNED_BYTE, bwimage256,
						  TXSIZE, TXSIZE, GL_UNSIGNED_BYTE, buf);
			for (ii = 0; ii < TXSIZE*TXSIZE; ii++) {
				buf[ii] = terr_base +
					FOG_LEVELS * (buf[ii] >> (8-TERR_BITS));
			}
#ifdef GL_COLOR_INDEX8_EXT  /* Requires SGI_index_texture and EXT_paletted_texture */
			glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, TXSIZE, TXSIZE,
						 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buf);
#endif
#undef TXSIZE
		}

		glBindTexture(GL_TEXTURE_2D, 2);
		if (!mipmap)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texmode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texmode);
		if (mipmap)
			gluBuild2DMipmaps(GL_TEXTURE_2D, /*0,*/ 1, 128, 128, /*0,*/ GL_LUMINANCE, GL_UNSIGNED_BYTE, bwimage128);
		else if (rgbmode)
			glTexImage2D(GL_TEXTURE_2D, 0, 1, 128, 128, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bwimage128);
		else {
#define TXSIZE 64
			GLubyte buf[TXSIZE*TXSIZE];
			int ii;
			gluScaleImage(GL_LUMINANCE, 128, 128, GL_UNSIGNED_BYTE, bwimage128,
						  TXSIZE, TXSIZE, GL_UNSIGNED_BYTE, buf);
			for (ii = 0; ii < TXSIZE*TXSIZE; ii++) {
				buf[ii] = sky_base +
					FOG_LEVELS * (buf[ii] >> (8-SKY_BITS));
			}
#ifdef GL_COLOR_INDEX8_EXT  /* Requires SGI_index_texture and EXT_paletted_texture */
			glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, TXSIZE, TXSIZE,
						 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buf);
#endif
#undef TXSIZE
		}
	} else {
		float r0, r1, g0, g1, b0, b1;
		int i;
		unsigned char *t256 = (unsigned char *)malloc(256*256*3);

		/* terrain */
		r0 = 0.40f;   r1 = 0.30f;
		g0 = 0.30f;   g1 = 0.70f;
		b0 = 0.15f;   b1 = 0.10f;

		for(i = 0; i < 256*256; i++) {
			float t = bwimage256[i] / 255.0f;
			t256[3*i+0] = (unsigned char) (255.0f * (r0 + t*t * (r1-r0)));
			t256[3*i+1] = (unsigned char) (255.0f * (g0 + t * (g1-g0)));
			t256[3*i+2] = (unsigned char) (255.0f * (b0 + t*t * (b1-b0)));
		}
		glBindTexture(GL_TEXTURE_2D, 1);
		if (!mipmap)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texmode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texmode);
		if (mipmap)
			gluBuild2DMipmaps(GL_TEXTURE_2D, /*0,*/ 3, 256, 256, /*0,*/ GL_RGB, GL_UNSIGNED_BYTE, t256);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, t256);


		/* sky without fog */
		r0 = 0.23;  r1 = 1.0f;
		g0 = 0.35;  g1 = 1.0f;
		b0 = 0.78;  b1 = 1.0f;
		for(i = 0; i < 128*128; i++) {
			float t = bwimage128[i] / 255.0f;
			t256[3*i+0] = (unsigned char) (255.0f * (r0 + t * (r1-r0)));
			t256[3*i+1] = (unsigned char) (255.0f * (g0 + t * (g1-g0)));
			t256[3*i+2] = (unsigned char) (255.0f * (b0 + t * (b1-b0)));
		}
		glBindTexture(GL_TEXTURE_2D, 2);
		if (!mipmap)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texmode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texmode);
		if (mipmap)
			gluBuild2DMipmaps(GL_TEXTURE_2D, /*0,*/ 3, 128, 128, /*0,*/ GL_RGB, GL_UNSIGNED_BYTE, t256);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, t256);

		/* sky with fog */
		r0 = fog_params[0];  r1 = 1.0f;
		g0 = fog_params[1];  g1 = 1.0f;
		b0 = fog_params[2];  b1 = 1.0f;
		for(i = 0; i < 128*128; i++) {
			float t = bwimage128[i] / 255.0f;
			t256[3*i+0] = (unsigned char) (255.0f * (r0 + t * (r1-r0)));
			t256[3*i+1] = (unsigned char) (255.0f * (g0 + t * (g1-g0)));
			t256[3*i+2] = (unsigned char) (255.0f * (b0 + t * (b1-b0)));
		}
		glBindTexture(GL_TEXTURE_2D, 3);
		if (!mipmap)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texmode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texmode);
		if (mipmap)
			gluBuild2DMipmaps(GL_TEXTURE_2D, /*0,*/ 3, 128, 128, /*0,*/ GL_RGB, GL_UNSIGNED_BYTE, t256);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, t256);
		free(t256);
	}

	free(bwimage256);
	free(bwimage128);

	/* both textures use BLEND environment */
	if (rgbmode) {
            if (threecomp) {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            }
            else {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            }
	} else if (FOG_LEVELS > 1) {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	} else {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}

	{
            GLfloat position[] = { LX, LY, LZ, 0., };
            GLfloat one[] = { 1.0, 1.0, 1.0, 1.0 };

            if (rgbmode)
                glLightfv(GL_LIGHT0, GL_AMBIENT, one);
            glLightfv(GL_LIGHT0, GL_POSITION, position);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, one);
            glLightfv(GL_LIGHT0, GL_SPECULAR, one);
	}

	if (rgbmode) {
		GLfloat ambient[] = { 0.3, 0.3, 0.1, 0.0 };
		GLfloat diffuse[] = { 0.7, 0.7, 0.1, 0.0 };
		GLfloat zero[] = { 0.0, 0.0, 0.0, 0.0 };

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	} else {
		glMaterialiv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, plane_colors);
	}

	{
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
		glEnable(GL_LIGHT0);
	}
}

void lightpos(void)
{
    GLfloat position[] = { LX, LY, LZ, 0., };
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

void texenv(int env)
{
    GLfloat colors[3][4] = { { 0., 0., 0., 0., },
                             { .1, .1, .1, 0., },       /* terrain */
                             { 1., 1., 1., 0., }, };    /* sky */
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colors[env]);
}

/*-------------------------------- Utility ---------------------------------*/

void err_msg(int type, char* func, char* error)
{
	char    msg[512];

	if (type & ERR_WARNING) {
		fprintf(stderr, "Warning:  ");
		sprintf(msg, "Warning:  %s", error);
	}
	else if (type & ERR_FATAL) {
		fprintf(stderr, "FATAL:  ");
		sprintf(msg, "FATAL:  %s", error);
	}

	fprintf(stderr, "%s: %s\n", func, error);
	if (type & ERR_SYSERR) {
		perror("perror() = ");
		fprintf(stderr, "errno = %d\n", errno);
	}
	fflush(stderr);

	if (type & ERR_FATAL) {
		exit(-1);
		}
}

void set_fog(int enable)
{
    fog = enable;
    if (fog) {
        glEnable(GL_FOG);
        if (rgbmode)
            glClearColor(fog_params[0], fog_params[1], fog_params[2], 1.0);
        else {
            glClearIndex(sky_base + FOG_LEVELS - 1);
        }
    } else {
        glDisable(GL_FOG);
        if (rgbmode)
            glClearColor(0.23, 0.35, 0.78, 1.0);
        else {
            glClearIndex(sky_base);
        }
    }
}

void set_dither(int enable)
{
    dither = enable;
    if (dither) {
        glEnable(GL_DITHER);
    } else {
        glDisable(GL_DITHER);
    }
}

