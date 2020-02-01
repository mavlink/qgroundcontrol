/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMapPolygonTest.h"
#include "QGCApplication.h"
#include "QGCQGeoCoordinate.h"

QGCMapPolygonTest::QGCMapPolygonTest(void)
{
    _polyPoints << QGeoCoordinate(47.635638361473475, -122.09269407980834 ) <<
                   QGeoCoordinate(47.635638361473475, -122.08545246602667) <<
                   QGeoCoordinate(47.63057923872075, -122.08545246602667) <<
                   QGeoCoordinate(47.63057923872075, -122.09269407980834);
}

void QGCMapPolygonTest::init(void)
{
    UnitTest::init();

    _rgPolygonSignals[polygonCountChangedIndex] =   SIGNAL(countChanged(int));
    _rgPolygonSignals[pathChangedIndex] =           SIGNAL(pathChanged());
    _rgPolygonSignals[polygonDirtyChangedIndex] =   SIGNAL(dirtyChanged(bool));
    _rgPolygonSignals[clearedIndex] =               SIGNAL(cleared());
    _rgPolygonSignals[centerChangedIndex] =         SIGNAL(centerChanged(QGeoCoordinate));

    _rgModelSignals[modelCountChangedIndex] = SIGNAL(countChanged(int));
    _rgModelSignals[modelDirtyChangedIndex] = SIGNAL(dirtyChanged(bool));

    _mapPolygon = new QGCMapPolygon(this);
    _pathModel = _mapPolygon->qmlPathModel();
    QVERIFY(_pathModel);

    _multiSpyPolygon = new MultiSignalSpy();
    QCOMPARE(_multiSpyPolygon->init(_mapPolygon, _rgPolygonSignals, _cPolygonSignals), true);

    _multiSpyModel = new MultiSignalSpy();
    QCOMPARE(_multiSpyModel->init(_pathModel, _rgModelSignals, _cModelSignals), true);
}

void QGCMapPolygonTest::cleanup(void)
{
    delete _mapPolygon;
    delete _multiSpyPolygon;
    delete _multiSpyModel;
}

void QGCMapPolygonTest::_testDirty(void)
{
    // Check basic dirty bit set/get

    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());

    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->checkNoSignals());
    QVERIFY(_multiSpyModel->checkNoSignals());

    _mapPolygon->setDirty(true);
    QVERIFY(_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->checkOnlySignalByMask(polygonDirtyChangedMask));
    QVERIFY(_multiSpyPolygon->pullBoolFromSignalIndex(polygonDirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkNoSignals());
    _multiSpyPolygon->clearAllSignals();

    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->checkOnlySignalByMask(polygonDirtyChangedMask));
    QVERIFY(!_multiSpyPolygon->pullBoolFromSignalIndex(polygonDirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkNoSignals());
    _multiSpyPolygon->clearAllSignals();

    _pathModel->setDirty(true);
    QVERIFY(_pathModel->dirty());
    QVERIFY(_mapPolygon->dirty());
    QVERIFY(_multiSpyPolygon->checkOnlySignalByMask(polygonDirtyChangedMask));
    QVERIFY(_multiSpyPolygon->pullBoolFromSignalIndex(polygonDirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask));
    QVERIFY(_multiSpyModel->pullBoolFromSignalIndex(modelDirtyChangedIndex));
    _multiSpyPolygon->clearAllSignals();
    _multiSpyModel->clearAllSignals();

    _mapPolygon->setDirty(false);
    QVERIFY(!_mapPolygon->dirty());
    QVERIFY(!_pathModel->dirty());
    QVERIFY(_multiSpyPolygon->checkOnlySignalByMask(polygonDirtyChangedMask));
    QVERIFY(!_multiSpyPolygon->pullBoolFromSignalIndex(polygonDirtyChangedIndex));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask));
    QVERIFY(!_multiSpyModel->pullBoolFromSignalIndex(modelDirtyChangedIndex));
    _multiSpyPolygon->clearAllSignals();
    _multiSpyModel->clearAllSignals();
}

