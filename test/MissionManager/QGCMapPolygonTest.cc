#include "QGCMapPolygonTest.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>

#include "CoordFixtures.h"
#include "MultiSignalSpy.h"
#include "QGCMapPolygon.h"
#include "QGCQGeoCoordinate.h"
#include "QmlObjectListModel.h"

QGCMapPolygonTest::QGCMapPolygonTest()
{
    _polyPoints = TestFixtures::Coord::missionTestRectangle();
}

void QGCMapPolygonTest::init()
{
    UnitTest::init();
    _mapPolygon = new QGCMapPolygon(this);
    _pathModel = _mapPolygon->qmlPathModel();
    QVERIFY(_pathModel);
    _multiSpyPolygon = new MultiSignalSpy();
    QVERIFY(_multiSpyPolygon->init(_mapPolygon,
                                   {"countChanged", "pathChanged", "dirtyChanged", "cleared", "centerChanged"}));
    _multiSpyModel = new MultiSignalSpy();
    QVERIFY(_multiSpyModel->init(_pathModel, {"countChanged", "dirtyChanged"}));
}

void QGCMapPolygonTest::cleanup()
{
    delete _mapPolygon;
    _mapPolygon = nullptr;
    delete _multiSpyPolygon;
    _multiSpyPolygon = nullptr;
    delete _multiSpyModel;
    _multiSpyModel = nullptr;
    UnitTest::cleanup();
}

void QGCMapPolygonTest::_testDirty()
{
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());
    _mapPolygon->setDirty(true);
    QVERIFY(_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpyPolygon->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->noneEmitted());
    _multiSpyPolygon->clearAllSignals();
    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpyPolygon->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->noneEmitted());
    _multiSpyPolygon->clearAllSignals();
    _pathModel->setDirty(true);
    QVERIFY(_pathModel->dirty());
    QVERIFY(_mapPolygon->dirty());
    QVERIFY(_multiSpyPolygon->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpyPolygon->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpyModel->argument<bool>("dirtyChanged"));
    _multiSpyPolygon->clearAllSignals();
    _multiSpyModel->clearAllSignals();
    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpyPolygon->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpyModel->argument<bool>("dirtyChanged"));
    _multiSpyPolygon->clearAllSignals();
    _multiSpyModel->clearAllSignals();
}

