/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

#include <QmlObjectListItem.h>

/// This is a QGeoCoordinate within a QObject such that it can be used on a QmlObjectListModel
class QGCQGeoCoordinate : public QmlObjectListItem
{
    Q_OBJECT

public:
    QGCQGeoCoordinate(const QGeoCoordinate& coord, QObject* parent = nullptr);

    Q_PROPERTY(QGeoCoordinate   coordinate  READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

    QGeoCoordinate  coordinate      (void) const { return _coordinate; }
    void            setCoordinate   (const QGeoCoordinate& coordinate);

signals:
    void coordinateChanged  (QGeoCoordinate coordinate);

private:
    QGeoCoordinate  _coordinate;
};
