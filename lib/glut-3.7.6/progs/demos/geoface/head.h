typedef struct TAG {

  int poly           ;      /* an index to a tagged polygon               */
  int vert           ;      /* an index to the tagged vertex              */

} TAG ;

typedef struct EXPRESSION {

  char  name[80]       ;    /* name of the expression                     */
  float m[20]          ;    /* an expression vector                       */
  float bias           ;    /* an bias control for the muscles            */

} EXPRESSION ;


typedef struct MUSCLE {

  int   active         ;     /* activity switch for the muscle            */
  float head[3]        ;     /* head of the muscle vector                 */
  float tail[3]        ;     /* tail of the muscle vector                 */
  float zone,                /* zone of influence                         */
        fs, fe, mval   ;     /* zone, start, end, contraction             */
  char  name[80]       ;     /* name of the muscle                        */
  float clampv         ;     /* clamping value                            */
  float mstat	       ;     /* current contraction value                 */
  
} MUSCLE ;


typedef struct  VERTEX {

  float    xyz[3]      ;     /* x,y,z of the vertex (modified)            */
  float    nxyz[3]     ;     /* x,y,z of the vertex (never modified)      */
  int      np          ;     /* number of polygons associated with node   */
  int      plist[30]   ;     /* list of polygons associated with node     */
  float    norm[3]     ;     /* polygon vertex normal                     */

} VERTEX ;


typedef struct  POLYGON {

  VERTEX  *vertex[3]   ;     /* pointer to an array of three vertices     */

} POLYGON ;


typedef struct  HEAD {

  int       npindices      ;  /* number of polygon indices                 */
  int      *indexlist      ;  /* integer index list of size npindices*4    */

  int       npolylinenodes ;  /* number of nodes in the poly line          */
  float    *polyline       ;  /* xyz nodes in the poly line                */

  int       npolygons      ;  /* total number of polygons                  */
  POLYGON **polygon        ;  /* pointer to the polygon list               */

  int       neyelidtags    ;  /* number of eyelid tags                     */
  TAG     **eyelidtag      ;  /* pointer to the eyelid tags                */
  float     eyelidang      ;  /* rotation of the eyelids                   */

  int       njawtags       ;  /* number of jaw tags                        */
  TAG     **jawtag         ;  /* pointer to the eyelid tags                */
  float     jawang         ;  /* rotation of the jaw                       */

  int       nmuscles       ;  /* number of muscles in the face             */
  MUSCLE  **muscle         ;  /* pointer to the muscle list                */

  int	    nexpressions   ;  /* number of expressions in the		   */
  EXPRESSION  **expression ;  /* point to an expression vector	           */

} HEAD ;

/* main.c								*/
extern int verbose;

/* make_face.c								*/
HEAD *create_face 	      	( char *, char *		       	) ;
void averaged_vertex_normals 	( HEAD *face, int p, 
			          float *n1, float *n2, float *n3       ) ; 
void face_reset ( HEAD *face );
void expressions ( HEAD *face, int e );
void data_struct ( HEAD *face );

/* display.c								*/
void paint_polyline     	( HEAD *face 				) ;
void paint_polygons     	( HEAD *face, int type, int normals     ) ;
void calculate_polygon_vertex_normal ( HEAD *face );
void paint_muscles ( HEAD *face );

/* muscle.c */
void activate_muscle (HEAD *face, float *vt, float *vh, float fstart,  float fin,  float ang,  float val);

/* fileio.c */
void read_polygon_indices ( char *FileName, HEAD *face );
void read_polygon_line ( char    *FileName , HEAD    *face );
void read_muscles ( char *FileName , HEAD *face );
void read_expression_vectors ( char    *FileName , HEAD    *face );

