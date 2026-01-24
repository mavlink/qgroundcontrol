#pragma once

#include "TestFixtures.h"

class QmlObjectListModel;
class QGCMapPolygon;
class MultiSignalSpy;

/// Unit test for QGCMapPolygon.
/// Uses OfflineTest since it doesn't require a vehicle connection.
class QGCMapPolygonTest : public OfflineTest
{
    Q_OBJECT

public:
    QGCMapPolygonTest();

protected:
    void init() final;
    void cleanup() final;

private slots:
    // Dirty State Tests
    void _testDirty();

    // Vertex Manipulation Tests
    void _testVertexManipulation();
    void _testSelectVertex();
    void _testSegmentSplit();

    // File Loading Tests
    void _testKMLLoad();

private:
    MultiSignalSpy* _multiSpyPolygon = nullptr;
    MultiSignalSpy* _multiSpyModel = nullptr;
    QGCMapPolygon* _mapPolygon = nullptr;
    QmlObjectListModel* _pathModel = nullptr;
    QList<QGeoCoordinate> _polyPoints;
};
