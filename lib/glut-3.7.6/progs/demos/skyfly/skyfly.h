/*
 * skyfly.h     $Revision: 1.3 $
*/

#ifdef _WIN32
#pragma warning(disable : 4244)
#endif

/* buttons */
#define ESCKEY          0
#define RKEY            1
#define PERIODKEY       2
#define SPACEKEY        3

#define DOWNARROWKEY    4
#define UPARROWKEY      5
#define LEFTARROWKEY    6
#define RIGHTARROWKEY   7

#define PAGEDOWNKEY     8
#define PAGEUPKEY       9

#define LEFTMOUSE       10
#define MIDDLEMOUSE     11
#define RIGHTMOUSE      12
#define BUTCOUNT        13

/* valuators */
#define MOUSEX          0
#define MOUSEY          1
#define VALCOUNT        2

#ifdef _WIN32
#undef random
extern long random(void);
#else
/* random is defined in stdlib.h, return type varies depending on headers*/
#endif

#undef	MIN
#undef	MAX
#define MIN(a,b)        (((a)<(b)) ? (a) : (b))
#define MAX(a,b)        (((a)>(b)) ? (a) : (b))
#define RAD_TO_DEG                      (180/M_PI)
#define IRND(x) ((int)((float)(x) * ((float)random()/(float)0x7fffffff)))
#define M_PI            3.14159265358979323846

#define NUM_PLANES                      20
#define GRID_RANGE                      200             /* 200 kilometers */
#define FOV                             (M_PI / 4.)     /* 45 degrees */

/*
 * Light vector
*/
#define LX                              0.0     
#define LY                              0.707
#define LZ                              0.707

#define FLOATS_PER_VERT_PAIR            16

#define NBUFFERS    2

#define SKY_CYCLOPS             0   
#define SKY_DUALCHANNEL         1
#define SKY_SINGLECHANNEL       2

/*
 * perfobj flags
*/
#define PD_TEXTURE_BIND                         0
#define PD_DRAW_PAPER_PLANE                     1
#define PD_DRAW_TERRAIN_CELL                    2
#define PD_PAPER_PLANE_MODE                     3
#define PD_PAPER_PLANE_POS                      4
#define PD_VIEWER_POS                           5
#define PD_DRAW_CLOUDS                          6
#define PD_END                                  0x3fff

#define PLANES_START                            0
#define PLANES_SECOND_PASS                      1
#define PLANES_END                              2

/* 
 * Offsets to data in perfobj_vert_t
*/
#define PD_V_POINT                      0
#define PD_V_CPACK                      3
#define PD_V_NORMAL                     4
#define PD_V_COLOR                      8
#define PD_V_TEX                        12
#define PD_V_SIZE                       16

/*
 * Padding ensures that vertex data remains quad-word aligned within struct
*/
typedef struct perfobj_vert_t {
        float vert[3];
        unsigned long vpad;

        float normal[3];
        unsigned long npad;

        float color[3];
        unsigned long cpad;

        float texture[2];
        unsigned long tpad[2]; 
} perfobj_vert_t;

/*
 * A perfobj is a structure designed for fast rendering. Flags are separated
 * from vertex data to improve cacheing behavior. Typically the flags are
 * tokens which determine the drawing operation to perform and the vdata are
 * perfobj_vert_t's or other floating point data.
*/
typedef struct perfobj_t {
        unsigned int    *flags;
        float           *vdata;
} perfobj_t;

extern void drawperfobj( perfobj_t *perfptr );

extern void putv3fdata( float *source, perfobj_vert_t *vertptr );
extern void putn3fdata( float *source, perfobj_vert_t *vertptr );
extern void putc3fdata( float *source, perfobj_vert_t *vertptr );
extern void putt2fdata( float *source, perfobj_vert_t *vertptr );

/*
 * This is the structure which contains the database. It is amalloc'ed
 * in shared memory so that forked processes can access it. Notice how
 * the flags and vertex data are separated to improve cacheing behavior.
*/ 
typedef struct shared_data_struct {
        /* objects */
        perfobj_t       paper_plane_obj;
        perfobj_t       paper_plane_start_obj;
        perfobj_t       paper_plane_2ndpass_obj;
        perfobj_t       paper_plane_end_obj;
        perfobj_t       terrain_texture_obj;
        perfobj_t       *terrain_cells;
        perfobj_t       clouds_texture_obj;
        perfobj_t       clouds_obj;

        /* flags */
        unsigned int    paper_plane_flags[2];
        unsigned int    paper_plane_start_flags[3];
        unsigned int    paper_plane_2ndpass_flags[3];
        unsigned int    paper_plane_end_flags[3];
        unsigned int    terrain_texture_flags[3];
        unsigned int    **terrain_cell_flags;
        unsigned int    clouds_texture_flags[3];
        unsigned int    clouds_flags[2];

        /* data */
        perfobj_vert_t  paper_plane_verts[22];
        perfobj_vert_t  **terrain_cell_verts;
        perfobj_vert_t  clouds_verts[4];

} shared_data;

/*
 * See skyfly.c for comments
*/
extern shared_data      *SharedData;
extern float            ScaleZ;
extern int              CellDim;
extern int              NumCells;
extern int              GridDim;
extern float            FarCull;
extern float            XYScale, CellSize;
extern int              Wxsize, Wysize;

extern int              Init_pos;
extern float            Init_x, Init_y, Init_z, Init_azimuth;

extern float            far_cull;
extern int              rgbmode;
extern float            fog_params[4];
extern int              dither, fog;
extern int 				mipmap;

/* Color index ramp info */
extern int              sky_base, terr_base;
extern int              plane_colors[3];

#define SKY_BITS        4
#define SKY_COLORS      (1 << SKY_BITS)

#define TERR_BITS       4
#define TERR_COLORS     (1 << TERR_BITS)

#define PLANE_BITS      4
#define PLANE_COLORS    (1 << PLANE_BITS)

#define FOG_LEVELS      6

#define SKY_HIGH        5.0f

extern void set_fog(int enable);
extern void set_dither(int enable);

#define cosf(a)  	cos((float)a)
#define sinf(a)  	sin((float)a)
#define sqrtf(a)  	sqrt((float)a)
#define expf(a)  	exp((float)a)

