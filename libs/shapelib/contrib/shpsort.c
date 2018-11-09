/******************************************************************************
 * Copyright (c) 2004, Eric G. Miller
 *
 * This code is based in part on the earlier work of Frank Warmerdam
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 * shpsort
 *
 * Rewrite a shapefile sorted by a field or by the geometry.  For polygons,
 * sort by area, for lines sort by length and do nothing for all others.
 *
 * $Log: shpsort.c,v $
 * Revision 1.3  2004-07-06 21:23:17  fwarmerdam
 * minor const warning fix
 *
 * Revision 1.2  2004/07/06 21:20:49  fwarmerdam
 * major upgrade .. sort on multiple fields
 *
 * Revision 1.4  2004/06/30 18:19:53  emiller
 * handle POINTZ, POINTM
 *
 * Revision 1.3  2004/06/30 17:40:32  emiller
 * major rewrite allows sorting on multiple fields.
 *
 * Revision 1.2  2004/06/23 23:19:58  emiller
 * use tuple copy, misc changes
 *
 * Revision 1.1  2004/06/23 21:38:17  emiller
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "shapefil.h"

enum FieldOrderEnum {DESCENDING, ASCENDING};
enum FieldTypeEnum {
  FIDType = -2, 
  SHPType = -1, 
  StringType = FTString,
  LogicalType = FTLogical,
  IntegerType = FTInteger,
  DoubleType = FTDouble
};

struct DataUnion {
  int null;
  union {
    int i;
    double d;
    char *s;
  } u;
};

struct DataStruct {
  int record;
  struct DataUnion *value;
};

/* 
   globals used in sorting, each element could have a pointer to
   a single data struct, but that's still nShapes pointers more
   memory.  Alternatively, write a custom sort rather than using
   library qsort.
*/
int nFields;
int *fldIdx;
int *fldOrder;
int *fldType;
int shpType;
int nShapes;

static struct DataStruct * build_index (SHPHandle shp, DBFHandle dbf);
static char * dupstr (const char *);
static void copy_related (const char *inName, const char *outName, 
			  const char *old_ext, const char *new_ext);
static char ** split(const char *arg, const char *delim);
static int compare(const void *, const void *);
static double area2d_polygon (int n, double *x, double *y);
static double shp_area (SHPObject *feat);
static double length2d_polyline (int n, double *x, double *y);
static double shp_length (SHPObject *feat);

