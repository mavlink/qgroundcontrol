#pragma once

#include <memory>

#include "UnitTest.h"

class QmlObjectListModel;
class QGCMapPolygon;
class MultiSignalSpy;

class QGCMapPolygonTest : public UnitTest
{
    Q_OBJECT

public:
    QGCMapPolygonTest();

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testVertexManipulation();
    void _testKMLLoad();
    void _testSelectVertex();
    void _testSegmentSplit();
    void _testCenterRectangle();
    void _testCenterExtraVertex();
    void _testCenterDegenerate();

private:
    std::unique_ptr<MultiSignalSpy> _multiSpyPolygon;
    std::unique_ptr<MultiSignalSpy> _multiSpyModel;
    QGCMapPolygon* _mapPolygon = nullptr;
    QmlObjectListModel* _pathModel = nullptr;
    QList<QGeoCoordinate> _polyPoints;
};
