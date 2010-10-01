/* Lifted from libtk/image.c */

/* This code lets your read in SGI .rgb files. */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "image.h"

#ifndef SEEK_SET
#  define SEEK_SET 0
#endif


/******************************************************************************/

typedef unsigned int GLuint;  /* Should be 32-bit value. */
typedef int GLint;  /* Should be 32-bit value. */

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

/******************************************************************************/

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

static void ConvertUint(GLuint *array, unsigned int length)
{
    unsigned int b1, b2, b3, b4;
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
    int swapFlag;
    int x;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
	swapFlag = 1;
    } else {
	swapFlag = 0;
    }

    raw = (rawImageRec *)malloc(sizeof(rawImageRec));
    if (raw == NULL) {
	fprintf(stderr, "Out of memory!\n");
	exit(1);
    }
    if ((raw->file = fopen(fileName, "rb")) == NULL) {
	perror(fileName);
	exit(1);
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
	fprintf(stderr, "Out of memory!\n");
	exit(1);
    }

    if ((raw->type & 0xFF00) == 0x0100) {
	x = raw->sizeY * raw->sizeZ * (int) sizeof(GLuint);
	raw->rowStart = (GLuint *)malloc(x);
	raw->rowSize = (GLint *)malloc(x);
	if (raw->rowStart == NULL || raw->rowSize == NULL) {
	    fprintf(stderr, "Out of memory!\n");
	    exit(1);
	}
	raw->rleEnd = 512 + (2 * x);
	fseek(raw->file, 512, SEEK_SET);
	fread(raw->rowStart, 1, x, raw->file);
	fread(raw->rowSize, 1, x, raw->file);
	if (swapFlag) {
	    ConvertUint(raw->rowStart,
	        (unsigned int) (x/sizeof(GLuint)));
	    ConvertUint((GLuint *)raw->rowSize,
	        (unsigned int) (x/sizeof(GLint)));
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

static void RawImageGetData(rawImageRec *raw, RGBImageRec *final)
{
    unsigned char *ptr;
    int i, j;

    final->data = (unsigned char *)malloc((raw->sizeX+1)*(raw->sizeY+1)*4);
    if (final->data == NULL) {
	fprintf(stderr, "Out of memory!\n");
	exit(1);
    }

    ptr = final->data;
    for (i = 0; i < (int)(raw->sizeY); i++) {
	RawImageGetRow(raw, raw->tmpR, i, 0);
	RawImageGetRow(raw, raw->tmpG, i, 1);
	RawImageGetRow(raw, raw->tmpB, i, 2);
	for (j = 0; j < (int)(raw->sizeX); j++) {
	    *ptr++ = *(raw->tmpR + j);
	    *ptr++ = *(raw->tmpG + j);
	    *ptr++ = *(raw->tmpB + j);
	}
    }
}

RGBImageRec *RGBImageLoad(char *fileName)
{
    rawImageRec *raw;
    RGBImageRec *final;

    raw = RawImageOpen(fileName);
    final = (RGBImageRec *)malloc(sizeof(RGBImageRec));
    if (final == NULL) {
	fprintf(stderr, "Out of memory!\n");
	exit(1);
    }
    final->sizeX = raw->sizeX;
    final->sizeY = raw->sizeY;
    RawImageGetData(raw, final);
    RawImageClose(raw);
    return final;
}

/******************************************************************************/
