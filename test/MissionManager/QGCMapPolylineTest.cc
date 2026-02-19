#include "QGCMapPolylineTest.h"
#include "QGCQGeoCoordinate.h"
#include "MultiSignalSpy.h"
#include "QGCMapPolyline.h"
#include "QmlObjectListModel.h"

#include <QtTest/QTest>

void QGCMapPolylineTest::init()
{
    UnitTest::init();

    _mapPolyline = new QGCMapPolyline(this);
    _pathModel = _mapPolyline->qmlPathModel();
    QVERIFY(_pathModel);

    _multiSpyPolyline = new MultiSignalSpy(this);
    QVERIFY(_multiSpyPolyline->init(_mapPolyline));
    _countChangedMask = _multiSpyPolyline->mask("countChanged");
    _pathChangedMask = _multiSpyPolyline->mask("pathChanged");
    _dirtyChangedMask = _multiSpyPolyline->mask("dirtyChanged");
    _isEmptyChangedMask = _multiSpyPolyline->mask("isEmptyChanged");
    _isValidChangedMask = _multiSpyPolyline->mask("isValidChanged");
    _clearedMask = _multiSpyPolyline->mask("cleared");

    _multiSpyModel = new MultiSignalSpy(this);
    QVERIFY(_multiSpyModel->init(_pathModel));
    _modelCountChangedMask = _multiSpyModel->mask("countChanged");
    _modelDirtyChangedMask = _multiSpyModel->mask("dirtyChanged");
}

void QGCMapPolylineTest::cleanup()
{
    UnitTest::cleanup();

    delete _multiSpyModel;
    _multiSpyModel = nullptr;
    delete _multiSpyPolyline;
    _multiSpyPolyline = nullptr;
    delete _mapPolyline;
    _mapPolyline = nullptr;
}

void QGCMapPolylineTest::_testDirty()
{
    // Check basic dirty bit set/get
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
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_dirtyChangedMask));
    QVERIFY(_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->noneEmitted());
    _multiSpyPolyline->clearAllSignals();

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_dirtyChangedMask));
    QVERIFY(!_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->noneEmitted());
    _multiSpyPolyline->clearAllSignals();

    _pathModel->setDirty(true);
    QVERIFY(_pathModel->dirty());
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_dirtyChangedMask));
    QVERIFY(_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->onlyEmittedOnceByMask(_modelDirtyChangedMask));
    QVERIFY(_multiSpyModel->argument<bool>("dirtyChanged"));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolyline->setDirty(false);
    QVERIFY(!_mapPolyline->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_dirtyChangedMask));
    QVERIFY(!_multiSpyPolyline->argument<bool>("dirtyChanged"));
    QVERIFY(_multiSpyModel->onlyEmittedOnceByMask(_modelDirtyChangedMask));
    QVERIFY(!_multiSpyModel->argument<bool>("dirtyChanged"));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();
}

