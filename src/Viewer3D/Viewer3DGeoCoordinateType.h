#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(Viewer3DGeoCoordinateTypeLog)

class Viewer3DGeoCoordinateType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QGeoCoordinate gpsRef          READ gpsRef          WRITE setGpsRef      NOTIFY gpsRefChanged)
    Q_PROPERTY(QGeoCoordinate coordinate      READ coordinate      WRITE setCoordinate  NOTIFY coordinateChanged)
    Q_PROPERTY(QVector3D      localCoordinate READ localCoordinate                      NOTIFY localCoordinateChanged)

public:
    explicit Viewer3DGeoCoordinateType(QObject *parent = nullptr) : QObject(parent) {}

    QGeoCoordinate gpsRef() const { return _gpsRef; }
    void setGpsRef(const QGeoCoordinate &newGpsRef);

    QGeoCoordinate coordinate() const { return _coordinate; }
    void setCoordinate(const QGeoCoordinate &newCoordinate);

    QVector3D localCoordinate() const { return _localCoordinate; }

signals:
    void gpsRefChanged();
    void coordinateChanged();
    void localCoordinateChanged();

private:
    void _gpsToLocal();

    QGeoCoordinate _gpsRef;
    QGeoCoordinate _coordinate;
    QVector3D _localCoordinate;
};
