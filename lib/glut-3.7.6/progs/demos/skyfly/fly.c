
/*
 * fly.c        $Revision: 1.2 $
 */

#include "stdio.h"
#include "math.h"
#include "skyfly.h"

#if defined(_WIN32)
#pragma warning (disable:4244)  /* disable bogus conversion warnings */
#pragma warning (disable:4305)  /* VC++ 5.0 version of above warning. */
#endif

#define cosf(a)  	cos((float)a)
#define sinf(a)  	sin((float)a)
#define sqrtf(a)  	sqrt((float)a)
#define expf(a)  	exp((float)a)

typedef struct paper_plane_struct {
    float           Pturn_rate;
    float           PX, PY, PZ, PZv;
    float           Pazimuth;
    float           Proll;
    int             Pcount, Pdirection;
} paper_plane;

static paper_plane  flock[NUM_PLANES];
static float  X, Y, Z, Azimuth, Speed;
static int Keyboard_mode;

extern float *A;
extern int Wxorg, Wyorg;

static float terrain_height(float x, float y);

int Xgetbutton(int b);
int Xgetvaluator(int v);

void init_positions(void)
{
    int             i;

    X = GRID_RANGE / 2.;
    Y = GRID_RANGE / 2.;
    Z = 1.5;

    /*
     * randomly position the planes near center of world
     * take MAX of height above terrain and 0, so planes
     * don't fall into canyon.  Canyon has negative elevation
     */

    for (i = 0; i < NUM_PLANES; i++) {
        flock[i].PX = (float) IRND(20) + GRID_RANGE / 2 - 10;
        flock[i].PY = (float) IRND(20) + GRID_RANGE / 2 - 10;
        flock[i].PZ = MAX(terrain_height(flock[i].PX, flock[i].PY),0.) +
                2.*(float)i/NUM_PLANES+.3;
        flock[i].Pazimuth = ((float)IRND(256) / 128.) * M_PI;
    }
	Speed = 0.1f;
	Azimuth = M_PI / 2.;

#if 0
//	if (Init_pos) {
//		X = Init_x;
//		Y = Init_y;
//		Z = Init_z;
//		Azimuth = Init_azimuth;
//		Keyboard_mode = 1;
//    }
#endif
}

int _frame = 0;

void fly(perfobj_t *viewer_pos)
{
	float       terrain_z, xpos, ypos, xcntr, ycntr;
	float       delta_speed = .003;

/*	if (++_frame == 1000) {
		_frame = 0;
		init_positions();
		}*/

    xcntr = Wxsize / 2;
    ycntr = Wysize / 2;

    if (Xgetbutton(RKEY))
        init_positions();

    if (Xgetbutton(SPACEKEY)) {
        Keyboard_mode = !Keyboard_mode;
    }

	if (Keyboard_mode) {

        /*
         * step-at-a-time debugging mode
         */

        if (Keyboard_mode && Xgetbutton(LEFTARROWKEY)) {
			Azimuth -= 0.025;
		}
		if (Keyboard_mode && Xgetbutton(RIGHTARROWKEY)) {
			Azimuth += 0.025;
		}
		if (Keyboard_mode && Xgetbutton(UPARROWKEY)) {
			X += cosf(-Azimuth + M_PI / 2.) * 0.025;
			Y += sinf(-Azimuth + M_PI / 2.) * 0.025;
		}
		if (Keyboard_mode && Xgetbutton(DOWNARROWKEY)) {
			X -= cosf(-Azimuth + M_PI / 2.) * 0.025;
			Y -= sinf(-Azimuth + M_PI / 2.) * 0.025;
		}
		if (Keyboard_mode && Xgetbutton(PAGEUPKEY)) {
			Z += 0.025;
        }
        if (Keyboard_mode && Xgetbutton(PAGEDOWNKEY)) {
            Z -= 0.025;
        }

    } else {

        /*
         * simple, mouse-driven flight model
         */

        if (Xgetbutton(LEFTMOUSE) && Speed < .3)
            Speed += delta_speed;
        if (Xgetbutton(RIGHTMOUSE) && Speed > -.3)
            Speed -= delta_speed;
        if (Xgetbutton(MIDDLEMOUSE))
            Speed = Speed*.8;

        xpos = (Xgetvaluator(MOUSEX)-xcntr) / ((float)Wxsize*14.);
        ypos = (Xgetvaluator(MOUSEY)-ycntr) / ((float)Wysize*.5);

        /*
         * move in direction of view
         */

        Azimuth += xpos;
        X += cosf(-Azimuth + M_PI / 2.) * Speed;
        Y += sinf(-Azimuth + M_PI / 2.) * Speed;
        Z -= ypos * Speed;
    }

    /*
     * keep from getting too close to terrain
     */

    terrain_z = terrain_height(X, Y);
    if (Z < terrain_z +.4)
        Z = terrain_z +.4;

    X = MAX(X, 1.);
    X = MIN(X, GRID_RANGE);
    Y = MAX(Y, 1.);
    Y = MIN(Y, GRID_RANGE);
    Z = MIN(Z, 20.);

    *((float *) viewer_pos->vdata + 0) = X;
    *((float *) viewer_pos->vdata + 1) = Y;
    *((float *) viewer_pos->vdata + 2) = Z;
    *((float *) viewer_pos->vdata + 3) = Azimuth;
}

