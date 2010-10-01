
/*
 * texture.h
 *
 * FUNCTION:
 * texture mapping hack
 *
 * HISTORY:
 * Created by Linas Vepstas April 1994
 */

typedef struct {
   int size;
   unsigned char * pixmap;
} Texture;

extern Texture * current_texture;

extern Texture * planet_texture;
extern Texture * check_texture;
extern Texture * barberpole_texture;
extern Texture * wild_tooth_texture;

extern void setup_textures (void);

/* ================== END OF FILE ========================= */
