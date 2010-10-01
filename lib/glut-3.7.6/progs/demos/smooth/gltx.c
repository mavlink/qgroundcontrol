/*
 *  Simple SGI .rgb (IRIS RGB) image file reader ripped off from
 *  texture.c (written by David Blythe).  See the SIGGRAPH '96
 *  Advanced OpenGL course notes.
 */


/* includes */
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <assert.h>
#include "gltx.h"


/* private typedefs */
typedef struct _rawImageRec {
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    unsigned long min, max;
    unsigned long wasteBytes;
    char name[80];
    unsigned long colorMap;
    FILE *file;
    unsigned char *tmp, *tmpR, *tmpG, *tmpB;
    unsigned long rleEnd;
    GLuint *rowStart;
    GLint *rowSize;
} rawImageRec;


/* private functions */
static void ConvertShort(unsigned short *array, unsigned int length)
{
    unsigned short b1, b2;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--) {
	b1 = *ptr++;
	b2 = *ptr++;
	*array++ = (b1 << 8) | (b2);
    }
}

static void ConvertLong(GLuint *array, unsigned int length)
{
    unsigned long b1, b2, b3, b4;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--) {
	b1 = *ptr++;
	b2 = *ptr++;
	b3 = *ptr++;
	b4 = *ptr++;
	*array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
    }
}

static rawImageRec *RawImageOpen(char *fileName)
{
    union {
	int testWord;
	char testByte[4];
    } endianTest;
    rawImageRec *raw;
    GLenum swapFlag;
    int x;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
	swapFlag = GL_TRUE;
    } else {
	swapFlag = GL_FALSE;
    }

    raw = (rawImageRec *)malloc(sizeof(rawImageRec));
    if (raw == NULL) {
	return NULL;
    }
    if ((raw->file = fopen(fileName, "rb")) == NULL) {
	return NULL;
    }

    fread(raw, 1, 12, raw->file);

    if (swapFlag) {
	ConvertShort(&raw->imagic, 6);
    }

    raw->tmp = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpR = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpG = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpB = (unsigned char *)malloc(raw->sizeX*256);
    if (raw->tmp == NULL || raw->tmpR == NULL || raw->tmpG == NULL ||
	raw->tmpB == NULL) {
	return NULL;
    }

    if ((raw->type & 0xFF00) == 0x0100) {
	x = raw->sizeY * raw->sizeZ * sizeof(GLuint);
	raw->rowStart = (GLuint *)malloc(x);
	raw->rowSize = (GLint *)malloc(x);
	if (raw->rowStart == NULL || raw->rowSize == NULL) {
	    return NULL;
	}
	raw->rleEnd = 512 + (2 * x);
	fseek(raw->file, 512, SEEK_SET);
	fread(raw->rowStart, 1, x, raw->file);
	fread(raw->rowSize, 1, x, raw->file);
	if (swapFlag) {
	    ConvertLong(raw->rowStart, x/sizeof(GLuint));
	    ConvertLong((GLuint *)raw->rowSize, x/sizeof(GLint));
	}
    }
    return raw;
}

static void RawImageClose(rawImageRec *raw)
{

    fclose(raw->file);
    free(raw->tmp);
    free(raw->tmpR);
    free(raw->tmpG);
    free(raw->tmpB);

    free(raw);
}

static void RawImageGetRow(rawImageRec *raw, unsigned char *buf, int y, int z)
{
  unsigned char *iPtr, *oPtr, pixel;
  int count;

  if ((raw->type & 0xFF00) == 0x0100) {
    fseek(raw->file, (long) raw->rowStart[y+z*raw->sizeY], SEEK_SET);
    fread(raw->tmp, 1, (unsigned int)raw->rowSize[y+z*raw->sizeY],
	  raw->file);

    iPtr = raw->tmp;
    oPtr = buf;
    for (;;) {
      pixel = *iPtr++;
      count = (int)(pixel & 0x7F);
      if (!count) {
	return;
      }
      if (pixel & 0x80) {
	while (count--) {
	  *oPtr++ = *iPtr++;
	}
      } else {
	pixel = *iPtr++;
	while (count--) {
	  *oPtr++ = pixel;
	}
      }
    }
  } else {
    fseek(raw->file, 512+(y*raw->sizeX)+(z*raw->sizeX*raw->sizeY),
	  SEEK_SET);
    fread(buf, 1, raw->sizeX, raw->file);
  }
}

static void
RawImageGetData(rawImageRec *raw, GLTXimage *image)
{
  unsigned char *ptr;
  int i, j;

  image->data = (unsigned char *)malloc((raw->sizeX+1)*(raw->sizeY+1)*4);
  if (image->data == NULL) {
    return;
  }

  ptr = image->data;
  for (i = 0; i < raw->sizeY; i++) {
    RawImageGetRow(raw, raw->tmpR, i, 0);
    RawImageGetRow(raw, raw->tmpG, i, 1);
    RawImageGetRow(raw, raw->tmpB, i, 2);
    for (j = 0; j < raw->sizeX; j++) {
      *ptr++ = *(raw->tmpR + j);
      *ptr++ = *(raw->tmpG + j);
      *ptr++ = *(raw->tmpB + j);
    }
  }
}



/* public functions */

/* gltxDelete: Deletes a texture image
 * 
 * image - properly initialized GLTXimage structure
 */
void
gltxDelete(GLTXimage* image)
{
  assert(image);

  free(image->data);
  free(image);
}

/* gltxReadRGB: Reads and returns data from an IRIS RGB image file.
 *
 * filename - name of the IRIS RGB file to read data from
 */
GLTXimage*
gltxReadRGB(char *filename)
{
  rawImageRec *raw;
  GLTXimage* image;

  raw = RawImageOpen(filename);
  if(!raw) {
    fprintf(stderr, "gltxReadRGB() failed: can't open image file \"%s\".\n",
	    filename);
    return NULL;
  }
  image = (GLTXimage*)malloc(sizeof(GLTXimage));
  if (image == NULL) {
    fprintf(stderr, "gltxReadRGB() failed: insufficient memory.\n");
    return NULL;
  }

  image->width = raw->sizeX;
  image->height = raw->sizeY;
  image->components = raw->sizeZ;
  RawImageGetData(raw, image);
  RawImageClose(raw);

  return image;
}
