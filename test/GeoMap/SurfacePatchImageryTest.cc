#include "SurfacePatchImageryTest.h"

#include <QtTest/QSignalSpy>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "QGCMapUrlEngine.h"
#include "SurfacePatchModel.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

// The unit-test tile generator serves a placeholder image on every cache miss
QString mapType()
{
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            return type;
        }
    }
    return QString();
}

void attach(SurfacePatchModel& model, GeoScene& scene, GeoMapCamera& camera)
{
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    scene.setCamera(&camera);
    model.setScene(&scene);
}

int rowsWithImage(const SurfacePatchModel& model)
{
    int count = 0;
    for (int row = 0; row < model.rowCount(); row++) {
        if (!model.data(model.index(row), SurfacePatchModel::TileImageRole).value<QImage>().isNull()) {
            count++;
        }
    }
    return count;
}

int rowsReportingImage(const SurfacePatchModel& model)
{
    int count = 0;
    for (int row = 0; row < model.rowCount(); row++) {
        if (model.data(model.index(row), SurfacePatchModel::HasTileImageRole).toBool()) {
            count++;
        }
    }
    return count;
}

}  // namespace

void SurfacePatchImageryTest::_imagesArriveForAllPatches()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);

    QCOMPARE_GT(model.rowCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);
}

void SurfacePatchImageryTest::_emptyMapTypeDisablesImagery()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Clearing the map type drops all imagery and stops requesting
    model.setMapType(QString());
    QCOMPARE(rowsWithImage(model), 0);

    // Patch churn with imagery disabled stays image-free
    camera.setCenter(QGeoCoordinate(47.42, 8.58));
    QCOMPARE(rowsWithImage(model), 0);
}

void SurfacePatchImageryTest::_mapTypeSwitchRefetches()
{
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    QStringList types;
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            types.append(type);
        }
        if (types.count() == 2) {
            break;
        }
    }
    QVERIFY2(types.count() == 2, "need two non-elevation map providers");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(types.first());
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Provider switch clears delivered images, then refetches for the new provider
    model.setMapType(types.last());
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);
    QCOMPARE(model.mapType(), types.last());
}

void SurfacePatchImageryTest::_fallbackCoversLodChanges()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Zoom in (LOD refines): every new patch lies inside previously imaged
    // ground and must immediately drape an ancestor fallback -- no flash
    camera.setDistance(500);
    QCOMPARE_GT(model.rowCount(), 0);
    QCOMPARE(rowsReportingImage(model), model.rowCount());
    QCOMPARE(rowsWithImage(model), model.rowCount());

    // Zoom back out to the original LOD: the retired originals revive from
    // the cache without any new fetch
    camera.setDistance(2000);
    QCOMPARE_GT(model.rowCount(), 0);
    QCOMPARE(rowsReportingImage(model), model.rowCount());

    // Coarsen further: patches over previously seen ground composite their
    // cached children. Edge patches cover never-seen ground and legitimately
    // have no fallback source, so only partial coverage is guaranteed.
    camera.setDistance(4000);
    QCOMPARE_GT(model.rowCount(), 0);
    QCOMPARE_GT(rowsReportingImage(model), 0);
    QCOMPARE(rowsWithImage(model), rowsReportingImage(model));
}

UT_REGISTER_TEST(SurfacePatchImageryTest, TestLabel::Integration)