void QGCMapPolylineTest::_testVertexManipulation()
{
    // Vertex addition testing
    for (qsizetype i = 0; i < _linePoints.count(); i++) {
        QCOMPARE(_mapPolyline->count(), i);

        _mapPolyline->appendVertex(_linePoints[i]);
        QTest::qWait(100); // Let event loop process so queued signals flow through
        QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_pathChangedMask | _dirtyChangedMask | _countChangedMask | _isEmptyChangedMask | _isValidChangedMask));
        QVERIFY(_multiSpyModel->emittedByMask(_modelDirtyChangedMask | _modelCountChangedMask));
        QCOMPARE(_multiSpyPolyline->argument<int>("countChanged"), static_cast<int>(i+1));
        QCOMPARE(_multiSpyModel->argument<int>("countChanged"), static_cast<int>(i+1));

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
    MultiSignalSpy multiSpyGeoCoord(this);
    QVERIFY(multiSpyGeoCoord.init(geoCoord));

    QGeoCoordinate adjustCoord(_linePoints[1].latitude() + 1, _linePoints[1].longitude() + 1);
    _mapPolyline->adjustVertex(1, adjustCoord);
    QTest::qWait(100); // Let event loop process so queued signals flow through
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_pathChangedMask | _dirtyChangedMask));
    QVERIFY(_multiSpyModel->onlyEmittedOnceByMask(_modelDirtyChangedMask));

    QVERIFY(multiSpyGeoCoord.emitted("coordinateChanged"));
    QVERIFY(multiSpyGeoCoord.emitted("dirtyChanged"));

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
    QVERIFY(_multiSpyPolyline->onlyEmittedOnceByMask(_pathChangedMask | _dirtyChangedMask | _countChangedMask | _isEmptyChangedMask | _isValidChangedMask));
    QVERIFY(_multiSpyModel->emittedByMask(_modelDirtyChangedMask | _modelCountChangedMask));
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
    QVERIFY(_multiSpyPolyline->onlyEmittedByMask(_pathChangedMask | _dirtyChangedMask | _countChangedMask | _isEmptyChangedMask | _isValidChangedMask | _clearedMask));
    QVERIFY(_multiSpyModel->emittedByMask(_modelDirtyChangedMask | _modelCountChangedMask));
    QVERIFY(_mapPolyline->dirty());
    QVERIFY(_pathModel->dirty());
    QCOMPARE(_mapPolyline->count(), 0);
    vertexList = _mapPolyline->coordinateList();
    QCOMPARE(vertexList.count(), 0);
    QCOMPARE(_pathModel->count(), 0);
}

QString QGCMapPolylineTest::_copyRes(const QTemporaryDir &tmpDir, const QString &name)
{
    const QString dstPath = tmpDir.filePath(name);
    (void) QFile::remove(dstPath);
    const QString resPath = QStringLiteral(":/unittest/%1").arg(name);
    (void) QFile(resPath).copy(dstPath);
    return dstPath;
}

void QGCMapPolylineTest::_testShapeLoad()
{
    const QTemporaryDir tmpDir;

    (void) _copyRes(tmpDir, "pline.dbf");
    (void) _copyRes(tmpDir, "pline.shx");
    (void) _copyRes(tmpDir, "pline.prj");
    const QString shpFile = _copyRes(tmpDir, "pline.shp");
    QVERIFY(_mapPolyline->loadShapeFile(shpFile));

    const QString kmlFile = _copyRes(tmpDir, "polyline.kml");
    QVERIFY(_mapPolyline->loadShapeFile(kmlFile));
}

