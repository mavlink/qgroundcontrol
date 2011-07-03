/**
******************************************************************************
*
* @file       point.h
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
#ifndef OPOINT_H
#define OPOINT_H


#include <QString>

namespace core {
    struct Size;
    struct Point
    {
        friend uint qHash(Point const& point);
        friend bool operator==(Point const& lhs,Point const& rhs);
        friend bool operator!=(Point const& lhs,Point const& rhs);
    public:

        Point();
        Point(int x,int y);
        Point(Size sz);
        Point(int dw);
        bool IsEmpty(){return empty;}
        int X()const{return this->x;}
        int Y()const{return this->y;}
        void SetX(const int &value){x=value;empty=false;}
        void SetY(const int &value){y=value;empty=false;}
        QString ToString()const{return "{"+QString::number(x)+","+QString::number(y)+"}";}

        static Point Empty;
        void Offset(const int &dx,const int &dy)
        {
            x += dx;
            y += dy;
        }
        void Offset(Point p)
        {
            Offset(p.x, p.y);
        }
        static int HIWORD(int n);
        static int LOWORD(int n);

    private:
        int x;
        int y;
        bool empty;
    };
}
#endif // POINT_H
