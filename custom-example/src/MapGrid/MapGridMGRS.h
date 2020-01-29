/****************************************************************************
 *
 *   (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariant>

class MapGridMGRS : public QObject
{
    Q_OBJECT
public:
    explicit MapGridMGRS();

public slots:
    void geometryChanged(double zoomLevel, QGeoCoordinate topLeft, QGeoCoordinate bottomRight);

signals:
    void updateValues(QVariant values);

private:
};
