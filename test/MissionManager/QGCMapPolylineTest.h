#pragma once

#include "TestFixtures.h"

class MultiSignalSpy;
class QGCMapPolyline;
class QmlObjectListModel;

/// Unit test for QGCMapPolyline.
/// Uses TempDirTest for automatic temp directory management.
class QGCMapPolylineTest : public TempDirTest
{
    Q_OBJECT

public:
    QGCMapPolylineTest() = default;

protected:
    void init() final;
    void cleanup() final;

private slots:
    // Dirty State Tests
    void _testDirty();

    // Vertex Manipulation Tests
    void _testVertexManipulation();
    void _testSelectVertex();

    // File Loading Tests
    void _testShapeLoad();

private:
    /// Copy a resource file to the temp directory
    QString _copyRes(const QString &name);

    MultiSignalSpy *_multiSpyPolyline = nullptr;
    int _countChangedMask = 0;
    int _pathChangedMask = 0;
    int _dirtyChangedMask = 0;
    int _clearedMask = 0;
    int _isEmptyChangedMask = 0;
    int _isValidChangedMask = 0;
    MultiSignalSpy *_multiSpyModel = nullptr;
    int _modelCountChangedMask = 0;
    int _modelDirtyChangedMask = 0;
    QGCMapPolyline *_mapPolyline = nullptr;
    QmlObjectListModel *_pathModel = nullptr;
    const QList<QGeoCoordinate> _linePoints = {
        QGeoCoordinate(47.635638361473475, -122.09269407980834),
        QGeoCoordinate(47.635638361473475, -122.08545246602667),
        QGeoCoordinate(47.63057923872075, -122.08545246602667),
        QGeoCoordinate(47.63057923872075, -122.09269407980834)
    };
};
