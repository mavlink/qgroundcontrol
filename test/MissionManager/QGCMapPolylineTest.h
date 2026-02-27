#pragma once

#include "CoordFixtures.h"
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

private:
    QString _copyRes(const QTemporaryDir& tmpDir, const QString& name);

    MultiSignalSpy* _multiSpyPolyline = nullptr;
    MultiSignalSpy* _multiSpyModel = nullptr;
    QGCMapPolyline* _mapPolyline = nullptr;
    QmlObjectListModel* _pathModel = nullptr;
    const QList<QGeoCoordinate> _linePoints = TestFixtures::Coord::missionTestRectangle();
};
