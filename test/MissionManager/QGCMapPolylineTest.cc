#include "QGCMapPolylineTest.h"
#include "QGCQGeoCoordinate.h"
#include "MultiSignalSpy.h"
#include "QGCMapPolyline.h"
#include "QmlObjectListModel.h"

#include <QtTest/QTest>

void QGCMapPolylineTest::init()
{
    TempDirTest::init();

    _mapPolyline = new QGCMapPolyline(this);
    VERIFY_NOT_NULL(_mapPolyline);
    _pathModel = _mapPolyline->qmlPathModel();
    VERIFY_NOT_NULL(_pathModel);

    _multiSpyPolyline = new MultiSignalSpy(this);
    VERIFY_NOT_NULL(_multiSpyPolyline);
    QVERIFY(_multiSpyPolyline->init(_mapPolyline));
    _countChangedMask = _multiSpyPolyline->mask("countChanged");
    _pathChangedMask = _multiSpyPolyline->mask("pathChanged");
    _dirtyChangedMask = _multiSpyPolyline->mask("dirtyChanged");
    _isEmptyChangedMask = _multiSpyPolyline->mask("isEmptyChanged");
    _isValidChangedMask = _multiSpyPolyline->mask("isValidChanged");
    _clearedMask = _multiSpyPolyline->mask("cleared");

    _multiSpyModel = new MultiSignalSpy(this);
    VERIFY_NOT_NULL(_multiSpyModel);
    QVERIFY(_multiSpyModel->init(_pathModel));
    _modelCountChangedMask = _multiSpyModel->mask("countChanged");
    _modelDirtyChangedMask = _multiSpyModel->mask("dirtyChanged");
}

void QGCMapPolylineTest::cleanup()
{
    delete _multiSpyModel;
    _multiSpyModel = nullptr;
    delete _multiSpyPolyline;
    _multiSpyPolyline = nullptr;
    delete _mapPolyline;
    _mapPolyline = nullptr;
    _pathModel = nullptr;

    TempDirTest::cleanup();
}

void QGCMapPolylineTest::_testDirty()
{
    // Check basic dirty bit set/get
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_NO_SIGNALS(*_multiSpyPolyline);
    QVERIFY_NO_SIGNALS(*_multiSpyModel);

    _mapPolyline->setDirty(true);
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_ONLY_SIGNAL(*_multiSpyPolyline, "dirtyChanged");
    QVERIFY(_multiSpyPolyline->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_NO_SIGNALS(*_multiSpyModel);
    _multiSpyPolyline->clearAllSignals();

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_ONLY_SIGNAL(*_multiSpyPolyline, "dirtyChanged");
    QVERIFY(!_multiSpyPolyline->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_NO_SIGNALS(*_multiSpyModel);
    _multiSpyPolyline->clearAllSignals();

    _pathModel->setDirty(true);
    QVERIFY(_pathModel->dirty());
    QVERIFY(_mapPolyline->dirty());
    QVERIFY_ONLY_SIGNAL(*_multiSpyPolyline, "dirtyChanged");
    QVERIFY(_multiSpyPolyline->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_ONLY_SIGNAL(*_multiSpyModel, "dirtyChanged");
    QVERIFY(_multiSpyModel->pullBoolFromSignal("dirtyChanged"));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY_ONLY_SIGNAL(*_multiSpyPolyline, "dirtyChanged");
    QVERIFY(!_multiSpyPolyline->pullBoolFromSignal("dirtyChanged"));
    QVERIFY_ONLY_SIGNAL(*_multiSpyModel, "dirtyChanged");
    QVERIFY(!_multiSpyModel->pullBoolFromSignal("dirtyChanged"));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();
}

void QGCMapPolylineTest::_testVertexManipulation()
{
    // Vertex addition testing
    for (qsizetype i = 0; i < _linePoints.count(); i++) {
        QCOMPARE(_mapPolyline->count(), i);

        _mapPolyline->appendVertex(_linePoints[i]);
        QVERIFY(_multiSpyPolyline->waitForSignal("pathChanged"));
        QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(_pathChangedMask | _dirtyChangedMask | _countChangedMask | _isEmptyChangedMask | _isValidChangedMask));
        QVERIFY(_multiSpyModel->checkSignalByMask(_modelDirtyChangedMask | _modelCountChangedMask));
        QCOMPARE(_multiSpyPolyline->pullIntFromSignal("countChanged"), i+1);
        QCOMPARE(_multiSpyModel->pullIntFromSignal("countChanged"), i+1);

        QVERIFY(_mapPolyline->dirty());
        QVERIFY(_pathModel->dirty());

        QCOMPARE(_mapPolyline->count(), i+1);

        const QList<QGeoCoordinate> vertexList = _mapPolyline->coordinateList();
        QCOMPARE(vertexList.count(), i+1);
        QCOMPARE(vertexList[i], _linePoints[i]);

        QCOMPARE(_pathModel->count(), i+1);
        QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(i)->coordinate(), _linePoints[i]);

        _mapPolyline->setDirty(false);
        _multiSpyPolyline->clearAllSignals();
        _multiSpyModel->clearAllSignals();
    }

    // Vertex adjustment testing
    QGCQGeoCoordinate *geoCoord = _pathModel->value<QGCQGeoCoordinate*>(1);
    VERIFY_NOT_NULL(geoCoord);
    MultiSignalSpy *multiSpyGeoCoord = new MultiSignalSpy(this);
    VERIFY_NOT_NULL(multiSpyGeoCoord);
    QVERIFY(multiSpyGeoCoord->init(geoCoord));

    QGeoCoordinate adjustCoord(_linePoints[1].latitude() + 1, _linePoints[1].longitude() + 1);
    _mapPolyline->adjustVertex(1, adjustCoord);
    QVERIFY(_multiSpyPolyline->waitForSignal("pathChanged"));
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(_pathChangedMask | _dirtyChangedMask));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(_modelDirtyChangedMask));

    QVERIFY(multiSpyGeoCoord->checkSignalByMask(multiSpyGeoCoord->mask("coordinateChanged")));
    QVERIFY(multiSpyGeoCoord->checkSignalByMask(multiSpyGeoCoord->mask("dirtyChanged")));

    QCOMPARE(geoCoord->coordinate(), adjustCoord);
    QList<QGeoCoordinate> vertexList = _mapPolyline->coordinateList();
    QCOMPARE(vertexList[0], _linePoints[0]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(0)->coordinate(), _linePoints[0]);
    QCOMPARE(vertexList[2], _linePoints[2]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(2)->coordinate(), _linePoints[2]);
    QCOMPARE(vertexList[3], _linePoints[3]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(3)->coordinate(), _linePoints[3]);

    _mapPolyline->setDirty(false);
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    // Vertex removal testing
    _mapPolyline->removeVertex(1);
    QVERIFY(_multiSpyPolyline->checkOnlySignalByMask(_pathChangedMask | _dirtyChangedMask | _countChangedMask | _isEmptyChangedMask | _isValidChangedMask));
    QVERIFY(_multiSpyModel->checkSignalByMask(_modelDirtyChangedMask | _modelCountChangedMask));
    QCOMPARE(_mapPolyline->count(), 3);
    vertexList = _mapPolyline->coordinateList();
    QCOMPARE(vertexList.count(), 3);
    QCOMPARE(_pathModel->count(), 3);
    QCOMPARE(vertexList[0], _linePoints[0]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(0)->coordinate(), _linePoints[0]);
    QCOMPARE(vertexList[1], _linePoints[2]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(1)->coordinate(), _linePoints[2]);
    QCOMPARE(vertexList[2], _linePoints[3]);
    QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(2)->coordinate(), _linePoints[3]);

    // Clear testing
    _mapPolyline->clear();
    QVERIFY(_multiSpyPolyline->checkOnlySignalsByMask(_pathChangedMask | _dirtyChangedMask | _countChangedMask | _isEmptyChangedMask | _isValidChangedMask | _clearedMask));
    QVERIFY(_multiSpyModel->checkSignalsByMask(_modelDirtyChangedMask | _modelCountChangedMask));
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(_pathModel->dirty());
    QCOMPARE(_mapPolyline->count(), 0);
    vertexList = _mapPolyline->coordinateList();
    QCOMPARE(vertexList.count(), 0);
    QCOMPARE(_pathModel->count(), 0);
}

