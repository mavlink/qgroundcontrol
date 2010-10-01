
/*
 * texture.c
 *
 * FUNCTION:
 * texture mapping hack
 *
 * HISTORY:
 * Created by Linas Vepstas April 1994
 */

#include <stdlib.h>
#include <math.h>
#include "texture.h"

Texture * current_texture = 0x0;

Texture * planet_texture = 0x0;
Texture * check_texture = 0x0;
Texture * barberpole_texture = 0x0;
Texture * wild_tooth_texture = 0x0;

/* ======================================================= */

#define TEXTURE_SIZE 256

Texture * create_planet_texture (void) {
   int i, j;
   Texture * tex;
   unsigned char * pixmap;

   pixmap = (unsigned char *) malloc (TEXTURE_SIZE*TEXTURE_SIZE*3*sizeof (unsigned char));

   for (i=0; i< TEXTURE_SIZE; i++) {
      for (j=0; j< TEXTURE_SIZE; j++) {

         int mi = i - TEXTURE_SIZE/2;
         int mj = j - TEXTURE_SIZE/2;

         pixmap [3*TEXTURE_SIZE*i + 3*j] = (100*mi*mi + 40*mj*mj) >> 8;
         pixmap [3*TEXTURE_SIZE*i + 3*j + 1] = (10*mi*mi + 4*mj*mj) ;
         pixmap [3*TEXTURE_SIZE*i + 3*j + 2] = (1000*mi*mi + 400*mj*mj) >> 16 ;

      }
   }

   tex = (Texture *) malloc (sizeof (Texture));
   tex -> size = TEXTURE_SIZE;
   tex -> pixmap = pixmap;

   return tex;
}

/* ======================================================= */

Texture * create_check_texture (void) {
   int i, j;
   Texture * tex;
   unsigned char * pixmap;

   pixmap = (unsigned char *) malloc (TEXTURE_SIZE*TEXTURE_SIZE*3*sizeof (unsigned char));

   for (i=0; i< TEXTURE_SIZE; i++) {
      for (j=0; j< TEXTURE_SIZE; j++) {

         pixmap [3*TEXTURE_SIZE*i + 3*j] = 
		255 * ( (((i)/32) %2) == (((j)/32) %2));
         pixmap [3*TEXTURE_SIZE*i + 3*j + 1] = 
		255 * ( (((i)/32) %2) == (((j)/32) %2));
         pixmap [3*TEXTURE_SIZE*i + 3*j + 2] = 
		255 * ( (((i)/32) %2) == (((j)/32) %2));
      }
   }

   tex = (Texture *) malloc (sizeof (Texture));
   tex -> size = TEXTURE_SIZE;
   tex -> pixmap = pixmap;

   return tex;
}

/* ======================================================= */

Texture * create_barberpole_texture (void) {
   int i, j;
   Texture * tex;
   unsigned char * pixmap;

   pixmap = (unsigned char *) malloc (TEXTURE_SIZE*TEXTURE_SIZE*3*sizeof (unsigned char));

   for (i=0; i< TEXTURE_SIZE; i++) {
      for (j=0; j< TEXTURE_SIZE; j++) {
         pixmap [3*TEXTURE_SIZE*i + 3*j] = 255 * (((i+j)/32) %2);
         pixmap [3*TEXTURE_SIZE*i + 3*j + 1] = 255 * (((i+j)/32) %2);
         pixmap [3*TEXTURE_SIZE*i + 3*j + 2] = 255 * (((i+j)/32) %2);

      }
   }

   tex = (Texture *) malloc (sizeof (Texture));
   tex -> size = TEXTURE_SIZE;
   tex -> pixmap = pixmap;

   return tex;
}

/* ======================================================= */

Texture * create_wild_tooth_texture (void) {
   int i, j;
   Texture * tex;
   unsigned char * pixmap;

   pixmap = (unsigned char *) malloc (TEXTURE_SIZE*TEXTURE_SIZE*3*sizeof (unsigned char));

   for (i=0; i< TEXTURE_SIZE; i++) {
      for (j=0; j< TEXTURE_SIZE; j++) {

         pixmap [3*TEXTURE_SIZE*i + 3*j] = 
         255 * ( (((i+j)/32) %2) == (((i-j)/32) %2));
         pixmap [3*TEXTURE_SIZE*i + 3*j + 1] = 
         255 * ( (((i+j)/32) %2) == (((i-j)/32) %2));
         pixmap [3*TEXTURE_SIZE*i + 3*j + 2] = 
         255 * ( (((i+j)/32) %2) == (((i-j)/32) %2));

      }
   }

   tex = (Texture *) malloc (sizeof (Texture));
   tex -> size = TEXTURE_SIZE;
   tex -> pixmap = pixmap;

   return tex;
}

/* ======================================================= */

void setup_textures (void) {

   planet_texture = create_planet_texture ();
   check_texture = create_check_texture ();
   barberpole_texture = create_barberpole_texture ();
   wild_tooth_texture = create_wild_tooth_texture ();

   current_texture = wild_tooth_texture;
   current_texture = check_texture;
}


/* ================== END OF FILE ========================= */
