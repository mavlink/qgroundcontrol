#include "QGCMapPolygonTest.h"
#include "QGCMapPolygon.h"
#include "QGCQGeoCoordinate.h"
#include "MultiSignalSpy.h"
#include "QmlObjectListModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

QGCMapPolygonTest::QGCMapPolygonTest()
{
    _polyPoints << QGeoCoordinate(47.635638361473475, -122.09269407980834 ) <<
                   QGeoCoordinate(47.635638361473475, -122.08545246602667) <<
                   QGeoCoordinate(47.63057923872075, -122.08545246602667) <<
                   QGeoCoordinate(47.63057923872075, -122.09269407980834);
}

void QGCMapPolygonTest::init()
{
    OfflineTest::init();

    _mapPolygon = new QGCMapPolygon(this);
    VERIFY_NOT_NULL(_mapPolygon);
    _pathModel = _mapPolygon->qmlPathModel();
    VERIFY_NOT_NULL(_pathModel);

    _multiSpyPolygon = new MultiSignalSpy();
    VERIFY_NOT_NULL(_multiSpyPolygon);
    QVERIFY(_multiSpyPolygon->init(_mapPolygon));

    _multiSpyModel = new MultiSignalSpy();
    VERIFY_NOT_NULL(_multiSpyModel);
    QVERIFY(_multiSpyModel->init(_pathModel));
}

void QGCMapPolygonTest::cleanup()
{
    delete _multiSpyModel;
    _multiSpyModel = nullptr;
    delete _multiSpyPolygon;
    _multiSpyPolygon = nullptr;
    delete _mapPolygon;
    _mapPolygon = nullptr;
    _pathModel = nullptr;

    OfflineTest::cleanup();
}

