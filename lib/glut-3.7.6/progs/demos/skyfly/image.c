#include "stdio.h"
#include "stdlib.h"
#include "string.h"

unsigned char * read_bwimage(char *name, int *w, int *h)
{
    unsigned char   *image;
    FILE            *image_in;
    int             components;

    if ( (image_in = fopen(name, "rb")) == NULL) { 
        return 0;
    }

    if (strncmp("terrain", name, 7) == 0) {
        *w = 256;
        *h = 256;
    } else if (strncmp("clouds", name, 6) == 0) {
        *w = 128;
        *h = 128;
    }
    components = 1;

    if (components != 1)
        return 0;

    image = (unsigned char *)malloc(sizeof(unsigned char) * *w * *h);

    fread(image, sizeof image[0], *w * *h, image_in);
    fclose(image_in);    
    return image;
}

