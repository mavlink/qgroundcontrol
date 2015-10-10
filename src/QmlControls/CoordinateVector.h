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

#ifndef CoordinateVector_H
#define CoordinateVector_H

#include <QObject>
#include <QGeoCoordinate>

class CoordinateVector : public QObject
{
    Q_OBJECT
    
public:
    CoordinateVector(QObject* parent = NULL);
    CoordinateVector(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2, QObject* parent = NULL);
    ~CoordinateVector();
    
    Q_PROPERTY(QGeoCoordinate coordinate1 MEMBER _coordinate1 NOTIFY coordinate1Changed)
    Q_PROPERTY(QGeoCoordinate coordinate2 MEMBER _coordinate2 NOTIFY coordinate2Changed)
    
    void setCoordinates(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2);
    
signals:
    void coordinate1Changed(QGeoCoordinate coordinate);
    void coordinate2Changed(QGeoCoordinate coordinate);
    
private:
    QGeoCoordinate  _coordinate1;
    QGeoCoordinate  _coordinate2;
};

#endif
