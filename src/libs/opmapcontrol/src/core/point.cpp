/**
******************************************************************************
*
* @file       point.cpp
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
#include "point.h"
#include "size.h"

namespace core {
    Point::Point(int dw)
    {
        this->x=(short)Point::loWord(dw);
        this->y=(short)Point::hiWord(dw);
        empty=false;
    }
    Point::Point(Size sz)
    {
        this->x=sz.Width();
        this->y=sz.Height();
        empty=false;
    }
    Point::Point(int x, int y)
    {
        this->x=x;
        this->y=y;
        empty=false;
    }
    Point::Point():x(0),y(0),empty(true)
    {}
    uint qHash(Point const& point)
    {
        return point.x^point.y;
    }
    bool operator==(Point const &lhs,Point const &rhs)
    {
        return (lhs.x==rhs.x && lhs.y==rhs.y);
    }
    bool operator!=(Point const &lhs,Point const &rhs)
    {
        return !(lhs==rhs);
    }
    Point Point::Empty=Point();

}
