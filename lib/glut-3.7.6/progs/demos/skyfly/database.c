
/*
 * database.c   $Revision: 1.2 $
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "skyfly.h"

#if defined(_WIN32)
#pragma warning (disable:4244)  /* disable bogus conversion warnings */
#pragma warning (disable:4305)  /* VC++ 5.0 version of above warning. */
#endif

#define cosf(a)		cos((float)a)
#define sinf(a)  	sin((float)a)
#define sqrtf(a)  	sqrt((float)a)
#define expf(a)  	exp((float)a)

static void create_terrain(void);
static void erode_terrain(void);
static void color_terrain(void);
static void init_cells(void);
static void put_cell(float *source, perfobj_t *pobj);
static void put_paper_plane(float *source, perfobj_t *pobj);
static void put_texture_bind(int bind, perfobj_t *pobj);

int clouds;
static float paper_plane_vertexes[] = {
/*Nx  Ny  Nz   Vx     Vy    Vz */
/* ----------------------------    Top view of plane, middle streached open  */
 0.2, 0., .98, -.10,    0,  .02,/* vertex #'s      4 (.48,0,-.06)            */
 0., 0., 1.,   -.36,  .20, -.04,/*                 .                         */
 0., 0., 1.,    .36,  .01,    0,/*                ...                        */
 0., 0.,-1.,   -.32,  .02,    0,/*                 .             +X          */
 0., 1., 0.,    .48,    0, -.06,/*               2 . 6,8          ^          */
 0., 1., 0.,   -.30,    0, -.12,/*               . . .            |          */
 0.,-1., 0.,    .36, -.01,    0,/*              .. . ..           |          */
 0.,-1., 0.,   -.32, -.02,    0,/*               . . .            |          */
 0., 0.,-1.,    .36, -.01,    0,/*             . . . . .  +Y<-----*          */
 0., 0.,-1.,   -.36, -.20, -.04,/*               . . .     for this picture  */
 -0.2, 0., .98,  -.10,  0,  .02,/*            .  . . .  .  coord system rot. */
 -0.2, 0., -.98, -.10,  0,  .02,/*               . . .     90 degrees        */
 0., 0., -1.,  -.36,  .20, -.04,/*           .   . . .   .                   */
 0., 0., -1.,   .36,  .01,    0,/*               . # .           # marks     */
 0., 0., 1.,   -.32,  .02,    0,/*          .    . . .    .   (0,0) origin   */
 0., -1., 0.,   .48,    0, -.06,/*               . . .         (z=0 at top   */
 0., -1., 0.,  -.30,    0, -.12,/*         .     0 . 10    .    of plane)    */
 0.,1., 0.,     .36, -.01,    0,/*             . . . . .                     */
 0.,1., 0.,    -.32, -.02,    0,/*        .  .   . . .   .  .                */
 0., 0.,1.,     .36, -.01,    0,/*         .     . . .     .                 */
 0., 0.,1.,    -.36, -.20, -.04,/*       1.......3.5.7.......9               */
 0.2, 0., -.98,  -.10,  0,  .02,/* (-.36,.2,-.04)                            */
};

#define SIZE    400

float *A;

void init_paper_planes(void)
{
    perfobj_t       *pobj;

    /* 
     * create various perf-objs for planes 
     */
    pobj = &(SharedData->paper_plane_obj);
    pobj->flags = SharedData->paper_plane_flags;
    pobj->vdata = (float *) SharedData->paper_plane_verts;
    put_paper_plane(paper_plane_vertexes, pobj);

    pobj = &(SharedData->paper_plane_start_obj);
    pobj->flags = SharedData->paper_plane_start_flags;
    *(pobj->flags) = PD_PAPER_PLANE_MODE;
    *(pobj->flags + 1) = PLANES_START;
    *(pobj->flags + 2) = PD_END;

    pobj = &(SharedData->paper_plane_2ndpass_obj);
    pobj->flags = SharedData->paper_plane_2ndpass_flags;
    *(pobj->flags) = PD_PAPER_PLANE_MODE;
    *(pobj->flags + 1) = PLANES_SECOND_PASS;
    *(pobj->flags + 2) = PD_END;

    pobj = &(SharedData->paper_plane_end_obj);
    pobj->flags = SharedData->paper_plane_end_flags;
    *(pobj->flags) = PD_PAPER_PLANE_MODE;
    *(pobj->flags + 1) = PLANES_END;
    *(pobj->flags + 2) = PD_END;
}


/*
 * create perfobj from static definition of plane geometry above
 */

