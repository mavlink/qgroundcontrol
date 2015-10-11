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

#ifndef QGCQGeoCoordinate_H
#define QGCQGeoCoordinate_H

#include <QObject>
#include <QGeoCoordinate>


/// This class wraps a QGeoCoordinate in a QObject so that it can be used from within a
/// QmlObjectListModel.
class QGCQGeoCoordinate : public QObject
{
    Q_OBJECT
    
public:
    QGCQGeoCoordinate(QObject* parent = NULL);
    QGCQGeoCoordinate(const QGeoCoordinate& coordinate, QObject* parent = NULL);
    ~QGCQGeoCoordinate();
    
    Q_PROPERTY(QGeoCoordinate coordinate MEMBER _coordinate NOTIFY coordinateChanged)
    
    void setCoordinate(const QGeoCoordinate& coordinate);
    
signals:
    void coordinateChanged(QGeoCoordinate coordinate);
    
private:
    QGeoCoordinate  _coordinate;
};

#endif
