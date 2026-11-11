#include "QGeoTiledMappingManagerEngineQGCTest.h"

#include <QtLocation/private/qgeomaptype_p.h>
#include <QtNetwork/QNetworkAccessManager>

#include "QGCMapUrlEngine.h"
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

UT_REGISTER_TEST(QGeoTiledMappingManagerEngineQGCTest, TestLabel::Unit)
