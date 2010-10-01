/* Lifted from include/gltk.h */

#ifdef __cplusplus
extern "C" {
#endif

/*
** RGB Image Structure
*/

typedef struct _RGBImageRec {
    int sizeX, sizeY;
    unsigned char *data;
} RGBImageRec;

extern RGBImageRec *RGBImageLoad(char *);

#ifdef __cplusplus
}
#endif
