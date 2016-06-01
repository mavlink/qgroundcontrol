/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