void QGCMapPolygonTest::_testVertexManipulation(void)
{
#if 0
    Q_INVOKABLE void clear(void);
    Q_INVOKABLE void appendVertex(const QGeoCoordinate& coordinate);
    Q_INVOKABLE void removeVertex(int vertexIndex);

    /// Adjust the value for the specified coordinate
    ///     @param vertexIndex Polygon point index to modify (0-based)
    ///     @param coordinate New coordinate for point
    Q_INVOKABLE void adjustVertex(int vertexIndex, const QGeoCoordinate coordinate);

    /// Splits the segment comprised of vertextIndex -> vertexIndex + 1
    Q_INVOKABLE void splitPolygonSegment(int vertexIndex);
#endif

    // Vertex addition testing

    for (int i=0; i<_polyPoints.count(); i++) {
        QCOMPARE(_mapPolygon->count(), i);

        _mapPolygon->appendVertex(_polyPoints[i]);
        if (i >= 2) {
            // Center is no recalculated until there are 3 points or more
            QVERIFY(_multiSpyPolygon->checkOnlySignalByMask(pathChangedMask | polygonDirtyChangedMask | polygonCountChangedMask | centerChangedMask));
        } else {
            QVERIFY(_multiSpyPolygon->checkOnlySignalByMask(pathChangedMask | polygonDirtyChangedMask | polygonCountChangedMask));
        }
        QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask | modelCountChangedMask));
        QCOMPARE(_multiSpyPolygon->pullIntFromSignalIndex(polygonCountChangedIndex), i+1);
        QCOMPARE(_multiSpyModel->pullIntFromSignalIndex(modelCountChangedIndex), i+1);

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
    QSignalSpy coordSpy(geoCoord, SIGNAL(coordinateChanged(QGeoCoordinate)));
    QSignalSpy coordDirtySpy(geoCoord, SIGNAL(dirtyChanged(bool)));
    QGeoCoordinate adjustCoord(_polyPoints[1].latitude() + 1, _polyPoints[1].longitude() + 1);
    _mapPolygon->adjustVertex(1, adjustCoord);
    QVERIFY(_multiSpyPolygon->checkOnlySignalByMask(pathChangedMask | polygonDirtyChangedMask | centerChangedMask));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask));
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
    QVERIFY(_multiSpyPolygon->checkOnlySignalsByMask(pathChangedMask | polygonDirtyChangedMask | polygonCountChangedMask | centerChangedMask));
    QVERIFY(_multiSpyModel->checkOnlySignalByMask(modelDirtyChangedMask | modelCountChangedMask));
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
    QVERIFY(_multiSpyPolygon->checkOnlySignalsByMask(pathChangedMask | polygonDirtyChangedMask | polygonCountChangedMask | centerChangedMask | clearedMask));
    QVERIFY(_multiSpyModel->checkOnlySignalsByMask(modelDirtyChangedMask | modelCountChangedMask));
    QVERIFY(_mapPolygon->dirty());
    QVERIFY(_pathModel->dirty());
    QCOMPARE(_mapPolygon->count(), 0);
    polyList = _mapPolygon->path();
    QCOMPARE(polyList.count(), 0);
    QCOMPARE(_pathModel->count(), 0);
}

void QGCMapPolygonTest::_testKMLLoad(void)
{
    QVERIFY(_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonGood.kml")));

    setExpectedMessageBox(QMessageBox::Ok);
    QVERIFY(!_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonBadXml.kml")));
    checkExpectedMessageBox();

    setExpectedMessageBox(QMessageBox::Ok);
    QVERIFY(!_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonMissingNode.kml")));
    checkExpectedMessageBox();

    setExpectedMessageBox(QMessageBox::Ok);
    QVERIFY(!_mapPolygon->loadKMLOrSHPFile(QStringLiteral(":/unittest/PolygonBadCoordinatesNode.kml")));
    checkExpectedMessageBox();
}
