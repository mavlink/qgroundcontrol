/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "CoordinateVector.h"

CoordinateVector::CoordinateVector(QObject* parent)
    : QObject(parent)
{

}

CoordinateVector::CoordinateVector(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2, QObject* parent)
    : QObject(parent)
    , _coordinate1(coordinate1)
    , _coordinate2(coordinate2)
{
    
}

CoordinateVector::~CoordinateVector()
{
    
}

void CoordinateVector::setCoordinates(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2)
{
    _coordinate1 = coordinate1;
    _coordinate2 = coordinate2;
    emit coordinate1Changed(_coordinate1);
    emit coordinate2Changed(_coordinate2);
}