void fly_paper_planes(perfobj_t *paper_plane_pos)
{
	int                 i;
	float               speed = .08;
	float               terrain_z;

	/*
	 * slow planes down in cyclops mode since
     * frame rate is doubled 
     */

    for (i = 0; i < NUM_PLANES; i++) {
        /*
         * If plane is not turning, one chance in 50 of
         * starting a turn
         */
        if (flock[i].Pcount == 0 && IRND(50) == 1) {
            /* initiate a roll */
            /* roll for a random period */
            flock[i].Pcount = IRND(100);
            /* random turn rate */
            flock[i].Pturn_rate = IRND(100) / 10000.;
            flock[i].Pdirection = IRND(3) - 1;
        }
        if (flock[i].Pcount > 0) {
            /* continue rolling */
            flock[i].Proll += flock[i].Pdirection * flock[i].Pturn_rate;
            flock[i].Pcount--;
        } else
            /* damp amount of roll when turn complete */
            flock[i].Proll *=.95;

        /* turn as a function of roll */
        flock[i].Pazimuth -= flock[i].Proll *.05;

        /* follow terrain elevation */
        terrain_z=terrain_height(flock[i].PX,flock[i].PY);

        /* use a "spring-mass-damp" system of terrain follow */
        flock[i].PZv = flock[i].PZv - 
            .01 * (flock[i].PZ - (MAX(terrain_z,0.) +
                         2.*(float)i/NUM_PLANES+.3)) - flock[i].PZv *.04;

        /* U-turn if fly off world!! */
        if (flock[i].PX < 1 || flock[i].PX > GRID_RANGE - 2 || flock[i].PY < 1 || flock[i].PY > GRID_RANGE - 2)
            flock[i].Pazimuth += M_PI;

        /* move planes */
        flock[i].PX += cosf(flock[i].Pazimuth) * speed;
        flock[i].PY += sinf(flock[i].Pazimuth) * speed;
        flock[i].PZ += flock[i].PZv;

    }

	for (i = 0; i < NUM_PLANES; i++) {
		*((float *) paper_plane_pos[i].vdata + 0) = flock[i].PX;
		*((float *) paper_plane_pos[i].vdata + 1) = flock[i].PY;
		*((float *) paper_plane_pos[i].vdata + 2) = flock[i].PZ;
		*((float *) paper_plane_pos[i].vdata + 3) = flock[i].Pazimuth * RAD_TO_DEG;
		*((float *) paper_plane_pos[i].vdata + 4) = flock[i].PZv * (-500.);
		*((float *) paper_plane_pos[i].vdata + 5) = flock[i].Proll *RAD_TO_DEG;
	}
}

/* compute height above terrain */
static float terrain_height(float x, float y)
{

    float           dx, dy;
    float           z00, z01, z10, z11;
    float           dzx1, dzx2, z1, z2;
    int             xi, yi;

    x /= XYScale;
    y /= XYScale;
    xi = MIN((int)x, GridDim-2);
    yi = MIN((int)y, GridDim-2);
    dx = x - xi;
    dy = y - yi;

    /*
                View looking straight down onto terrain

                        <--dx-->

                        z00----z1-------z10  (terrain elevations)
                         |     |         |   
                      ^  |     Z at(x,y) |   
                      |  |     |         |   
					 dy  |     |         |
                      |  |     |         |   
					  |  |     |         |
                      V z00----z2-------z10
                      (xi,yi)

                        Z= height returned
                        
    */

    z00 = A[xi * GridDim + yi];
    z10 = A[(xi + 1) * GridDim + yi];
    z01 = A[xi * GridDim + yi + 1];
    z11 = A[(xi + 1) * GridDim + yi + 1];

    dzx1 = z10 - z00;
    dzx2 = z11 - z01;

    z1 = z00 + dzx1 * dx;
    z2 = z01 + dzx2 * dx;

    return (ScaleZ*((1.0 - dy) * z1 + dy * z2));
}