static void put_paper_plane(float *source, perfobj_t *pobj)
{
    int             j;
    perfobj_vert_t *pdataptr =(perfobj_vert_t *) pobj->vdata;
    unsigned int  *flagsptr = pobj->flags;
    float          *sp = source;

    *flagsptr++ = PD_DRAW_PAPER_PLANE;

    for (j = 0; j < 22; j++) {
        putn3fdata(sp + 0, pdataptr);
        putv3fdata(sp + 3, pdataptr);

        sp += 6;
        pdataptr++;
    }
    *flagsptr++ = PD_END;

}

static void put_texture_bind(int bind, perfobj_t *pobj)
{
	unsigned int  *flagsptr = pobj->flags;

	*flagsptr++ = PD_TEXTURE_BIND;
	*flagsptr++ = bind;

	*flagsptr++ = PD_END;

}

static void put_clouds_vert(float s, float t, float x, float y, float z,
				perfobj_vert_t *pdataptr)
{
	float           D[5];
	D[0] = s;
	D[1] = t;
	D[2] = x;
	D[3] = y;
	D[4] = z;
	putt2fdata(D, pdataptr);
	putv3fdata(D + 2, pdataptr);
}

float S[SIZE][SIZE];
float T[SIZE][SIZE];
float C[SIZE][SIZE][3];
int   M[SIZE][SIZE];

void init_terrain(void)
{
	GridDim = CellDim * NumCells;
	XYScale = (float)GRID_RANGE / (float)GridDim;
	CellSize = (float)GRID_RANGE / (float)NumCells;

	create_terrain();
	erode_terrain();
	color_terrain();
	init_cells();
}

#define SKY                             50.

void init_clouds(void)
{
    perfobj_t *pobj;
    perfobj_vert_t *pdataptr;

	clouds = 0;
    pobj = &(SharedData->clouds_texture_obj);
    pobj->flags = SharedData->clouds_texture_flags;
    put_texture_bind(2,pobj);

    pobj = &(SharedData->clouds_obj);
    pobj->flags = SharedData->clouds_flags;
    pobj->vdata = (float *)SharedData->clouds_verts;
    *(pobj->flags+ 0) = PD_DRAW_CLOUDS;
    *(pobj->flags+ 1) = PD_END;

    pdataptr =(perfobj_vert_t *) pobj->vdata;

    put_clouds_vert(0.,0., -SKY, -SKY, SKY_HIGH,pdataptr);
    pdataptr++;
    put_clouds_vert(24.,0.,  SKY+GRID_RANGE, -SKY, SKY_HIGH,pdataptr);
    pdataptr++;
    put_clouds_vert(24.,24.,  SKY+GRID_RANGE,  SKY+GRID_RANGE, SKY_HIGH,pdataptr);
    pdataptr++;
    put_clouds_vert(0.,24., -SKY,  SKY+GRID_RANGE, SKY_HIGH,pdataptr);
}

static void create_terrain(void)
{
	int             r, c, i, x1, y1, x2, y2;
	int             hillsize;

    hillsize = GRID_RANGE / 12;

    A = (float*)calloc(GridDim * GridDim, sizeof(float));

    /*
     *  initialize elevation to zero, except band down middle
     *  where make a maximum height 'hill' that will later be
     *  inverted to make the negative elevation 'canyon'
     */

    for (r = 0; r < GridDim; r++)
        for (c = 0; c < GridDim; c++)
             if(r>=(GridDim/2-2-IRND(2)) && r<=(GridDim/2+2+IRND(2)))
                A[r * GridDim + c] = 1.0;
             else
                A[r * GridDim + c] = 0.0;

    /*
     * create random sinusoidal hills that add on top
     * of each other
     */
    for (i = 1; i <= 10*GridDim; i++) {

        /* randomly position hill */
        x1 = IRND(GridDim - hillsize);
        x2 = x1 + hillsize/ 8 + IRND(hillsize-hillsize/ 8);
        y1 = IRND(GridDim - hillsize);
        y2 = y1 + hillsize/ 8 + IRND(hillsize-hillsize/ 8);

        if((x1<=GridDim/2-4 && x2>=GridDim/2-4) || 
           (x1<=GridDim/2+4 && x2>=GridDim/2+4))
        {
            x1 = IRND(2)-2 + GridDim/2;
            x2 = x1 + IRND(GridDim/2 - x1 + 2);
        }

        /* make a sinusoidal hill */
        for (r = x1; r < x2; r++)
            for (c = y1; c < y2; c++) {
                A[r * GridDim + c] +=.35 *
                    (sinf(M_PI * (float) (r - x1) / (float) (x2 - x1)) *
					 (sinf(M_PI * (float) (c - y1) / (float) (y2 - y1))));
            }
    }

    /* clamp the elevation of the terrain */
	for (r = 1; r < GridDim; r++)
		for (c = 1; c < GridDim; c++) {
			A[r * GridDim + c] = MIN(A[r * GridDim + c], .95);
			A[r * GridDim + c] = MAX(A[r * GridDim + c], 0.);
		}

}