void QGCMapPolylineTest::_testSelectVertex()
{
    // Create polyline
    for (const QGeoCoordinate &vertex : std::as_const(_linePoints)) {
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
    QVERIFY(_mapPolyline->selectedVertex() == (_linePoints.count() - 1));

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
    QVERIFY(_mapPolyline->selectedVertex() == (_mapPolyline->count() - 1));
}

void QGCMapPolylineTest::_testRemoveVertexOutOfRange()
{
    _mapPolyline->setPath(_linePoints);
    _mapPolyline->selectVertex(1);
    QTest::qWait(100);
    QVERIFY(_multiSpyPolyline->emittedByMask(_pathChangedMask));
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolyline->removeVertex(-1);
    QCOMPARE(_mapPolyline->count(), _linePoints.count());
    QCOMPARE(_pathModel->count(), _linePoints.count());
    QCOMPARE(_mapPolyline->selectedVertex(), 1);
    QVERIFY(_multiSpyPolyline->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());

    _mapPolyline->removeVertex(_linePoints.count());
    QCOMPARE(_mapPolyline->count(), _linePoints.count());
    QCOMPARE(_pathModel->count(), _linePoints.count());
    QCOMPARE(_mapPolyline->selectedVertex(), 1);
    QVERIFY(_multiSpyPolyline->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());
}

void QGCMapPolylineTest::_testAdjustVertexOutOfRange()
{
    _mapPolyline->setPath(_linePoints);
    _mapPolyline->selectVertex(1);
    QTest::qWait(100);
    QVERIFY(_multiSpyPolyline->emittedByMask(_pathChangedMask));
    const QVariantList initialPath = _mapPolyline->path();
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolyline->adjustVertex(-1, QGeoCoordinate(1.0, 1.0));
    QCOMPARE(_mapPolyline->count(), _linePoints.count());
    QCOMPARE(_pathModel->count(), _linePoints.count());
    QCOMPARE(_mapPolyline->selectedVertex(), 1);
    QCOMPARE(_mapPolyline->path(), initialPath);
    QVERIFY(_multiSpyPolyline->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());

    _mapPolyline->adjustVertex(_linePoints.count(), QGeoCoordinate(2.0, 2.0));
    QCOMPARE(_mapPolyline->count(), _linePoints.count());
    QCOMPARE(_pathModel->count(), _linePoints.count());
    QCOMPARE(_mapPolyline->selectedVertex(), 1);
    QCOMPARE(_mapPolyline->path(), initialPath);
    QVERIFY(_multiSpyPolyline->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());
}

void QGCMapPolylineTest::_testSplitSegmentOutOfRange()
{
    _mapPolyline->setPath(_linePoints);
    _mapPolyline->selectVertex(1);
    QTest::qWait(100);
    QVERIFY(_multiSpyPolyline->emittedByMask(_pathChangedMask));
    const QVariantList initialPath = _mapPolyline->path();
    _multiSpyPolyline->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolyline->splitSegment(-1);
    QCOMPARE(_mapPolyline->count(), _linePoints.count());
    QCOMPARE(_pathModel->count(), _linePoints.count());
    QCOMPARE(_mapPolyline->selectedVertex(), 1);
    QCOMPARE(_mapPolyline->path(), initialPath);
    QVERIFY(_multiSpyPolyline->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());

    _mapPolyline->splitSegment(_linePoints.count());
    QCOMPARE(_mapPolyline->count(), _linePoints.count());
    QCOMPARE(_pathModel->count(), _linePoints.count());
    QCOMPARE(_mapPolyline->selectedVertex(), 1);
    QCOMPARE(_mapPolyline->path(), initialPath);
    QVERIFY(_multiSpyPolyline->noneEmitted());
    QVERIFY(_multiSpyModel->noneEmitted());
}

void QGCMapPolylineTest::_testSegmentSplit()
{
    for (const QGeoCoordinate& vertex : std::as_const(_linePoints)) {
        _mapPolyline->appendVertex(vertex);
    }

    QVERIFY(_mapPolyline->selectedVertex() == -1);
    QVERIFY(_mapPolyline->count() == _linePoints.count());
    QVERIFY(_mapPolyline->count() == 4);

    // Split in middle, with no selected
    _mapPolyline->selectVertex(-1);
    _mapPolyline->splitSegment(1);
    QVERIFY(_mapPolyline->count() == 5);
    QVERIFY(_mapPolyline->selectedVertex() == -1);

    // Split in middle, with same index selected
    _mapPolyline->selectVertex(1);
    _mapPolyline->splitSegment(1);
    QVERIFY(_mapPolyline->count() == 6);
    QVERIFY(_mapPolyline->selectedVertex() == 1);

    // Split in middle, with later index selected
    _mapPolyline->selectVertex(2);
    _mapPolyline->splitSegment(1);
    QVERIFY(_mapPolyline->count() == 7);
    QVERIFY(_mapPolyline->selectedVertex() == 3);

    // Split at end for open path should be no-op
    const int oldCount = _mapPolyline->count();
    _mapPolyline->selectVertex(oldCount - 1);
    _mapPolyline->splitSegment(oldCount - 1);
    QVERIFY(_mapPolyline->count() == oldCount);
    QVERIFY(_mapPolyline->selectedVertex() == oldCount - 1);
}

UT_REGISTER_TEST(QGCMapPolylineTest, TestLabel::Unit, TestLabel::MissionManager)
