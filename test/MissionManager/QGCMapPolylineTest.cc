#include "QGCMapPolylineTest.h"

#include "MultiSignalSpy.h"
#include "QGCMapPolyline.h"
#include "QGCQGeoCoordinate.h"
#include "QmlObjectListModel.h"

void QGCMapPolylineTest::init()
{
    UnitTest::init();
    _mapPolyline = new QGCMapPolyline(this);
    _pathModel = _mapPolyline->qmlPathModel();
    QVERIFY(_pathModel);
    _multiSpyPolyline = new MultiSignalSpy(this);
    QVERIFY(_multiSpyPolyline->init(_mapPolyline));
    _multiSpyModel = new MultiSignalSpy(this);
    QVERIFY(_multiSpyModel->init(_pathModel));
}

void QGCMapPolylineTest::cleanup()
{
    delete _multiSpyModel;
    _multiSpyModel = nullptr;
    delete _multiSpyPolyline;
    _multiSpyPolyline = nullptr;
    delete _mapPolyline;
    _mapPolyline = nullptr;
    UnitTest::cleanup();
}

void QGCMapPolylineTest::_testDirty()
{
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());
    _mapPolyline->setDirty(true);
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->noneEmitted());
    _multiSpyPolyline->clearAllSignals();
    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->noneEmitted());
    _multiSpyPolyline->clearAllSignals();
    _pathModel->setDirty(true);
    QVERIFY(_pathModel->dirty());
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(_multiSpyPolyline->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpyModel->argument<bool>("dirtyChanged"));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();
    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpyModel->argument<bool>("dirtyChanged"));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();
}

void QGCMapPolylineTest::_testVertexManipulation()
{
    for (qsizetype i = 0; i < _linePoints.count(); i++) {
        QCOMPARE(_mapPolyline->count(), i);
        _mapPolyline->appendVertex(_linePoints[i]);
        QTest::qWait(100);
        QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_multiSpyPolyline->mask(
            "pathChanged", "dirtyChanged", "countChanged", "isEmptyChanged", "isValidChanged")));
        QVERIFY(_multiSpyModel->emittedOnceByMask(_multiSpyModel->mask("dirtyChanged", "countChanged")));
        QCOMPARE(_multiSpyPolyline->argument<int>("countChanged"), static_cast<int>(i + 1));
        QCOMPARE(_multiSpyModel->argument<int>("countChanged"), static_cast<int>(i + 1));
        QVERIFY(_mapPolyline->dirty());
        QVERIFY(_pathModel->dirty());
        QCOMPARE(_mapPolyline->count(), i + 1);
        const QList<QGeoCoordinate> vertexList = _mapPolyline->coordinateList();
        QCOMPARE(vertexList.count(), i + 1);
        QCOMPARE(vertexList[i], _linePoints[i]);
        QCOMPARE(_pathModel->count(), i + 1);
        QCOMPARE(_pathModel->value<QGCQGeoCoordinate*>(i)->coordinate(), _linePoints[i]);
        _mapPolyline->setDirty(false);
        _multiSpyPolyline->clearAllSignals();
        _multiSpyModel->clearAllSignals();
    }
    // Vertex adjustment testing
    QGCQGeoCoordinate* geoCoord = _pathModel->value<QGCQGeoCoordinate*>(1);
    MultiSignalSpy* multiSpyGeoCoord = new MultiSignalSpy(this);
    QVERIFY(multiSpyGeoCoord->init(geoCoord));
    QGeoCoordinate adjustCoord(_linePoints[1].latitude() + 1, _linePoints[1].longitude() + 1);
    _mapPolyline->adjustVertex(1, adjustCoord);
    QTest::qWait(100);
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_multiSpyPolyline->mask("pathChanged", "dirtyChanged")));
    QVERIFY(_multiSpyModel->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(multiSpyGeoCoord->emittedOnce("coordinateChanged"));
    QVERIFY(multiSpyGeoCoord->emittedOnce("dirtyChanged"));
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
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(
        _multiSpyPolyline->mask("pathChanged", "dirtyChanged", "countChanged", "isEmptyChanged", "isValidChanged")));
    QVERIFY(_multiSpyModel->emittedOnceByMask(_multiSpyModel->mask("dirtyChanged", "countChanged")));
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
    QVERIFY(_multiSpyPolyline->onlyEmittedByMask(_multiSpyPolyline->mask(
        "pathChanged", "dirtyChanged", "countChanged", "isEmptyChanged", "isValidChanged", "cleared")));
    QVERIFY(_multiSpyModel->emittedByMask(_multiSpyModel->mask("dirtyChanged", "countChanged")));
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(_pathModel->dirty());
    QCOMPARE(_mapPolyline->count(), 0);
    vertexList = _mapPolyline->coordinateList();
    QCOMPARE(vertexList.count(), 0);
    QCOMPARE(_pathModel->count(), 0);
    delete multiSpyGeoCoord;
}

QString QGCMapPolylineTest::_copyRes(const QTemporaryDir& tmpDir, const QString& name)
{
    const QString dstPath = tmpDir.filePath(name);
    (void)QFile::remove(dstPath);
    const QString resPath = QStringLiteral(":/unittest/%1").arg(name);
    (void)QFile(resPath).copy(dstPath);
    return dstPath;
}

void QGCMapPolylineTest::_testShapeLoad()
{
    const QTemporaryDir tmpDir;
    (void)_copyRes(tmpDir, "pline.dbf");
    (void)_copyRes(tmpDir, "pline.shx");
    (void)_copyRes(tmpDir, "pline.prj");
    const QString shpFile = _copyRes(tmpDir, "pline.shp");
    QVERIFY(_mapPolyline->loadKMLOrSHPFile(shpFile));
    const QString kmlFile = _copyRes(tmpDir, "polyline.kml");
    QVERIFY(_mapPolyline->loadKMLOrSHPFile(kmlFile));
}

void QGCMapPolylineTest::_testSelectVertex()
{
    for (const QGeoCoordinate& vertex : std::as_const(_linePoints)) {
        _mapPolyline->appendVertex(vertex);
    }
    QVERIFY(_mapPolyline->selectedVertex() == -1);
    QVERIFY(_mapPolyline->count() == _linePoints.count());
    _mapPolyline->selectVertex(-1);
    QVERIFY(_mapPolyline->selectedVertex() == -1);
    _mapPolyline->selectVertex(_linePoints.count());
    QVERIFY(_mapPolyline->selectedVertex() == -1);
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    QVERIFY(_mapPolyline->selectedVertex() == (_linePoints.count() - 1));
    _mapPolyline->selectVertex(0);
    _mapPolyline->removeVertex(_linePoints.count() - 1);
    QVERIFY(_mapPolyline->selectedVertex() == 0);
    _mapPolyline->appendVertex(_linePoints[_linePoints.count() - 1]);
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    _mapPolyline->removeVertex(_linePoints.count() - 1);
    QVERIFY(_mapPolyline->selectedVertex() == -1);
    _mapPolyline->appendVertex(_linePoints[_linePoints.count() - 1]);
    _mapPolyline->selectVertex(_linePoints.count() - 1);
    _mapPolyline->removeVertex(0);
    QVERIFY(_mapPolyline->selectedVertex() == (_mapPolyline->count() - 1));
}

UT_REGISTER_TEST(QGCMapPolylineTest, TestLabel::Unit, TestLabel::MissionManager)
