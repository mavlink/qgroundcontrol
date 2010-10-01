
/*
 * This file contains routines to support the SGI compatible quad-mesh
 * primitve.
 *
 * Written By Linas Vepstas November 1991 
 */

#include <stdlib.h>

struct _emu_qmesh_vertex_pair {
   float ca[3];
   float na[3];
   float va[4];

   float cb[3];
   float nb[3];
   float vb[4];
   };

#define QMESH 6
static int bgnmode = 0;

struct _emu_qmesh {
   int num_vert;
   struct _emu_qmesh_vertex_pair paira;
   struct _emu_qmesh_vertex_pair pairb;
   struct _emu_qmesh_vertex_pair *first_pair;
   struct _emu_qmesh_vertex_pair *second_pair;
   float defer_color[3];
   float defer_normal[3];
}  * _emu_qmesh_GC;


#define COPY_THREE_WORDS(A,B) {						\
	struct three_words { long a, b, c; };				\
	*(struct three_words *) (A) = *(struct three_words *) (B);	\
}

#define COPY_FOUR_WORDS(A,B) {						\
	struct four_words { long a, b, c, d; };				\
	*(struct four_words *) (A) = *(struct four_words *) (B);	\
}

/* ================================================================= */

void _emu_qmesh_InitGC (struct _emu_qmesh * tmp)
{

   tmp -> num_vert = 0;
   tmp -> first_pair = & (tmp ->  paira);
   tmp -> second_pair = & (tmp ->  pairb);

   tmp -> defer_color[0] = 0.0;
   tmp -> defer_color[1] = 0.0;
   tmp -> defer_color[2] = 0.0;

   tmp -> defer_normal[0] = 0.0;
   tmp -> defer_normal[1] = 0.0;
   tmp -> defer_normal[2] = 0.0;

}

/* ================================================================= */

struct _emu_qmesh * _emu_qmesh_CreateGC (void) 
{
   struct _emu_qmesh * tmp;

   tmp = (struct _emu_qmesh *) malloc (sizeof (struct _emu_qmesh));
   _emu_qmesh_InitGC (tmp);

   return (tmp);
}

/* ================================================================= */

void _emu_qmesh_DestroyGC (void) 
{
   free (_emu_qmesh_GC);
}

/* ================================================================= */

void _emu_qmesh_bgnqmesh (void) 
{
   _emu_qmesh_GC = _emu_qmesh_CreateGC ();
   bgnmode = QMESH;
}

/* ================================================================= */

void _emu_qmesh_endqmesh (void)
{
   _emu_qmesh_DestroyGC ();
  bgnmode = 0;
}

/* ================================================================= */

void _emu_qmesh_c3f (float c[3])  
{
   if (bgnmode == QMESH) {
      COPY_THREE_WORDS (_emu_qmesh_GC -> defer_color, c); 
   } else {
      c3f (c);
   }
}

/* ================================================================= */

void _emu_qmesh_n3f (float n[3])  
{
   if (bgnmode == QMESH) {
      COPY_THREE_WORDS (_emu_qmesh_GC -> defer_normal, n); 
   } else {
      n3f (n);
   }
}

/* ================================================================= */

void _emu_qmesh_v3f (float v[3])  
{
   int nv, even_odd, fs;
   struct _emu_qmesh_vertex_pair *tmp;

   if (bgnmode == QMESH) {
      nv = _emu_qmesh_GC -> num_vert;
      even_odd = nv %2;
      fs = (nv %4) / 2;
   
      if (fs) {
         if (even_odd) {
            COPY_THREE_WORDS (_emu_qmesh_GC -> pairb.cb, 
                              _emu_qmesh_GC -> defer_color); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> pairb.nb, 
                              _emu_qmesh_GC -> defer_normal); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> pairb.vb, v); 
            _emu_qmesh_GC -> pairb.vb [3] = 1.0;
         } else {
            COPY_THREE_WORDS (_emu_qmesh_GC -> pairb.ca, 
                              _emu_qmesh_GC -> defer_color); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> pairb.na, 
                              _emu_qmesh_GC -> defer_normal); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> pairb.va, v); 
            _emu_qmesh_GC -> pairb.va [3] = 1.0;
         }
      } else {
         if (even_odd) {
            COPY_THREE_WORDS (_emu_qmesh_GC -> paira.cb, 
                              _emu_qmesh_GC -> defer_color); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> paira.nb, 
                              _emu_qmesh_GC -> defer_normal); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> paira.vb, v); 
            _emu_qmesh_GC -> paira.vb [3] = 1.0;
         } else {
            COPY_THREE_WORDS (_emu_qmesh_GC -> paira.ca, 
                              _emu_qmesh_GC -> defer_color); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> paira.na, 
                              _emu_qmesh_GC -> defer_normal); 
            COPY_THREE_WORDS (_emu_qmesh_GC -> paira.va, v); 
            _emu_qmesh_GC -> paira.va [3] = 1.0;
         }
      }
      
      if (even_odd && (nv >= 3)) {
         bgnpolygon ();
         c3f ( _emu_qmesh_GC -> first_pair -> ca);
         n3f ( _emu_qmesh_GC -> first_pair -> na);
         v4f ( _emu_qmesh_GC -> first_pair -> va);
         c3f ( _emu_qmesh_GC -> first_pair -> cb);
         n3f ( _emu_qmesh_GC -> first_pair -> nb);
         v4f ( _emu_qmesh_GC -> first_pair -> vb);
         c3f ( _emu_qmesh_GC -> second_pair -> cb);
         n3f ( _emu_qmesh_GC -> second_pair -> nb);
         v4f ( _emu_qmesh_GC -> second_pair -> vb);
         c3f ( _emu_qmesh_GC -> second_pair -> ca);
         n3f ( _emu_qmesh_GC -> second_pair -> na);
         v4f ( _emu_qmesh_GC -> second_pair -> va);
         endpolygon ();
   
         /* swap the data buffers */
         tmp = _emu_qmesh_GC -> first_pair;
         _emu_qmesh_GC -> first_pair = _emu_qmesh_GC -> second_pair;
         _emu_qmesh_GC -> second_pair = tmp;
      }
   
      _emu_qmesh_GC -> num_vert ++;

   } else {
      v3f (v);
   }
}

/* ================================================================= */
