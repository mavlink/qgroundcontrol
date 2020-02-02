/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    CoordinateVector(QObject* parent = nullptr);
    CoordinateVector(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2, QObject* parent = nullptr);
    
    Q_PROPERTY(QGeoCoordinate coordinate1   MEMBER _coordinate1                         NOTIFY coordinate1Changed)
    Q_PROPERTY(QGeoCoordinate coordinate2   MEMBER _coordinate2                         NOTIFY coordinate2Changed)
    Q_PROPERTY(bool           specialVisual READ specialVisual WRITE setSpecialVisual   NOTIFY specialVisualChanged)

    QGeoCoordinate  coordinate1(void) const { return _coordinate1; }
    QGeoCoordinate  coordinate2(void) const { return _coordinate2; }
    bool            specialVisual(void) const { return _specialVisual; }

    void setCoordinates(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2);
    void setSpecialVisual(bool specialVisual);

public slots:
    void setCoordinate1(const QGeoCoordinate& coordinate);
    void setCoordinate2(const QGeoCoordinate& coordinate);
    
signals:
    void coordinate1Changed     (QGeoCoordinate coordinate);
    void coordinate2Changed     (QGeoCoordinate coordinate);
    void specialVisualChanged   (bool specialVisual);
    
private:
    QGeoCoordinate  _coordinate1;
    QGeoCoordinate  _coordinate2;
    bool            _specialVisual = false;
};

#endif
