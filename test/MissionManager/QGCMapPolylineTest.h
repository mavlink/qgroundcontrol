#pragma once

#include "UnitTest.h"

class MultiSignalSpy;
class QGCMapPolyline;
class QmlObjectListModel;
class QTemporaryDir;

class QGCMapPolylineTest : public UnitTest
{
    Q_OBJECT

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testVertexManipulation();
    void _testShapeLoad();
    void _testSelectVertex();
    void _testSegmentSplit();
    void _testAdjustVertexOutOfRange();
    void _testSplitSegmentOutOfRange();
    void _testRemoveVertexOutOfRange();

private:
    QString _copyRes(const QTemporaryDir& tmpDir, const QString& name);

    MultiSignalSpy* _multiSpyPolyline = nullptr;
    MultiSignalSpy* _multiSpyModel = nullptr;
    QGCMapPolyline* _mapPolyline = nullptr;
    QmlObjectListModel* _pathModel = nullptr;
    const QList<QGeoCoordinate> _linePoints = {QGeoCoordinate(47.635638361473475, -122.09269407980834),
                                               QGeoCoordinate(47.635638361473475, -122.08545246602667),
                                               QGeoCoordinate(47.63057923872075, -122.08545246602667),
                                               QGeoCoordinate(47.63057923872075, -122.09269407980834)};

    quint64 _countChangedMask = 0;
    quint64 _pathChangedMask = 0;
    quint64 _dirtyChangedMask = 0;
    quint64 _isEmptyChangedMask = 0;
    quint64 _isValidChangedMask = 0;
    quint64 _clearedMask = 0;
    quint64 _modelCountChangedMask = 0;
    quint64 _modelDirtyChangedMask = 0;
};
