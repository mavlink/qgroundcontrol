/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMapPolylineTest.h"
#include "QGCApplication.h"
#include "QGCQGeoCoordinate.h"

QGCMapPolylineTest::QGCMapPolylineTest(void)
{
    _linePoints << QGeoCoordinate(47.635638361473475, -122.09269407980834 ) <<
                   QGeoCoordinate(47.635638361473475, -122.08545246602667) <<
                   QGeoCoordinate(47.63057923872075, -122.08545246602667) <<
                   QGeoCoordinate(47.63057923872075, -122.09269407980834);
}

void QGCMapPolylineTest::init(void)
{
    UnitTest::init();

    _rgSignals[countChangedIndex] = SIGNAL(countChanged(int));
    _rgSignals[pathChangedIndex] =  SIGNAL(pathChanged());
    _rgSignals[dirtyChangedIndex] = SIGNAL(dirtyChanged(bool));
    _rgSignals[clearedIndex] =      SIGNAL(cleared());

    _rgModelSignals[modelCountChangedIndex] = SIGNAL(countChanged(int));
    _rgModelSignals[modelDirtyChangedIndex] = SIGNAL(dirtyChanged(bool));

    _mapPolyline = new QGCMapPolyline(this);
    _pathModel = _mapPolyline->qmlPathModel();
    QVERIFY(_pathModel);

    _multiSpyPolyline = new MultiSignalSpy();
    QCOMPARE(_multiSpyPolyline->init(_mapPolyline, _rgSignals, _cSignals), true);

    _multiSpyModel = new MultiSignalSpy();
    QCOMPARE(_multiSpyModel->init(_pathModel, _rgModelSignals, _cModelSignals), true);
}

void QGCMapPolylineTest::cleanup(void)
{
    delete _mapPolyline;
    delete _multiSpyPolyline;
    delete _multiSpyModel;
}

void QGCMapPolylineTest::_testDirty(void)
{
    // Check basic dirty bit set/get

    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->checkNoSignals());
    QVERIFY(_multiSpyModel->checkNoSignals());

    _mapPolyline->setDirty(true);
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpyPolyline->pullBoolFromSignalIndex(dirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkNoSignals());
    _multiSpyPolyline->clearAllSignals();

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(!_multiSpyPolyline->pullBoolFromSignalIndex(dirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkNoSignals());
    _multiSpyPolyline->clearAllSignals();

    _pathModel->setDirty(true);
    QVERIFY(_pathModel->dirty());
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpyPolyline->pullBoolFromSignalIndex(dirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask));
    QVERIFY(_multiSpyModel->pullBoolFromSignalIndex(modelDirtyChangedIndex));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(!_multiSpyPolyline->pullBoolFromSignalIndex(dirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask));
    QVERIFY(!_multiSpyModel->pullBoolFromSignalIndex(modelDirtyChangedIndex));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();
}

