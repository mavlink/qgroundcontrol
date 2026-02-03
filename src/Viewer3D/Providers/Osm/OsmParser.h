#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QVariant>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtQmlIntegration/QtQmlIntegration>

#include "Viewer3DMapProvider.h"

Q_DECLARE_LOGGING_CATEGORY(OsmParserLog)

class OsmParserThread;

class OsmParser : public Viewer3DMapProvider
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    friend class OsmParserTest;

public:
    explicit OsmParser(QObject *parent = nullptr);

    bool mapLoaded() const override { return _mapLoadedFlag; }
    QGeoCoordinate gpsRef() const override { return _gpsRefPoint; }
    std::pair<QGeoCoordinate, QGeoCoordinate> mapBoundingBox() const override { return {_coordinateMin, _coordinateMax}; }

    float buildingLevelHeight() const { return _buildingLevelHeight; }

    void setGpsRef(const QGeoCoordinate &gpsRef);
    void resetGpsRef();
    void parseOsmFile(const QString &filePath);
    QByteArray buildingToMesh();

signals:
    void buildingLevelHeightChanged();

private:
    void _setBuildingLevelHeight(const QVariant &value);
    void _onOsmParserFinished(bool isValid);
    void _triangulateWallsExtrudedPolygon(std::vector<QVector3D> &triangulatedMesh, const std::vector<QVector2D> &verticesCcw, float h, bool inverseOrder);
    void _triangulateRectangle(std::vector<QVector3D> &triangulatedMesh, const std::vector<QVector3D> &verticesCcw, bool invertNormal);

    OsmParserThread *_osmParserWorker = nullptr;

    QGeoCoordinate _gpsRefPoint;
    QGeoCoordinate _coordinateMin;
    QGeoCoordinate _coordinateMax;

    float _buildingLevelHeight = 0;
    bool _gpsRefSet = false;
    bool _mapLoadedFlag = false;
};
