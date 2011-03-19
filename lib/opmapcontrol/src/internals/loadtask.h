/**
******************************************************************************
*
* @file       loadtask.h
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
#ifndef LOADTASK_H
#define LOADTASK_H

#include <QString>
#include "../core/point.h"

using namespace core;
namespace internals
{
struct LoadTask
  {
     friend bool operator==(LoadTask const& lhs,LoadTask const& rhs);
  public:
    core::Point Pos;
    int Zoom;


    LoadTask(Point pos, int zoom)
     {
        Pos = pos;
        Zoom = zoom;
    }
    LoadTask()
    {
        Pos=core::Point(-1,-1);
        Zoom=-1;
    }
    bool HasValue()
    {
        return !(Zoom==-1);
    }

    QString ToString()const
     {
        return QString::number(Zoom) + " - " + Pos.ToString();
     }
  };
}
#endif // LOADTASK_H
