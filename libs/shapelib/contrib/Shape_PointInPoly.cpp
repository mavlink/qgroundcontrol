/******************************************************************************
 * $Id: Shape_PointInPoly.cpp,v 1.2 2016-12-05 12:44:07 erouault Exp $
 *
 * Project:  Shapelib
 * Purpose:  Commandline program to generate points-in-polygons from a 
 *           shapefile as a shapefile.
 * Author:   Marko Podgorsek, d-mon@siol.net
 *
 ******************************************************************************
 * Copyright (c) 2004, Marko Podgorsek, d-mon@siol.net
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see COPYING).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
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
 *
 * $Log: Shape_PointInPoly.cpp,v $
 * Revision 1.2  2016-12-05 12:44:07  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.1  2004-01-09 16:47:57  fwarmerdam
 * New
 *
 */

static char rcsid[] = 
  "$Id: Shape_PointInPoly.cpp,v 1.2 2016-12-05 12:44:07 erouault Exp $";

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <shapefil.h>

#define MAXINTERSECTIONPOINTS 255

enum loopDir {
    kExterior,   
    kInterior,
    kError
};

struct DPoint2d
{
    DPoint2d() 
	{
            x = y = 0.0;
	};
    DPoint2d(double x, double y)
	{
            this->x = x;
            this->y = y;
	};
    double x,y;
};

struct IntersectPoint
{
    IntersectPoint(void) 
	{
            x = y = 0.0;
            boundry_nmb = 0;
            loopdirection = kError;
	};
    double x,y;
    int boundry_nmb;
    loopDir loopdirection;
};

loopDir LoopDirection(DPoint2d *vertices, int vertsize)
{
    int i;
    double sum = 0.0;
    for(i=0;i<vertsize-1;i++)
    {
        sum += (vertices[i].x*vertices[i+1].y)-(vertices[i].y*vertices[i+1].x);
    }

    if(sum>0)
        return kInterior;
    else
        return kExterior;
}

DPoint2d CreatePointInPoly(SHPObject *psShape, int quality)
{
    int i, j, k, end, vert, pointpos;
    double part, dx, xmin, xmax, ymin, ymax, y, x3, x4, y3, y4, len, maxlen = 0;
    DPoint2d *vertices;
    loopDir direction;
    IntersectPoint mp1, mp2, point1, point2, points[MAXINTERSECTIONPOINTS];

    xmin = psShape->dfXMin;
    ymin = psShape->dfYMin;
    xmax = psShape->dfXMax;
    ymax = psShape->dfYMax;
    part = (ymax-ymin)/(quality+1);
    dx = xmax-xmin;
    for(i=0;i<quality;i++)
    {
        y = ymin+part*(i+1);
        pointpos = 0;
        for(j=0;j<psShape->nParts;j++)
        {
            if(j==psShape->nParts-1)
                end = psShape->nVertices;
            else
                end = psShape->panPartStart[j+1];
            vertices = new DPoint2d [end-psShape->panPartStart[j]];
            for(k=psShape->panPartStart[j],vert=0;k<end;k++)
            {
                vertices[vert].x = psShape->padfX[k];
                vertices[vert++].y = psShape->padfY[k];
            }
            direction = LoopDirection(vertices, vert);
            for(k=0;k<vert-1;k++)
            {
                y3 = vertices[k].y;
                y4 = vertices[k+1].y;
                if((y3 >= y && y4 < y) || (y3 <= y && y4 > y)) //I check >= only once, because if it's not checked now (y3) it will be in the next iteration (which is y4 now)
                {
                    point1.boundry_nmb = j;
                    point1.loopdirection = direction;
                    x3 = vertices[k].x;
                    x4 = vertices[k+1].x;
                    if(y3==y)
                    {
                        point1.y = y3;
                        point1.x = x3;
                        if(direction == kInterior) //add point 2 times if the direction is interior, so that the final count of points is even
                        {
                            points[pointpos++]=point1;
                        }
                    }
                    else
                    {
                        point1.x = xmin+(((((x4-x3)*(y-y3))-((y4-y3)*(xmin-x3)))/((y4-y3)*dx))*dx); //striped down calculation of intersection of 2 lines
                        point1.y = y;
                    }
                    points[pointpos++]=point1;
                }
            }
            delete [] vertices;
        }

        for(j=1;j<pointpos;j++) //sort the found intersection points by x value
        {
            for(k=j;k>0;k--)
            {
                if(points[k].x < points[k-1].x)
                {
                    point1 = points[k];
                    points[k] = points[k-1];
                    points[k-1] = point1;
                }
                else
                {
                    break;
                }
            }
        }

        for(j=0;j<pointpos-1;j++)
        {
            point1 = points[j];
            point2 = points[j+1];
            if((point1.loopdirection == kExterior         &&  //some checkings for valid point
                point2.loopdirection == kExterior         &&
                point1.boundry_nmb == point2.boundry_nmb  &&
                j%2==0)                                   ||
               (point1.loopdirection == kExterior        &&
                point2.loopdirection == kInterior)        ||
               (point1.loopdirection == kInterior        &&
                point2.loopdirection == kExterior))
            {
                len = sqrt(pow(point1.x-point2.x, 2)+pow(point1.y-point2.y, 2));
                if(len >= maxlen)
                {
                    maxlen = len;
                    mp1 = point1;
                    mp2 = point2;
                }
            }
        }
    }

    return DPoint2d((mp1.x+mp2.x)*0.5, (mp1.y+mp2.y)*0.5);
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        printf("Usage: %s shpfile_path quality\n", argv[0]);
        return 1;
    }

    int i, nEntities, quality;
    SHPHandle hSHP;
    SHPObject	*psShape;
    DPoint2d pt;
    quality = atoi(argv[2]);
    hSHP = SHPOpen(argv[1], "rb");
    SHPGetInfo(hSHP, &nEntities, NULL, NULL, NULL);

    printf("PointInPoly v1.0, by Marko Podgorsek\n----------------\n");
    for( i = 0; i < nEntities; i++ )
    {
        psShape = SHPReadObject( hSHP, i );
        if(psShape->nSHPType == SHPT_POLYGON)
        {
            pt = CreatePointInPoly(psShape, quality);
            printf("%d: x=%f y=%f\n",i, pt.x,pt.y);
        }
        SHPDestroyObject( psShape );
    }

    SHPClose(hSHP);

    return 0;
}

