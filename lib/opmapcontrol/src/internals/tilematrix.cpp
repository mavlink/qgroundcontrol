/**
******************************************************************************
*
* @file       tilematrix.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
* 
*****************************************************************************/
/* 
* This program is free software; you can redistribute it and/or modify 
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 3 of the License, or 
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
* for more details.
* 
* You should have received a copy of the GNU General Public License along 
* with this program; if not, write to the Free Software Foundation, Inc., 
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "tilematrix.h"

 
namespace internals {
TileMatrix::TileMatrix()
{
}
void TileMatrix::Clear()
{
    mutex.lock();
    foreach(Tile* t,matrix.values())
    {
        delete t;
        t=0;
    }
    matrix.clear();
    mutex.unlock();
}
//void TileMatrix::RebuildToUpperZoom()
//{
//    mutex.lock();
//    QList<Tile*> newtiles;
//    foreach(Tile* t,matrix.values())
//    {
//       Point point=Point(t->GetPos().X()*2,t->GetPos().Y()*2);
//       Tile* tile1=new Tile(t->GetZoom()+1,point);
//       Tile* tile2=new Tile(t->GetZoom()+1,Point(point.X()+1,point.Y()+0));
//       Tile* tile3=new Tile(t->GetZoom()+1,Point(point.X()+0,point.Y()+1));
//       Tile* tile4=new Tile(t->GetZoom()+1,Point(point.X()+1,point.Y()+1));
//
//       foreach(QByteArray arr, t->Overlays)
//        {
//           QImage ima=QImage::fromData(arr);
//           QImage ima1=ima.copy(0,0,ima.width()/2,ima.height()/2);
//           QImage ima2=ima.copy(ima.width()/2,0,ima.width()/2,ima.height()/2);
//           QImage ima3=ima.copy(0,ima.height()/2,ima.width()/2,ima.height()/2);
//           QImage ima4=ima.copy(ima.width()/2,ima.height()/2,ima.width()/2,ima.height()/2);
//           QByteArray ba;
//           QBuffer buf(&ba);
//           ima1.scaled(QSize(ima.width(),ima.height())).save(&buf,"PNG");
//           tile1->Overlays.append(ba);
//           QByteArray ba1;
//           QBuffer buf1(&ba1);
//           ima2.scaled(QSize(ima.width(),ima.height())).save(&buf1,"PNG");
//           tile2->Overlays.append(ba1);
//           QByteArray ba2;
//           QBuffer buf2(&ba2);
//           ima3.scaled(QSize(ima.width(),ima.height())).save(&buf2,"PNG");
//           tile3->Overlays.append(ba2);
//           QByteArray ba3;
//           QBuffer buf3(&ba3);
//           ima4.scaled(QSize(ima.width(),ima.height())).save(&buf3,"PNG");
//           tile4->Overlays.append(ba3);
//           newtiles.append(tile1);
//           newtiles.append(tile2);
//           newtiles.append(tile3);
//           newtiles.append(tile4);
//        }
//    }
//    foreach(Tile* t,matrix.values())
//    {
//        delete t;
//        t=0;
//    }
//    matrix.clear();
//    foreach(Tile* t,newtiles)
//    {
//        matrix.insert(t->GetPos(),t);
//    }
//
//    mutex.unlock();
//}

void TileMatrix::ClearPointsNotIn(QList<Point>list)
{
    removals.clear();
    mutex.lock();
    foreach(Point p, matrix.keys())
    {
        if(!list.contains(p))
        {
            removals.append(p);
        }
    }
    mutex.unlock();
    foreach(Point p,removals)
    {
        Tile* t=TileAt(p);
        if(t!=0)
        {
            mutex.lock();
            delete t;
            t=0;
            matrix.remove(p);
            mutex.unlock();
        }

    }
    removals.clear();
}
Tile* TileMatrix::TileAt(const Point &p)
{

#ifdef DEBUG_TILEMATRIX
    qDebug()<<"TileMatrix:TileAt:"<<p.ToString();
#endif //DEBUG_TILEMATRIX
    Tile* ret;
    mutex.lock();
    ret=matrix.value(p,0);
    mutex.unlock();
    return ret;
}
void TileMatrix::SetTileAt(const Point &p, Tile* tile)
{
    mutex.lock();
    Tile* t=matrix.value(p,0);
    if(t!=0)
        delete t;
    matrix.insert(p,tile);
    mutex.unlock();
}
}
