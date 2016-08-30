/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef QGCMapPolygon_H
#define QGCMapPolygon_H

#include <QObject>
#include <QGeoCoordinate>
#include <QVariantList>

class QGCMapPolygon : public QObject
{
    Q_OBJECT

public:
    QGCMapPolygon(QObject* parent = NULL);

    const QGCMapPolygon& operator=(const QGCMapPolygon& other);

    Q_PROPERTY(QVariantList path    READ path   WRITE setPath   NOTIFY pathChanged)
    Q_PROPERTY(bool         dirty   READ dirty  WRITE setDirty  NOTIFY dirtyChanged)

    Q_INVOKABLE void clear(void);
    Q_INVOKABLE void addCoordinate(const QGeoCoordinate coordinate);
    Q_INVOKABLE void adjustCoordinate(int vertexIndex, const QGeoCoordinate coordinate);
    Q_INVOKABLE QGeoCoordinate center(void) const;
    Q_INVOKABLE int count(void) const { return _polygonPath.count(); }

    const QVariantList path(void) const { return _polygonPath; }
    void setPath(const QList<QGeoCoordinate>& path);
    void setPath(const QVariantList& path);

    const QGeoCoordinate operator[](int index) { return _polygonPath[index].value<QGeoCoordinate>(); }

    bool dirty(void) const { return _dirty; }
    void setDirty(bool dirty);

signals:
    void pathChanged(void);
    void dirtyChanged(bool dirty);

private:
    QVariantList    _polygonPath;
    bool            _dirty;
};

#endif