QString QGCMapPolylineTest::_copyRes(const QString &name)
{
    const QString dstPath = tempFilePath(name);
    (void)QFile::remove(dstPath);
    const QString resPath = QStringLiteral(":/unittest/%1").arg(name);
    (void)QFile(resPath).copy(dstPath);
    return dstPath;
}

void QGCMapPolylineTest::_testShapeLoad()
{
    (void) _copyRes("pline.dbf");
    (void) _copyRes("pline.shx");
    (void) _copyRes("pline.prj");
    const QString shpFile = _copyRes("pline.shp");
    QVERIFY(_mapPolyline->loadKMLOrSHPFile(shpFile));

    const QString kmlFile = _copyRes("polyline.kml");
    QVERIFY(_mapPolyline->loadKMLOrSHPFile(kmlFile));
}

void QGCMapPolylineTest::_testSelectVertex()
{
    // Create polyline
    for (const QGeoCoordinate &vertex : std::as_const(_linePoints)) {
        _mapPolyline->appendVertex(vertex);
    }

    QCOMPARE(_mapPolyline->selectedVertex(), -1);
    QCOMPARE(_mapPolyline->count(), _linePoints.count());

    // Test deselect
    _mapPolyline->selectVertex(-1);
    QCOMPARE(_mapPolyline->selectedVertex(), -1);

    // Test out of bounds
    _mapPolyline->selectVertex(_linePoints.count());
    QCOMPARE(_mapPolyline->selectedVertex(), -1);

    // Simple select test
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    QCOMPARE(_mapPolyline->selectedVertex(), _linePoints.count() - 1);

    // Keep selected test
    _mapPolyline->selectVertex(0);
    _mapPolyline->removeVertex(_linePoints.count() - 1);
    QCOMPARE(_mapPolyline->selectedVertex(), 0);

    // Deselect if selected vertex removed
    _mapPolyline->appendVertex(_linePoints[_linePoints.count() - 1]);
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    _mapPolyline->removeVertex(_linePoints.count() - 1);
    QCOMPARE(_mapPolyline->selectedVertex(), -1);

    // Shift selected index down if removed index < selected index
    _mapPolyline->appendVertex(_linePoints[_linePoints.count() - 1]);
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    _mapPolyline->removeVertex(0);
    QCOMPARE(_mapPolyline->selectedVertex(), _mapPolyline->count() - 1);
}
