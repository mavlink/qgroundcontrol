/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>

/// This is a QGeoCoordinate within a QObject such that it can be used on a QmlObjectListModel
class QGCQGeoCoordinate : public QObject
{
    Q_OBJECT

public:
    QGCQGeoCoordinate(const QGeoCoordinate& coord, QObject* parent = nullptr);

    Q_PROPERTY(QGeoCoordinate   coordinate  READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(bool             dirty       READ dirty      WRITE setDirty      NOTIFY dirtyChanged)

    QGeoCoordinate  coordinate      (void) const { return _coordinate; }
    void            setCoordinate   (const QGeoCoordinate& coordinate);
    bool            dirty           (void) const { return _dirty; }
    void            setDirty        (bool dirty);

signals:
    void coordinateChanged  (QGeoCoordinate coordinate);
    void dirtyChanged       (bool dirty);

private:
    QGeoCoordinate  _coordinate;
    bool            _dirty;
};