void QGCMapPolygonTest::_testDirty()
{
    // Check basic dirty bit set/get

    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());

    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_NO_SIGNALS(*_multiSpyPolygon);
    QVERIFY_NO_SIGNALS(*_multiSpyModel);

    _mapPolygon->setDirty(true);
    QVERIFY(_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_SIGNAL_EMITTED(*_multiSpyPolygon, "dirtyChanged");
    QVERIFY(_multiSpyPolygon->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_NO_SIGNALS(*_multiSpyModel);
    _multiSpyPolygon->clearAllSignals();

    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_SIGNAL_EMITTED(*_multiSpyPolygon, "dirtyChanged");
    QVERIFY(!_multiSpyPolygon->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_NO_SIGNALS(*_multiSpyModel);
    _multiSpyPolygon->clearAllSignals();

    _pathModel->setDirty(true);
    QVERIFY(_pathModel->dirty());
    QVERIFY(_mapPolygon->dirty());
    QVERIFY_SIGNAL_EMITTED(*_multiSpyPolygon, "dirtyChanged");
    QVERIFY(_multiSpyPolygon->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_SIGNAL_EMITTED(*_multiSpyModel, "dirtyChanged");
    QVERIFY(_multiSpyModel->pullBoolFromSignal("dirtyChanged"));
    _multiSpyPolygon->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_SIGNAL_EMITTED(*_multiSpyPolygon, "dirtyChanged");
    QVERIFY(!_multiSpyPolygon->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_SIGNAL_EMITTED(*_multiSpyModel, "dirtyChanged");
    QVERIFY(!_multiSpyModel->pullBoolFromSignal("dirtyChanged"));
    _multiSpyPolygon->clearAllSignals();
    _multiSpyModel->clearAllSignals();
}

void QGCMapPolygonTest::_testVertexManipulation()
{
    // Vertex addition testing

    for (int i=0; i<_polyPoints.count(); i++) {
        QCOMPARE(_mapPolygon->count(), i);

        _mapPolygon->appendVertex(_polyPoints[i]);
        QVERIFY(_multiSpyPolygon->waitForSignal("pathChanged"));
        if (i >= 2) {
            // Center is no recalculated until there are 3 points or more
            QVERIFY(_multiSpyPolygon->checkSignalsByMask(_multiSpyPolygon->mask("pathChanged") | _multiSpyPolygon->mask("dirtyChanged") | _multiSpyPolygon->mask("countChanged") | _multiSpyPolygon->mask("centerChanged")));
        } else {
            QVERIFY(_multiSpyPolygon->checkSignalsByMask(_multiSpyPolygon->mask("pathChanged") | _multiSpyPolygon->mask("dirtyChanged") | _multiSpyPolygon->mask("countChanged")));
        }
        QVERIFY(_multiSpyModel->checkSignalsByMask(_multiSpyModel->mask("dirtyChanged") | _multiSpyModel->mask("countChanged")));
        QCOMPARE(_multiSpyPolygon->pullIntFromSignal("countChanged"), i+1);
        QCOMPARE(_multiSpyModel->pullIntFromSignal("countChanged"), i+1);

        QVERIFY(_mapPolygon->dirty());
        QVERIFY(_pathModel->dirty());

        QCOMPARE(_mapPolygon->count(), i+1);

        QVariantList polyList = _mapPolygon->path();
        QCOMPARE(polyList.count(), i+1);
        QCOMPARE(polyList[i].value<QGeoCoordinate>(), _polyPoints[i]);

        QCOMPARE(_pathModel->count(), i+1);
        QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(i)->coordinate(), _polyPoints[i]);

        _mapPolygon->setDirty(false);
        _multiSpyPolygon->clearAllSignals();
        _multiSpyModel->clearAllSignals();
    }

    // Vertex adjustment testing

    QGCQGeoCoordinate* geoCoord = _pathModel->value<QGCQGeoCoordinate*>(1);
    VERIFY_NOT_NULL(geoCoord);
    QSignalSpy coordSpy(geoCoord, SIGNAL(coordinateChanged(QGeoCoordinate)));
    QGC_VERIFY_SPY_VALID(coordSpy);
    QSignalSpy coordDirtySpy(geoCoord, SIGNAL(dirtyChanged(bool)));
    QGC_VERIFY_SPY_VALID(coordDirtySpy);
    QGeoCoordinate adjustCoord(_polyPoints[1].latitude() + 1, _polyPoints[1].longitude() + 1);
    _mapPolygon->adjustVertex(1, adjustCoord);
    QVERIFY(_multiSpyPolygon->waitForSignal("pathChanged"));
    QVERIFY(_multiSpyPolygon->checkSignalsByMask(_multiSpyPolygon->mask("pathChanged") | _multiSpyPolygon->mask("dirtyChanged") | _multiSpyPolygon->mask("centerChanged")));
    QVERIFY_SIGNAL_EMITTED(*_multiSpyModel, "dirtyChanged");
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
    // There is some double signalling on centerChanged which is not yet fixed, hence checkOnlySignals
    QVERIFY(_multiSpyPolygon->checkSignalsByMask(_multiSpyPolygon->mask("pathChanged") | _multiSpyPolygon->mask("dirtyChanged") | _multiSpyPolygon->mask("countChanged") | _multiSpyPolygon->mask("centerChanged")));
    QVERIFY(_multiSpyModel->checkSignalsByMask(_multiSpyModel->mask("dirtyChanged") | _multiSpyModel->mask("countChanged")));
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
    QVERIFY(_multiSpyPolygon->checkSignalsByMask(_multiSpyPolygon->mask("pathChanged") | _multiSpyPolygon->mask("dirtyChanged") | _multiSpyPolygon->mask("countChanged") | _multiSpyPolygon->mask("centerChanged") | _multiSpyPolygon->mask("cleared")));
    QVERIFY(_multiSpyModel->checkSignalsByMask(_multiSpyModel->mask("dirtyChanged") | _multiSpyModel->mask("countChanged")));
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
    // Create polygon
    for (const QGeoCoordinate &vertex : std::as_const(_polyPoints)) {
        _mapPolygon->appendVertex(vertex);
    }

    QCOMPARE(_mapPolygon->selectedVertex(), -1);
    QCOMPARE(_mapPolygon->count(), _polyPoints.count());

    // Test deselect
    _mapPolygon->selectVertex(-1);
    QCOMPARE(_mapPolygon->selectedVertex(), -1);
    // Test out of bounds
    _mapPolygon->selectVertex(_polyPoints.count());
    QCOMPARE(_mapPolygon->selectedVertex(), -1);
    // Simple select test
    _mapPolygon->selectVertex(_polyPoints.count() - 1);
    QCOMPARE(_mapPolygon->selectedVertex(), _polyPoints.count() - 1);
    // Keep selected test
    _mapPolygon->selectVertex(0);
    _mapPolygon->removeVertex(_polyPoints.count() - 1);
    QCOMPARE(_mapPolygon->selectedVertex(), 0);
    // Deselect if selected vertex removed
    _mapPolygon->appendVertex(_polyPoints[_polyPoints.count() - 1]);
    _mapPolygon->selectVertex(_polyPoints.count() - 1);
    _mapPolygon->removeVertex(_polyPoints.count() - 1);
    QCOMPARE(_mapPolygon->selectedVertex(), -1);
    // Shift selected index down if removed index < selected index
    _mapPolygon->appendVertex(_polyPoints[_polyPoints.count() - 1]);
    _mapPolygon->selectVertex(_polyPoints.count() - 1);
    _mapPolygon->removeVertex(0);
    QCOMPARE(_mapPolygon->selectedVertex(), _mapPolygon->count() - 1);
}

void QGCMapPolygonTest::_testSegmentSplit()
{
    // Create polygon
    for (const QGeoCoordinate &vertex : std::as_const(_polyPoints)) {
        _mapPolygon->appendVertex(vertex);
    }

    QCOMPARE(_mapPolygon->selectedVertex(), -1);
    QCOMPARE(_mapPolygon->count(), _polyPoints.count());
    QCOMPARE(_mapPolygon->count(), 4);

    // Test deselect, select, deselect
    _mapPolygon->selectVertex(-1);
    QCOMPARE(_mapPolygon->selectedVertex(), -1);
    _mapPolygon->selectVertex(3);
    QCOMPARE(_mapPolygon->selectedVertex(), 3);
    _mapPolygon->selectVertex(-1);
    QCOMPARE(_mapPolygon->selectedVertex(), -1);

    // Test split at beginning, with no selected
    _mapPolygon->selectVertex(-1);
    _mapPolygon->splitPolygonSegment(0);
    QCOMPARE(_mapPolygon->count(), 5);
    QCOMPARE(_mapPolygon->selectedVertex(), -1);

    // Test split at beginning, with same idx selected
    _mapPolygon->selectVertex(0);
    _mapPolygon->splitPolygonSegment(0);
    QCOMPARE(_mapPolygon->count(), 6);
    QCOMPARE(_mapPolygon->selectedVertex(), 0);

    // Test split at beginning, with later idx selected
    _mapPolygon->selectVertex(1);
    _mapPolygon->splitPolygonSegment(0);
    QCOMPARE(_mapPolygon->count(), 7);
    QCOMPARE(_mapPolygon->selectedVertex(), 2);

    // Test split in middle, with no selected
    _mapPolygon->selectVertex(-1);
    _mapPolygon->splitPolygonSegment(2);
    QCOMPARE(_mapPolygon->count(), 8);
    QCOMPARE(_mapPolygon->selectedVertex(), -1);

    // Test split in middle, with earlier selected
    _mapPolygon->selectVertex(1);
    _mapPolygon->splitPolygonSegment(2);
    QCOMPARE(_mapPolygon->count(), 9);
    QCOMPARE(_mapPolygon->selectedVertex(), 1);

    // Test split in middle, with same selected
    _mapPolygon->selectVertex(2);
    _mapPolygon->splitPolygonSegment(2);
    QCOMPARE(_mapPolygon->count(), 10);
    QCOMPARE(_mapPolygon->selectedVertex(), 2);

    // Test split in middle, with later selected
    _mapPolygon->selectVertex(3);
    _mapPolygon->splitPolygonSegment(2);
    QCOMPARE(_mapPolygon->count(), 11);
    QCOMPARE(_mapPolygon->selectedVertex(), 4);

    // Test split at end, with no selected
    _mapPolygon->selectVertex(-1);
    _mapPolygon->splitPolygonSegment(_mapPolygon->count()-1);
    QCOMPARE(_mapPolygon->count(), 12);
    QCOMPARE(_mapPolygon->selectedVertex(), -1);

    // Test split at end, with earlier selected
    _mapPolygon->selectVertex(_mapPolygon->count()-2);
    _mapPolygon->splitPolygonSegment(_mapPolygon->count()-1);
    QCOMPARE(_mapPolygon->count(), 13);
    QCOMPARE(_mapPolygon->selectedVertex(), _mapPolygon->count()-3);

    // Test split at end, with same selected
    _mapPolygon->selectVertex(_mapPolygon->count()-1);
    _mapPolygon->splitPolygonSegment(_mapPolygon->count()-1);
    QCOMPARE(_mapPolygon->count(), 14);
    QCOMPARE(_mapPolygon->selectedVertex(), _mapPolygon->count()-2);
}
