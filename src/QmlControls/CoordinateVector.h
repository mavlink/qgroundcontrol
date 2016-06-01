/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
