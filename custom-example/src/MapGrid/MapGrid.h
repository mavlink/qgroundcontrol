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

class MapGrid : public QObject
{
    Q_OBJECT
public:
    explicit MapGrid(QObject* mapGridQML);

    Q_INVOKABLE void geometryChanged(double zoomLevel, QGeoCoordinate topLeft, QGeoCoordinate bottomRight);

private:
    QObject* _mapGridQML = nullptr;
};