void QGCMapPolylineTest::_testVertexManipulation(void)
{
    // Vertex addition testing

    for (int i=0; i<_linePoints.count(); i++) {
        QCOMPARE(_mapPolyline->count(), i);

        _mapPolyline->appendVertex(_linePoints[i]);
        QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(pathChangedMask | dirtyChangedMask | countChangedMask));
        QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask | modelCountChangedMask));
        QCOMPARE(_multiSpyPolyline->pullIntFromSignalIndex(countChangedIndex), i+1);
        QCOMPARE(_multiSpyModel->pullIntFromSignalIndex(modelCountChangedIndex), i+1);

        QVERIFY(_mapPolyline->dirty());
        QVERIFY(_pathModel->dirty());

        QCOMPARE(_mapPolyline->count(), i+1);

        QVariantList vertexList = _mapPolyline->path();
        QCOMPARE(vertexList.count(), i+1);
        QCOMPARE(vertexList[i].value<QGeoCoordinate>(), _linePoints[i]);

        QCOMPARE(_pathModel->count(), i+1);
        QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(i)->coordinate(), _linePoints[i]);

        _mapPolyline->setDirty(false);
        _multiSpyPolyline->clearAllSignals();
        _multiSpyModel->clearAllSignals();
    }

    // Vertex adjustment testing

    QGCQGeoCoordinate* geoCoord = _pathModel->value<QGCQGeoCoordinate*>(1);
    QSignalSpy coordSpy(geoCoord, SIGNAL(coordinateChanged(QGeoCoordinate)));
    QSignalSpy coordDirtySpy(geoCoord, SIGNAL(dirtyChanged(bool)));
    QGeoCoordinate adjustCoord(_linePoints[1].latitude() + 1, _linePoints[1].longitude() + 1);
    _mapPolyline->adjustVertex(1, adjustCoord);
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(pathChangedMask | dirtyChangedMask));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask));
    QCOMPARE(coordSpy.count(), 1);
    QCOMPARE(coordDirtySpy.count(), 1);
    QCOMPARE(geoCoord->coordinate(), adjustCoord);
    QVariantList vertexList = _mapPolyline->path();
    QCOMPARE(vertexList[0].value<QGeoCoordinate>(), _linePoints[0]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(0)->coordinate(), _linePoints[0]);
    QCOMPARE(vertexList[2].value<QGeoCoordinate>(), _linePoints[2]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(2)->coordinate(), _linePoints[2]);
    QCOMPARE(vertexList[3].value<QGeoCoordinate>(), _linePoints[3]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(3)->coordinate(), _linePoints[3]);

    _mapPolyline->setDirty(false);
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    // Vertex removal testing

    _mapPolyline->removeVertex(1);
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(pathChangedMask | dirtyChangedMask | countChangedMask));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask | modelCountChangedMask));
    QCOMPARE(_mapPolyline->count(), 3);
    vertexList = _mapPolyline->path();
    QCOMPARE(vertexList.count(), 3);
    QCOMPARE(_pathModel->count(), 3);
    QCOMPARE(vertexList[0].value<QGeoCoordinate>(), _linePoints[0]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(0)->coordinate(), _linePoints[0]);
    QCOMPARE(vertexList[1].value<QGeoCoordinate>(), _linePoints[2]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(1)->coordinate(), _linePoints[2]);
    QCOMPARE(vertexList[2].value<QGeoCoordinate>(), _linePoints[3]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(2)->coordinate(), _linePoints[3]);

    // Clear testing

    _mapPolyline->clear();
    QVERIFY(_multiSpyPolyline->checkOnlySignalsByMask(pathChangedMask | dirtyChangedMask | countChangedMask | clearedMask));
    QVERIFY(_multiSpyModel->checkOnlySignalsByMask(modelDirtyChangedMask | modelCountChangedMask));
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(_pathModel->dirty());
    QCOMPARE(_mapPolyline->count(), 0);
    vertexList = _mapPolyline->path();
    QCOMPARE(vertexList.count(), 0);
    QCOMPARE(_pathModel->count(), 0);
}

#if 0
void QGCMapPolylineTest::_testKMLLoad(void)
{
    QVERIFY(_mapPolyline->loadKMLFile(QStringLiteral(":/unittest/PolygonGood.kml")));

    setExpectedMessageBox(QMessageBox::Ok);
    QVERIFY(!_mapPolyline->loadKMLFile(QStringLiteral(":/unittest/BadXml.kml")));
    checkExpectedMessageBox();

    setExpectedMessageBox(QMessageBox::Ok);
    QVERIFY(!_mapPolyline->loadKMLFile(QStringLiteral(":/unittest/MissingPolygonNode.kml")));
    checkExpectedMessageBox();

    setExpectedMessageBox(QMessageBox::Ok);
    QVERIFY(!_mapPolyline->loadKMLFile(QStringLiteral(":/unittest/BadCoordinatesNode.kml")));
    checkExpectedMessageBox();
}
#endif

void QGCMapPolylineTest::_testSelectVertex(void)
{
    // Create polyline
    foreach (auto vertex, _linePoints) {
        _mapPolyline->appendVertex(vertex);
    }

    QVERIFY(_mapPolyline->selectedVertex() == -1);
    QVERIFY(_mapPolyline->count() == _linePoints.count());

    // Test deselect
    _mapPolyline->selectVertex(-1);
    QVERIFY(_mapPolyline->selectedVertex() == -1);
    // Test out of bounds
    _mapPolyline->selectVertex(_linePoints.count());
    QVERIFY(_mapPolyline->selectedVertex() == -1);
    // Simple select test
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    QVERIFY(_mapPolyline->selectedVertex() == _linePoints.count() - 1);
    // Keep selected test
    _mapPolyline->selectVertex(0);
    _mapPolyline->removeVertex(_linePoints.count() - 1);
    QVERIFY(_mapPolyline->selectedVertex() == 0);
    // Deselect if selected vertex removed
    _mapPolyline->appendVertex(_linePoints[_linePoints.count() - 1]);
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    _mapPolyline->removeVertex(_linePoints.count() - 1);
    QVERIFY(_mapPolyline->selectedVertex() == -1);
    // Shift selected index down if removed index < selected index
    _mapPolyline->appendVertex(_linePoints[_linePoints.count() - 1]);
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    _mapPolyline->removeVertex(0);
    QVERIFY(_mapPolyline->selectedVertex() == _mapPolyline->count() - 1);
}