#define NUM_DROPS                       80

static void erode_terrain(void)
{
    float           x, y, xv, yv, dx, dy;
    float           cut, min, take;
    int             nm;
    int             t, xi, yi, xo, yo, done;
    int             ii, jj, r, c;

    for (nm = 1; nm < NUM_DROPS*GridDim; nm++) {

        /* find a random position to start the 'rain drop' */
        x = (float) (IRND(GridDim));
        y = (float) (IRND(GridDim));

        /* Clamp x and y to be inside grid */
        x = MIN(MAX(2., x), (float)GridDim-2.);
        y = MIN(MAX(2., y), (float)GridDim-2.);
        
        done = 0;
        yv = xv = 0.;

        t = 0;
        cut = .3;

        while (!done) {
            xi = (int) x;
            yi = (int) y;

            min = 90.;

			if (xi != xo || yi != yo) {
                cut *=.99;

                /* gradient */
                dx = (A[(xi + 1)*GridDim + yi] - A[(xi - 1) * GridDim + yi]);
                dy = (A[xi * GridDim + yi + 1] - A[xi * GridDim + yi - 1]);


                /* find lowest neighbor */
                for (ii = -1; ii <= 1; ii++)
                    for (jj = -1; jj <= 1; jj++)
                        if (A[(xi + ii)*GridDim + yi + jj] < min)
                            min = A[(xi + ii)*GridDim + yi + jj];

                /* evaporate drop if sitting on my old location */
                if (M[xi][yi] == nm)
					done = 1;
                M[xi][yi] = nm;

                /* cave in neighbors by .3  */
                for (ii = -1; ii <= 1; ii++)
                    for (jj = -1; jj <= 1; jj++) {
                        take =.3 * cut * (A[(xi + ii)*GridDim + yi + jj]-min);
                        A[(xi + ii)*GridDim + yi + jj] -= take;
                    }

                /* take away from this cell by .7 */
                take = (A[xi*GridDim + yi] - min) *.7 * cut;
                A[xi*GridDim + yi] -= take;

            }
            xo = xi;
            yo = yi;

            /* move drop using kinematic motion*/
            xv = xv - dx -.8 * xv;
            yv = yv - dy -.8 * yv;

            x += xv;
            y += yv;

            /*
             * make sure can't move by more that 1.0
             * in any direction 
             */

            xv = MAX(xv, -1);
            yv = MAX(yv, -1);
            xv = MIN(xv,  1);
            yv = MIN(yv,  1);

            /* check to see if need a new drop */
            /* ie ran of world, got stuck, or at 'sea level' */
            if (x < 1.|| x > GridDim - 1.|| y < 1.|| y > GridDim - 1.
                                                || t++ > 2000
                || cut <.01)
                done = 1;

            if (A[xi*GridDim + yi] < 0.0001) {
                A[xi*GridDim + yi] = 0.;
                done = 1;
            }
		}                       /* while (!done) with this drop */
	}                           /* next drop */

	/*
	 *  invert the pseudo hill int the pseudo canyon
	 */

	for (r = 0; r < GridDim; r++)
		for (c = 0; c < GridDim; c++)
			 if(r>=GridDim/2-4 && r<=GridDim/2+4)
				A[r * GridDim + c] = MAX((-3.2 * A[r * GridDim + c]), -1.8);
}

static void color_terrain(void)
{
    float           N[3], D, alt, maxelev = -1.;
    int             x, y;

    for (x = 0; x < GridDim; x++)
        for (y = 0; y < GridDim; y++)
            maxelev = MAX(maxelev, A[x * GridDim + y]);

    for (x = 1; x < GridDim - 1; x++)
        for (y = 1; y < GridDim - 1; y++) {
            alt = A[x * GridDim + y] * 1.5;
            /* randomly perterb to get a mottling effect */
            alt += IRND(100) / 400. -.125;
            alt = MIN(alt, 1.0);
            if (alt < -.11) {
                C[x][y][0] = 0.6;         /* soil/rock in canyon */
                C[x][y][1] = 0.5;
                C[x][y][2] = 0.2;
            } else if (alt < .000001) {
                C[x][y][0] = 0.0;         /* dark, jungle lowlands */
                C[x][y][1] = 0.2;
                C[x][y][2] = 0.05;
            } else if (alt <.90) {
                C[x][y][0] = alt*.25;     /* green to redish hillsides */
                C[x][y][1] = (1.0 - alt) *.4 + .1;
                C[x][y][2] = 0.1;
            } else {
                C[x][y][0] = alt;
                C[x][y][1] = alt;         /* incresingly white snow */
                C[x][y][2] = alt;
            }

            /* compute normal to terrain */

            N[0] = A[(x - 1)*GridDim + y] - A[(x + 1)*GridDim + y];
            N[1] = A[x*GridDim + y - 1] - A[x*GridDim + y + 1];
            N[2] = 2.0 / ScaleZ;

            D = 1.0 / sqrtf(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);

            N[0] *= D;
            N[1] *= D;
            N[2] *= D;

            /* perform diffuse lighting of terrain */

            D = N[0] * LX + N[1] * LY + N[2] * LZ;
            D *= 1.2;

            if(!IRND(4)) D *= .5;
        
            D = MAX(D,0);

            /* darken terrain on shaded side */
            C[x][y][0] *= D;
            C[x][y][1] *= D;
            C[x][y][2] *= D;

            S[x][y] = (float) (x) / (float)CellDim;
            T[x][y] = (float) (y) / (float)CellDim;

        }
}

