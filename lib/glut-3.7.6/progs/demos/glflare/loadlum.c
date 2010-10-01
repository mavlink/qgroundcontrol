
/* texture.c - by David Blythe, SGI */

/* load_luminace is a simplistic routine for reading an SGI .bw image file. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loadlum.h"

typedef struct _ImageRec {
  unsigned short imagic;
  unsigned short type;
  unsigned short dim;
  unsigned short xsize, ysize, zsize;
  unsigned int min, max;
  unsigned int wasteBytes;
  char name[80];
  unsigned long colorMap;
  FILE *file;
  unsigned char *tmp;
  unsigned long rleEnd;
  unsigned int *rowStart;
  int *rowSize;
} ImageRec;

static void
ConvertShort(unsigned short *array, unsigned int length)
{
  unsigned short b1, b2;
  unsigned char *ptr;

  ptr = (unsigned char *) array;
  while (length--) {
    b1 = *ptr++;
    b2 = *ptr++;
    *array++ = (b1 << 8) | (b2);
  }
}

static void
ConvertUint(unsigned *array, unsigned int length)
{
  unsigned int b1, b2, b3, b4;
  unsigned char *ptr;

  ptr = (unsigned char *) array;
  while (length--) {
    b1 = *ptr++;
    b2 = *ptr++;
    b3 = *ptr++;
    b4 = *ptr++;
    *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
  }
}

static ImageRec *
ImageOpen(char *fileName)
{
  union {
    int testWord;
    char testByte[4];
  } endianTest;
  ImageRec *image;
  int swapFlag, x;

  endianTest.testWord = 1;
  if (endianTest.testByte[0] == 1)
    swapFlag = 1;
  else
    swapFlag = 0;

  image = (ImageRec *) malloc(sizeof(ImageRec));
  if (image == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }
  if ((image->file = fopen(fileName, "rb")) == NULL) {
    perror(fileName);
    exit(1);
  }
  fread(image, 1, 12, image->file);

  if (swapFlag)
    ConvertShort(&image->imagic, 6);
  image->tmp = (unsigned char *) malloc(image->xsize * 256);
  if (image->tmp == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }
  if ((image->type & 0xFF00) == 0x0100) {
    x = image->ysize * image->zsize * (int) sizeof(unsigned);
    image->rowStart = (unsigned *) malloc(x);
    image->rowSize = (int *) malloc(x);
    if (image->rowStart == NULL || image->rowSize == NULL) {
      fprintf(stderr, "Out of memory!\n");
      exit(1);
    }
    image->rleEnd = 512 + (2 * x);
    fseek(image->file, 512, SEEK_SET);
    fread(image->rowStart, 1, x, image->file);
    fread(image->rowSize, 1, x, image->file);
    if (swapFlag) {
      ConvertUint(image->rowStart, x / (int) sizeof(unsigned));
      ConvertUint((unsigned *) image->rowSize, x / (int) sizeof(int));
    }
  }
  return image;
}

static void
ImageClose(ImageRec * image)
{
  fclose(image->file);
  free(image->tmp);
  free(image);
}

static void
ImageGetRow(ImageRec * image, unsigned char *buf, int y, int z)
{
  unsigned char *iPtr, *oPtr, pixel;
  int count;

  if ((image->type & 0xFF00) == 0x0100) {
    fseek(image->file, (long) image->rowStart[y + z * image->ysize], SEEK_SET);
    fread(image->tmp, 1, (unsigned int) image->rowSize[y + z * image->ysize],
      image->file);
    iPtr = image->tmp;
    oPtr = buf;
    for (;;) {
      pixel = *iPtr++;
      count = (int) (pixel & 0x7F);
      if (!count)
        return;
      if (pixel & 0x80) {
        while (count--)
          *oPtr++ = *iPtr++;
      } else {
        pixel = *iPtr++;
        while (count--)
          *oPtr++ = pixel;
      }
    }
  } else {
    fseek(image->file, 512 + (y * image->xsize) + (z * image->xsize * image->ysize),
      SEEK_SET);
    fread(buf, 1, image->xsize, image->file);
  }
}

unsigned char *
load_luminance(char *name, int *width, int *height, int *components)
{
  unsigned char *base, *lptr;
  ImageRec *image;
  int y;

  image = ImageOpen(name);

  if (!image)
    return NULL;
  if (image->zsize != 1)
    return NULL;

  *width = image->xsize;
  *height = image->ysize;
  *components = image->zsize;

  base = (unsigned char *)
    malloc(image->xsize * image->ysize * sizeof(unsigned char));
  if (!base)
    return NULL;
  lptr = base;
  for (y = 0; y < image->ysize; y++) {
    ImageGetRow(image, lptr, y, 0);
    lptr += image->xsize;
  }
  ImageClose(image);
  return base;
}
