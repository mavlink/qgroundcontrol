#pragma once

#include "CoordFixtures.h"
#include "TempDirectoryTest.h"

class MultiSignalSpy;
class QGCMapPolyline;
class QmlObjectListModel;

class QGCMapPolylineTest : public TempDirectoryTest
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
    QString _copyRes(const QString& dirPath, const QString& name);

    MultiSignalSpy* _multiSpyPolyline = nullptr;
    MultiSignalSpy* _multiSpyModel = nullptr;
    QGCMapPolyline* _mapPolyline = nullptr;
    QmlObjectListModel* _pathModel = nullptr;
    const QList<QGeoCoordinate> _linePoints = TestFixtures::Coord::missionTestRectangle();
};
