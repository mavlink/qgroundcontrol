/* 
 *   sgiflag.h
 *
 */

/* useful for lmdef, nurbssurface, nurbscurve, and more */
#define ELEMENTS(x)    (sizeof(x)/sizeof(x[0]))

/* Define nurbs surface properties */
#define S_NUMPOINTS    4
#define S_ORDER        4   /* cubic, degree 3 */
#define S_NUMKNOTS    (S_NUMPOINTS + S_ORDER)
#define S_NUMCOORDS    3

#define T_NUMPOINTS 4
#define T_ORDER        4 
#define T_NUMKNOTS    (T_NUMPOINTS + T_ORDER)
#define T_NUMCOORDS    3

typedef GLfloat Knot;
typedef GLfloat Point[3];
typedef GLfloat TrimPoint[2];

/* Trimming curves are either piecewise linear or nurbscurve */
enum TrimType {PWL, CURVE};


/* A trimming curve is made up of one or more trimming pieces.
 * A trimming piece may be of PWL or CURVE. If a trim piece is PWL,
 * it has at least two trim points, with each trim point composing
 * the endpoints of the line segments. If a trim piece is CURVE, it
 * has four trim points defining the cubic bezier trim.
 */

#define MAX_PIECES 20

struct TrimPieceStruct {
    enum TrimType type;             /* type of the trim              */
    int points;                     /* # of points in the trim piece */
    TrimPoint point[MAX_PIECES];    /* pointer to first trim point   */
};
typedef struct TrimPieceStruct TrimPiece;

struct TrimCurveStruct {
    int pieces;
    TrimPiece *trim;
};
typedef struct TrimCurveStruct TrimCurve;


struct teststruct {
    int a, b, c[2];
};
typedef struct teststruct Test;

/* function prototypes */
static void interp(TrimPoint a, TrimPoint b, GLfloat d, TrimPoint result);
static void join_trims(TrimPiece *trim1, TrimPiece *trim2, GLfloat radius);
static void translate_trim(TrimPiece *trim, GLfloat tx, GLfloat ty);
static void scale_trim(TrimPiece *trim, GLfloat sx, GLfloat sy);
static void rotate_trim(TrimPiece *trim, GLfloat angle);
static void copy_path(TrimCurve *src, TrimCurve **dst);
static void init_trims(void);
static void initialize(void);
static void draw_nurb(GLboolean);
static void draw_hull(Point cpoints[S_NUMPOINTS][T_NUMPOINTS]);
static void dotrim(TrimCurve *curve);
