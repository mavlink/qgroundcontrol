/**
******************************************************************************
*
* @file       size.h
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
#ifndef SIZE_H
#define SIZE_H

#include "point.h"
#include <QString>
#include <QHash>

namespace core {
    struct Size
    {

        Size();
        Size(Point pt){width=pt.X(); height=pt.Y();};
        Size(int Width,int Height){width=Width; height=Height;};
        friend uint qHash(Size const& size);
        //  friend bool operator==(Size const& lhs,Size const& rhs);
        Size operator-(const Size &sz1){return Size(width-sz1.width,height-sz1.height);}
        Size operator+(const Size &sz1){return Size(sz1.width+width,sz1.height+height);}

        int GetHashCode(){return width^height;}
        uint qHash(Size const& /*rect*/){return width^height;}
        QString ToString(){return "With="+QString::number(width)+" ,Height="+QString::number(height);}
        int Width()const {return width;}
        int Height()const {return height;}
        void SetWidth(int const& value){width=value;}
        void SetHeight(int const& value){height=value;}
    private:
        int width;
        int height;
    };
}
#endif // SIZE_H