void QGCMapPolygonTest::_testVertexManipulation()
{
    for (int i = 0; i < _polyPoints.count(); i++) {
        QCOMPARE(_mapPolygon->count(), i);
        _mapPolygon->appendVertex(_polyPoints[i]);
        QCoreApplication::processEvents();
        if (i >= 2) {
            QVERIFY2(_multiSpyPolygon->onlyEmittedOnceByMask(
                         _multiSpyPolygon->mask("pathChanged", "dirtyChanged", "countChanged", "centerChanged")),
                     qPrintable(_multiSpyPolygon->summary()));
        } else {
            QVERIFY2(_multiSpyPolygon->onlyEmittedOnceByMask(
                         _multiSpyPolygon->mask("pathChanged", "dirtyChanged", "countChanged")),
                     qPrintable(_multiSpyPolygon->summary()));
        }
        QVERIFY(_multiSpyModel->onlyEmittedOnceByMask(_multiSpyModel->mask("dirtyChanged", "countChanged")));
        QCOMPARE(_multiSpyPolygon->argument<int>("countChanged"), i + 1);
        QCOMPARE(_multiSpyModel->argument<int>("countChanged"), i + 1);
        QVERIFY(_mapPolygon->dirty());
        QVERIFY(_pathModel->dirty());
        QCOMPARE(_mapPolygon->count(), i + 1);
        QVariantList polyList = _mapPolygon->path();
        QCOMPARE(polyList.count(), i + 1);
        QCOMPARE(polyList[i].value<QGeoCoordinate>(), _polyPoints[i]);
        QCOMPARE(_pathModel->count(), i + 1);
        QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(i)->coordinate(), _polyPoints[i]);
        _mapPolygon->setDirty(false);
        _multiSpyPolygon->clearAllSignals();
        _multiSpyModel->clearAllSignals();
    }
    // Vertex adjustment testing
    QGCQGeoCoordinate* geoCoord = _pathModel->value<QGCQGeoCoordinate*>(1);
    QSignalSpy coordSpy(geoCoord, &QGCQGeoCoordinate::coordinateChanged);
    QSignalSpy coordDirtySpy(geoCoord, &QGCQGeoCoordinate::dirtyChanged);
    QGeoCoordinate adjustCoord(_polyPoints[1].latitude() + 1, _polyPoints[1].longitude() + 1);
    _mapPolygon->adjustVertex(1, adjustCoord);
    QCoreApplication::processEvents();
    QVERIFY(_multiSpyPolygon->onlyEmittedOnceByMask(
        _multiSpyPolygon->mask("pathChanged", "dirtyChanged", "centerChanged")));
    QVERIFY(_multiSpyModel->onlyEmittedOnce("dirtyChanged"));
    QCOMPARE(coordSpy.count(), 1);
    QCOMPARE(coordDirtySpy.count(), 1);
    QCOMPARE(geoCoord->coordinate(), adjustCoord);
    QVariantList polyList = _mapPolygon->path();
    QCOMPARE(polyList[0].value<QGeoCoordinate>(), _polyPoints[0]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(0)->coordinate(), _polyPoints[0]);
    QCOMPARE(polyList[2].value<QGeoCoordinate>(), _polyPoints[2]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(2)->coordinate(), _polyPoints[2]);
    QCOMPARE(polyList[3].value<QGeoCoordinate>(), _polyPoints[3]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(3)->coordinate(), _polyPoints[3]);
    _mapPolygon->setDirty(false);
    _multiSpyPolygon->clearAllSignals();
    _multiSpyModel->clearAllSignals();
    // Vertex removal testing
    _mapPolygon->removeVertex(1);
    QVERIFY(_multiSpyPolygon->onlyEmittedByMask(
        _multiSpyPolygon->mask("pathChanged", "dirtyChanged", "countChanged", "centerChanged")));
    QVERIFY(_multiSpyModel->onlyEmittedOnceByMask(_multiSpyModel->mask("dirtyChanged", "countChanged")));
    QCOMPARE(_mapPolygon->count(), 3);
    polyList = _mapPolygon->path();
    QCOMPARE(polyList.count(), 3);
    QCOMPARE(_pathModel->count(), 3);
    QCOMPARE(polyList[0].value<QGeoCoordinate>(), _polyPoints[0]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(0)->coordinate(), _polyPoints[0]);
    QCOMPARE(polyList[1].value<QGeoCoordinate>(), _polyPoints[2]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(1)->coordinate(), _polyPoints[2]);
    QCOMPARE(polyList[2].value<QGeoCoordinate>(), _polyPoints[3]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(2)->coordinate(), _polyPoints[3]);
    // Clear testing
    _mapPolygon->clear();
    QVERIFY(_multiSpyPolygon->onlyEmittedByMask(
        _multiSpyPolygon->mask("pathChanged", "dirtyChanged", "countChanged", "centerChanged", "cleared")));
    QVERIFY(_multiSpyModel->onlyEmittedByMask(_multiSpyModel->mask("dirtyChanged", "countChanged")));
    QVERIFY(_mapPolygon->dirty());
    QVERIFY(_pathModel->dirty());
    QCOMPARE(_mapPolygon->count(), 0);
    polyList = _mapPolygon->path();
    QCOMPARE(polyList.count(), 0);
    QCOMPARE(_pathModel->count(), 0);
}

