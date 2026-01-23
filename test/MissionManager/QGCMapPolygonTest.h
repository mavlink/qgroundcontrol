#pragma once

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
    void _testAdjustVertexOutOfRange();
    void _testSplitSegmentOutOfRange();
    void _testRemoveVertexOutOfRange();

private:
    MultiSignalSpy* _multiSpyPolygon = nullptr;
    MultiSignalSpy* _multiSpyModel = nullptr;
    QGCMapPolygon* _mapPolygon = nullptr;
    QmlObjectListModel* _pathModel = nullptr;
    QList<QGeoCoordinate> _polyPoints;

    quint64 _polygonCountChangedMask = 0;
    quint64 _pathChangedMask = 0;
    quint64 _polygonDirtyChangedMask = 0;
    quint64 _polygonIsEmptyChangedMask = 0;
    quint64 _polygonIsValidChangedMask = 0;
    quint64 _clearedMask = 0;
    quint64 _centerChangedMask = 0;
    quint64 _modelCountChangedMask = 0;
    quint64 _modelDirtyChangedMask = 0;
};