/*
 * create perobj to hold a cell of terrain
 * for a 5x5 terrain cell, there will be 5
 * tmeshes each with 10 triangles (12 vertexes)
 * this means looking past my cell to neighbors
 * to stitch things together.  To keep data contigious,
 * vertexes are repeated in tmeshes, not shared.
 */

static void init_cells(void)
{
    int             x, y, xx, yy, world_x, world_y;
    int             pntr, index, sstart, tstart;
    float           *D;
    perfobj_t       *pobj;

    D = (float*)calloc(CellDim *(CellDim + 1) * 
                        2 * FLOATS_PER_VERT_PAIR, sizeof(float));

    pobj = &(SharedData->terrain_texture_obj);
    pobj->flags = SharedData->terrain_texture_flags;

    put_texture_bind(1, pobj);

    for (x = 0; x < NumCells; x++)
        for (y = 0; y < NumCells; y++) {
			index = x*NumCells+y;
            SharedData->terrain_cells[index].flags = 
                SharedData->terrain_cell_flags[index];

            SharedData->terrain_cells[index].vdata =
                (float *) SharedData->terrain_cell_verts[index];

            pntr = 0;

            /*
             * Guarantee S,T to be within range 0-8 for tmesh strip using
             * 256x256 sized texture. This avoids texture index clipping
             * in pipe. 
            */
            sstart = (int)S[x*CellDim][y*CellDim];
            tstart = (int)T[x*CellDim][y*CellDim];

            for (xx = 0; xx < CellDim; xx++)
                for (yy = 0; yy < CellDim + 1; yy++) {

                    /* init a perfobj */

                    world_x = MIN(x * CellDim + xx, GridDim-2);
                    world_y = MIN(y * CellDim + yy, GridDim-2);

                    D[pntr + 0] = S[world_x][world_y] - sstart;
                    D[pntr + 1] = T[world_x][world_y] - tstart;
                    D[pntr + 2] = C[world_x][world_y][0];
                    D[pntr + 3] = C[world_x][world_y][1];
					D[pntr + 4] = C[world_x][world_y][2];
                    D[pntr + 5] = (float)world_x * XYScale;
                    D[pntr + 6] = (float)world_y * XYScale;
					D[pntr + 7] = A[world_x*GridDim + world_y]*ScaleZ;


                    D[pntr + 8] = S[world_x + 1][world_y] - sstart;
                    D[pntr + 9] = T[world_x + 1][world_y] - tstart;
                    D[pntr + 10] = C[world_x + 1][world_y][0];
                    D[pntr + 11] = C[world_x + 1][world_y][1];
                    D[pntr + 12] = C[world_x + 1][world_y][2];
                    D[pntr + 13] = (float)(world_x+1) * XYScale;
                    D[pntr + 14] = (float)world_y * XYScale;
                    D[pntr + 15] = A[(world_x+1)*GridDim + world_y] * ScaleZ;

                    pntr += FLOATS_PER_VERT_PAIR;

                }               /* for each cell */
            put_cell(D, &(SharedData->terrain_cells[index]));

        }                       /* for all cells in world */

        free(D);
}

static void put_cell(float *source, perfobj_t *pobj)
{
	int             i, j;
	perfobj_vert_t *pdataptr =(perfobj_vert_t *) pobj->vdata;
	unsigned int  *flagsptr = pobj->flags;
	float          *sp = source;

	/* For all tmesh strips in cell */
	for (i = 0; i < CellDim; i++) {
		*flagsptr++ = PD_DRAW_TERRAIN_CELL;
		*flagsptr++ = (unsigned long) pdataptr;

		/* For all verts in tmesh strip */
		for (j = 0; j < (CellDim + 1) * 2; j++) {
			putt2fdata(sp, pdataptr);
			putc3fdata(sp + 2, pdataptr);
            putv3fdata(sp + 5, pdataptr);

            sp += 8;
            pdataptr++;
        }
    }
    *flagsptr++ = PD_END;
}