void QGCMapPolygonTest::_testKMLLoad()
{
    QVERIFY(_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonGood.kml")));
    QVERIFY(!_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonBadXml.kml")));
    QVERIFY(!_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonMissingNode.kml")));
    QVERIFY(!_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonBadCoordinatesNode.kml")));
}

void QGCMapPolygonTest::_testSelectVertex()
{
    for (const QGeoCoordinate& vertex : std::as_const(_polyPoints)) {
        _mapPolygon->appendVertex(vertex);
    }
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    QVERIFY(_mapPolygon->count() == _polyPoints.count());
    _mapPolygon->selectVertex(-1);
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->selectVertex(_polyPoints.count());
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->selectVertex(_polyPoints.count() - 1);
    QVERIFY(_mapPolygon->selectedVertex() == _polyPoints.count() - 1);
    _mapPolygon->selectVertex(0);
    _mapPolygon->removeVertex(_polyPoints.count() - 1);
    QVERIFY(_mapPolygon->selectedVertex() == 0);
    _mapPolygon->appendVertex(_polyPoints[_polyPoints.count() - 1]);
    _mapPolygon->selectVertex(_polyPoints.count() - 1);
    _mapPolygon->removeVertex(_polyPoints.count() - 1);
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->appendVertex(_polyPoints[_polyPoints.count() - 1]);
    _mapPolygon->selectVertex(_polyPoints.count() - 1);
    _mapPolygon->removeVertex(0);
    QVERIFY(_mapPolygon->selectedVertex() == _mapPolygon->count() - 1);
}

void QGCMapPolygonTest::_testSegmentSplit()
{
    for (const QGeoCoordinate& vertex : std::as_const(_polyPoints)) {
        _mapPolygon->appendVertex(vertex);
    }
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    QVERIFY(_mapPolygon->count() == _polyPoints.count());
    QVERIFY(_mapPolygon->count() == 4);
    _mapPolygon->selectVertex(-1);
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->selectVertex(3);
    QVERIFY(_mapPolygon->selectedVertex() == 3);
    _mapPolygon->selectVertex(-1);
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->selectVertex(-1);
    _mapPolygon->splitPolygonSegment(0);
    QVERIFY(_mapPolygon->count() == 5);
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->selectVertex(0);
    _mapPolygon->splitPolygonSegment(0);
    QVERIFY(_mapPolygon->count() == 6);
    QVERIFY(_mapPolygon->selectedVertex() == 0);
    _mapPolygon->selectVertex(1);
    _mapPolygon->splitPolygonSegment(0);
    QVERIFY(_mapPolygon->count() == 7);
    QVERIFY(_mapPolygon->selectedVertex() == 2);
    _mapPolygon->selectVertex(-1);
    _mapPolygon->splitPolygonSegment(2);
    QVERIFY(_mapPolygon->count() == 8);
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->selectVertex(1);
    _mapPolygon->splitPolygonSegment(2);
    QVERIFY(_mapPolygon->count() == 9);
    QVERIFY(_mapPolygon->selectedVertex() == 1);
    _mapPolygon->selectVertex(2);
    _mapPolygon->splitPolygonSegment(2);
    QVERIFY(_mapPolygon->count() == 10);
    QVERIFY(_mapPolygon->selectedVertex() == 2);
    _mapPolygon->selectVertex(3);
    _mapPolygon->splitPolygonSegment(2);
    QVERIFY(_mapPolygon->count() == 11);
    QVERIFY(_mapPolygon->selectedVertex() == 4);
    _mapPolygon->selectVertex(-1);
    _mapPolygon->splitPolygonSegment(_mapPolygon->count() - 1);
    QVERIFY(_mapPolygon->count() == 12);
    QVERIFY(_mapPolygon->selectedVertex() == -1);
    _mapPolygon->selectVertex(_mapPolygon->count() - 2);
    _mapPolygon->splitPolygonSegment(_mapPolygon->count() - 1);
    QVERIFY(_mapPolygon->count() == 13);
    QVERIFY(_mapPolygon->selectedVertex() == _mapPolygon->count() - 3);
    _mapPolygon->selectVertex(_mapPolygon->count() - 1);
    _mapPolygon->splitPolygonSegment(_mapPolygon->count() - 1);
    QVERIFY(_mapPolygon->count() == 14);
    QVERIFY(_mapPolygon->selectedVertex() == _mapPolygon->count() - 2);
}

#include "UnitTest.h"

UT_REGISTER_TEST(QGCMapPolygonTest, TestLabel::Unit, TestLabel::MissionManager)