int main (int argc, char *argv[]) {

  SHPHandle  inSHP, outSHP;
  DBFHandle  inDBF, outDBF;
  int        len; 
  int        i;
  char       **fieldNames;
  char       **strOrder = 0;
  struct DataStruct *index;
  int        width;
  int        decimals;
  SHPObject  *feat;
  void       *tuple;

  if (argc < 4) {
    printf("USAGE: shpsort <infile> <outfile> <field[;...]> [<(ASCENDING|DESCENDING)[;...]>]\n");
    exit(EXIT_FAILURE);
  }

  inSHP = SHPOpen (argv[1], "rb");
  if (!inSHP) {
    fputs("Couldn't open shapefile for reading!\n", stderr);
    exit(EXIT_FAILURE);
  }
  SHPGetInfo(inSHP, &nShapes, &shpType, NULL, NULL);

  /* If we can open the inSHP, open its DBF */
  inDBF = DBFOpen (argv[1], "rb");
  if (!inDBF) {
    fputs("Couldn't open dbf file for reading!\n", stderr);
    exit(EXIT_FAILURE);
  }

  /* Parse fields and validate existence */
  fieldNames = split(argv[3], ";");
  if (!fieldNames) {
    fputs("ERROR: parsing field names!\n", stderr);
    exit(EXIT_FAILURE);
  }
  for (nFields = 0; fieldNames[nFields] ; nFields++) {
    continue;
  }

  fldIdx = malloc(sizeof *fldIdx * nFields);
  if (!fldIdx) {
    fputs("malloc failed!\n", stderr);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < nFields; i++) {
    len = (int)strlen(fieldNames[i]);
    while(len > 0) {
      --len;
      fieldNames[i][len] = (char)toupper((unsigned char)fieldNames[i][len]); 
    }
    fldIdx[i] = DBFGetFieldIndex(inDBF, fieldNames[i]);
    if (fldIdx[i] < 0) {
      /* try "SHAPE" */
      if (strcmp(fieldNames[i], "SHAPE") == 0) {
	fldIdx[i] = -1;
      }
      else if (strcmp(fieldNames[i], "FID") == 0) {
	fldIdx[i] = -2;
      }
      else {
	fprintf(stderr, "ERROR: field '%s' not found!\n", fieldNames[i]);
	exit(EXIT_FAILURE);
      }
    }
  }


  /* set up field type array */
  fldType = malloc(sizeof *fldType * nFields);
  if (!fldType) {
    fputs("malloc failed!\n", stderr);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < nFields; i++) {
    if (fldIdx[i] < 0) {
      fldType[i] = fldIdx[i];
    }
    else {
      fldType[i] = DBFGetFieldInfo(inDBF, fldIdx[i], NULL, &width, &decimals);
      if (fldType[i] == FTInvalid) {
	fputs("Unrecognized field type in dBASE file!\n", stderr);
	exit(EXIT_FAILURE);
      }
    }
  }


  /* set up field order array */
  fldOrder = malloc(sizeof *fldOrder * nFields);
  if (!fldOrder) {
    fputs("malloc failed!\n", stderr);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < nFields; i++) {
    /* default to ascending order */
    fldOrder[i] = ASCENDING;
  }
  if (argc > 4) {
    strOrder = split(argv[4], ";");
    if (!strOrder) {
      fputs("ERROR: parsing fields ordering!\n", stderr);
      exit(EXIT_FAILURE);
    }
    for (i = 0; i < nFields && strOrder[i]; i++) {
      if (strcmp(strOrder[i], "DESCENDING") == 0) {
	fldOrder[i] = DESCENDING;
      }
    }
  }

  /* build the index */
  index = build_index (inSHP, inDBF);

  /* Create output shapefile */
  outSHP = SHPCreate(argv[2], shpType);
  if (!outSHP) {
    fprintf(stderr, "%s:%d: couldn't create output shapefile!\n",
	    __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  
  /* Create output dbf */
  outDBF = DBFCloneEmpty(inDBF, argv[2]);
  if (!outDBF) {
    fprintf(stderr, "%s:%d: couldn't create output dBASE file!\n",
	    __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  /* Copy projection file, if any */
  copy_related(argv[1], argv[2], ".shp", ".prj");

  /* Copy metadata file, if any */
  copy_related(argv[1], argv[2], ".shp", ".shp.xml");

  /* Write out sorted results */
  for (i = 0; i < nShapes; i++) {
    feat = SHPReadObject(inSHP, index[i].record);
    if (SHPWriteObject(outSHP, -1, feat) < 0) {
      fprintf(stderr, "%s:%d: error writing shapefile!\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
    tuple = (void *) DBFReadTuple(inDBF, index[i].record);
    if (DBFWriteTuple(outDBF, i, tuple) < 0) {
      fprintf(stderr, "%s:%d: error writing dBASE file!\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
  }
  SHPClose(inSHP);
  SHPClose(outSHP);
  DBFClose(inDBF);
  DBFClose(outDBF);

  return EXIT_SUCCESS;

}

static char ** split(const char *arg, const char *delim)
{
  char *copy = dupstr(arg);
  char *cptr = copy;
  char **result = NULL;
  char **tmp;
  int i = 0;

  for (cptr = strtok(copy, delim); cptr; cptr = strtok(NULL, delim)) {
    tmp = realloc (result, sizeof *result * (i + 1));
    if (!tmp && result) {
      while (i > 0) {
	free(result[--i]);
      }
      free(result);
      free(copy);
      return NULL;
    }
    result = tmp;
    result[i++] = dupstr(cptr);
  }

  free(copy);

  if (i) {
    tmp = realloc(result, sizeof *result * (i + 1));
    if (!tmp) {
      while (i > 0) {
	free(result[--i]);
      }
      free(result);
      free(copy);
      return NULL;
    }
    result = tmp;
    result[i++] = NULL;
  }

  return result;
}


static void copy_related (const char *inName, const char *outName, 
			  const char *old_ext, const char *new_ext) 
{
  char *in;
  char *out;
  FILE *inFile;
  FILE *outFile;
  int  c;
  size_t name_len = strlen(inName);
  size_t old_len  = strlen(old_ext); 
  size_t new_len  = strlen(new_ext);

  in = malloc(name_len - old_len + new_len + 1);
  strncpy(in, inName, (name_len - old_len));
  strcpy(&in[(name_len - old_len)], new_ext);
  inFile = fopen(in, "rb");
  if (!inFile) {
    free(in);
    return;
  }
  name_len = strlen(outName);
  out = malloc(name_len - old_len + new_len + 1);
  strncpy(out, outName, (name_len - old_len));
  strcpy(&out[(name_len - old_len)], new_ext);
  outFile = fopen(out, "wb");
  if (!out) {
    fprintf(stderr, "%s:%d: couldn't copy related file!\n",
	    __FILE__, __LINE__);
    free(in);
    free(out);
    return;
  }
  while ((c = fgetc(inFile)) != EOF) {
    fputc(c, outFile);
  }
  fclose(inFile);
  fclose(outFile);
  free(in);
  free(out);
}

static char * dupstr (const char *src)
{
  char *dst = malloc(strlen(src) + 1);
  char *cptr;
  if (!dst) {
    fprintf(stderr, "%s:%d: malloc failed!\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  cptr = dst;
  while ((*cptr++ = *src++))
    ;
  return dst;
}

#ifdef DEBUG
static void PrintDataStruct (struct DataStruct *data) {
  int i, j;
  for (i = 0; i < nShapes; i++) {
    printf("data[%d] {\n", i);
    printf("\t.record = %d\n", data[i].record);
    for (j = 0; j < nFields; j++) {
      printf("\t.value[%d].null = %d\n", j, data[i].value[j].null);
      if (!data[i].value[j].null) {
	switch(fldType[j]) {
	case FIDType:
	case IntegerType:
	case LogicalType:
	  printf("\t.value[%d].u.i = %d\n", j, data[i].value[j].u.i);
	  break;
	case DoubleType:
	case SHPType:
	  printf("\t.value[%d].u.d = %f\n", j, data[i].value[j].u.d);
	  break;
	case StringType:
	  printf("\t.value[%d].u.s = %s\n", j, data[i].value[j].u.s);
	  break;
	}
      }
    }
    puts("}");
  }
}
#endif

static struct DataStruct * build_index (SHPHandle shp, DBFHandle dbf) {
  struct DataStruct *data;
  SHPObject  *feat;
  int i;
  int j;

  /* make array */
  data = malloc (sizeof *data * nShapes);
  if (!data) {
    fputs("malloc failed!\n", stderr);
    exit(EXIT_FAILURE);
  }

  /* populate array */
  for (i = 0; i < nShapes; i++) {
    data[i].value = malloc(sizeof data[0].value[0] * nFields);
    if (0 == data[i].value) {
      fputs("malloc failed!\n", stderr);
      exit(EXIT_FAILURE);
    }
    data[i].record = i;
    for (j = 0; j < nFields; j++) {
      data[i].value[j].null = 0;
      switch (fldType[j]) {
      case FIDType:
	data[i].value[j].u.i = i;
	break;
      case SHPType:
	feat = SHPReadObject(shp, i);
	switch (feat->nSHPType) {
	case SHPT_NULL:
	  fprintf(stderr, "Shape %d is a null feature!\n", i);
	  data[i].value[j].null = 1;
	  break;
	case SHPT_POINT:
	case SHPT_POINTZ:
	case SHPT_POINTM:
	case SHPT_MULTIPOINT:
	case SHPT_MULTIPOINTZ:
	case SHPT_MULTIPOINTM:
	case SHPT_MULTIPATCH:
	  /* Y-sort bounds */
	  data[i].value[j].u.d = feat->dfYMax;
	  break;
	case SHPT_ARC:
	case SHPT_ARCZ:
	case SHPT_ARCM:
	  data[i].value[j].u.d = shp_length(feat);
	  break;
	case SHPT_POLYGON:
	case SHPT_POLYGONZ:
	case SHPT_POLYGONM:
	  data[i].value[j].u.d = shp_area(feat);
	  break;
	default:
	  fputs("Can't sort on Shapefile feature type!\n", stderr);
	  exit(EXIT_FAILURE);
	}
	SHPDestroyObject(feat);
	break;
      case FTString:
	data[i].value[j].null = DBFIsAttributeNULL(dbf, i, fldIdx[j]);
	if (!data[i].value[j].null) {
	  data[i].value[j].u.s = dupstr(DBFReadStringAttribute(dbf, i, fldIdx[j]));
	}
	break;
      case FTInteger:
      case FTLogical:
	data[i].value[j].null = DBFIsAttributeNULL(dbf, i, fldIdx[j]);
	if (!data[i].value[j].null) {
	  data[i].value[j].u.i  = DBFReadIntegerAttribute(dbf, i, fldIdx[j]);
	}
	break;
      case FTDouble:
	data[i].value[j].null = DBFIsAttributeNULL(dbf, i, fldIdx[j]);
	if (!data[i].value[j].null) {
	  data[i].value[j].u.d = DBFReadDoubleAttribute(dbf, i, fldIdx[j]);	
	}
	break;
      }
    }
  }
  
#ifdef DEBUG
  PrintDataStruct(data);
  fputs("build_index: sorting array\n", stdout);
#endif

  qsort (data, nShapes, sizeof data[0], compare);

#ifdef DEBUG
  PrintDataStruct(data);
  fputs("build_index: returning array\n", stdout);
#endif

  return data;
}

static int compare(const void *A, const void *B) {
  const struct DataStruct *a = A;
  const struct DataStruct *b = B;
  int i;
  int result = 0;

  for (i = 0; i < nFields; i++) {
    if (a->value[i].null && b->value[i].null) {
      continue;
    }
    if (a->value[i].null && !b->value[i].null) {
      return (fldOrder[i]) ? 1 : -1;
    }
    if (!a->value[i].null && b->value[i].null) {
      return (fldOrder[i]) ? -1 : 1;
    }
    switch (fldType[i]) {
    case FIDType:
    case IntegerType:
    case LogicalType:
      if (a->value[i].u.i < b->value[i].u.i) {
	return (fldOrder[i]) ? -1 : 1;
      }
      if (a->value[i].u.i > b->value[i].u.i) {
	return (fldOrder[i]) ? 1 : -1;
      }
      break;
    case DoubleType:
    case SHPType:
      if (a->value[i].u.d < b->value[i].u.d) {
	return (fldOrder[i]) ? -1 : 1;
      }
      if (a->value[i].u.d > b->value[i].u.d) {
	return (fldOrder[i]) ? 1 : -1;
      }
      break;      
    case StringType:
      result = strcmp(a->value[i].u.s, b->value[i].u.s);
      if (result) {
	return (fldOrder[i]) ? result : -result;
      }
      break;
    default:
      fprintf(stderr, "compare: Program Error! Unhandled field type! fldType[%d] = %d\n", i, fldType[i]);
      break;
    }
  }
  return 0;
}

static double area2d_polygon (int n, double *x, double *y) {
  double area = 0;
  int i;
  for (i = 1; i < n; i++) {
    area += (x[i-1] + x[i]) * (y[i] - y[i-1]);
  }
  return area / 2.0;
}

static double shp_area (SHPObject *feat) {
  double area = 0.0;
  if (feat->nParts == 0) {
    area = area2d_polygon (feat->nVertices, feat->padfX, feat->padfY);
  }
  else {
    int part, n;
    for (part = 0; part < feat->nParts; part++) {
      if (part < feat->nParts - 1) {
	n = feat->panPartStart[part+1] - feat->panPartStart[part];
      }
      else {
	n = feat->nVertices - feat->panPartStart[part];
      }
      area += area2d_polygon (n, &(feat->padfX[feat->panPartStart[part]]),
			      &(feat->padfY[feat->panPartStart[part]]));
    }
  }
  /* our area function computes in opposite direction */
  return -area;
}

static double length2d_polyline (int n, double *x, double *y) {
  double length = 0.0;
  int i;
  for (i = 1; i < n; i++) {
    length += sqrt((x[i] - x[i-1])*(x[i] - x[i-1]) 
		   +
		   (y[i] - y[i-1])*(y[i] - y[i-1]));
  }
  return length;
}

static double shp_length (SHPObject *feat) {
  double length = 0.0;
  if (feat->nParts == 0) {
    length = length2d_polyline(feat->nVertices, feat->padfX, feat->padfY);
  }
  else {
    int part, n;
    for (part = 0; part < feat->nParts; part++) {
      if (part < feat->nParts - 1) {
	n = feat->panPartStart[part+1] - feat->panPartStart[part];
      }
      else {
	n = feat->nVertices - feat->panPartStart[part];
      }
      length += length2d_polyline (n, 
				   &(feat->padfX[feat->panPartStart[part]]),
				   &(feat->padfY[feat->panPartStart[part]]));
    }
  }
  return length;
}

