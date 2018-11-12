===============================================================================
Project:  Shape_PoinInPoly
Purpose:  Sample and the function for calculatin a point in a polygon 
	  (complex,compound - it doesn't matter). Can be used for labeling.
Author:   Copyright (c) 2004, Marko Podgorsek, d-mon@siol.net
===============================================================================
Requires: shapelib 1.2 (http://shapelib.maptools.org/)
Tested and created on platform: 
   Windows 2000 Professional
   Visual Studio .NET 7.0
   P4 2.4 GHz
   1GB RAM

I just found out about the ShapeLib, GDAL and OGR and I must say that they're 
all great projects.
I belive I'll use some of those libraries in the future. Right now I'm using 
only shapelib.
The thing that led me to the http://wwww.maptools.org was the need of finding 
the point in poly...but as I found out that even OGR didn't support it. So 
there I was. I was forced to make my own function. Well, it was fun. I learned
a lot.
I wrote this function for the Autodesk Autocad 2004 MPolygon, because there was
no function to do this in the Object Arx SDK (the Acad programming SDK). Well, 
it will be in the 2005 release...but, still. There is a function in the 
Autodesk Map 2004 version...in the menu.
Not useful when you need the coordinates, not the point on the screen...
So when the Acad version was done I was thinking of doing it on the Shape files,
too. A little bit of changing the structures and variable
types (so they're not using Object Arx) and I was done.
And here it is....Contribution from me to the ShapeLib world :)...and maybe even
OGR (a little bit of changing there).

Some statistics:
For about 69000 polygons in Autocad picture (.dwg files)
Autodesk Map 2004 was creating centroids (the menu command) about 45s (1 scan 
line)
My function, with 3 scan lines took about 5s. And I was drawing the dots on the
picture...

-------------------------------------------------------------------------------
DPoint2d CreatePointInPoly(SHPObject *psShape, int quality)

The second parameter quality tell the function just how many rays shall it use
to get the point. 
quality = 3 works very well, but anything below 5 is good.
This doesn't mean that the execution will slow down, but it just finds a good
point. That's all.

The qality shows on the compound objects (multiple poligons with more than one
exterior loop) - if not enough rays, then there may be no centroid.
Or the U shaped thin polygon, only the bootom (below the y center line) is fat.
Autodesk Map with one scan line will create the centroid on one of the thin
parts, because it only uses the y center line. If you have more rays, one will
surely pass the fat area and centroid will be created there.

-------------------------------------------------------------------------------
Anyone using this function:
Just send me an e-mail, so I'll see if I did anything good for the public.
And you can send me e-mail with questions also.
