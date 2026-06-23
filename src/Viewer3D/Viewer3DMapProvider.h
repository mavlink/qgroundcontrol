#pragma once

#include <utility>

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class Viewer3DMapProvider : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Base class")

public:
    explicit Viewer3DMapProvider(QObject *parent = nullptr) : QObject(parent) {}

    virtual bool mapLoaded() const = 0;
    virtual QGeoCoordinate gpsRef() const = 0;
    virtual std::pair<QGeoCoordinate, QGeoCoordinate> mapBoundingBox() const = 0;

signals:
    void gpsRefChanged(QGeoCoordinate newGpsRef, bool isRefSet);
    void mapChanged();
};
