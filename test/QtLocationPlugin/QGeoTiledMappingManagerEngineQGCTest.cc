#include "QGeoTiledMappingManagerEngineQGCTest.h"

#include <QtLocation/private/qgeomaptype_p.h>
#include <QtNetwork/QNetworkAccessManager>

#include "QGCMapUrlEngine.h"
#include "QGeoMapIdQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"

void QGeoTiledMappingManagerEngineQGCTest::_testElevationProvidersAreNotMapTypes()
{
    QNetworkAccessManager networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    const QGeoTiledMappingManagerEngineQGC engine({}, &error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    QVERIFY(!elevationTypes.isEmpty());

    // A map showing an elevation provider would request map tiles from it, which it does not have
    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    QCOMPARE(mapTypes.size(), UrlFactory::getProviderTypes().size() - elevationTypes.size());
    for (const QGeoMapType& mapType : mapTypes) {
        QVERIFY2(!elevationTypes.contains(mapType.name()), qPrintable(mapType.name()));
    }
}

void QGeoTiledMappingManagerEngineQGCTest::_testMapTypeIdsAreSequential()
{
    QNetworkAccessManager networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    const QGeoTiledMappingManagerEngineQGC engine({}, &error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    QVERIFY(!mapTypes.isEmpty());
    for (qsizetype i = 0; i < mapTypes.size(); i++) {
        QCOMPARE(mapTypes[i].mapId(), static_cast<int>(i) + 1);
        // A tile QtLocation requests for the map type is fetched from the provider the map type names
        QCOMPARE(UrlFactory::getProviderTypeFromMapId(QGeoMapIdQGC::toProviderId(mapTypes[i].mapId())),
                 mapTypes[i].name());
    }
}

void QGeoTiledMappingManagerEngineQGCTest::_testMapIdTranslationWithGaps()
{
    // The map providers of a build without those of ids 1 to 5 and 8
    const QList<int> providerIds = {6, 7, 9, 39};

    for (qsizetype i = 0; i < providerIds.size(); i++) {
        const int qtMapId = static_cast<int>(i) + 1;
        QCOMPARE(QGeoMapIdQGC::toProviderId(qtMapId, providerIds), providerIds[i]);
        QCOMPARE(QGeoMapIdQGC::fromProviderId(providerIds[i], providerIds), qtMapId);
    }

    QCOMPARE(QGeoMapIdQGC::toProviderId(0, providerIds), -1);
    QCOMPARE(QGeoMapIdQGC::toProviderId(5, providerIds), -1);
    QCOMPARE(QGeoMapIdQGC::fromProviderId(8, providerIds), -1);
}

UT_REGISTER_TEST(QGeoTiledMappingManagerEngineQGCTest, TestLabel::Unit)
