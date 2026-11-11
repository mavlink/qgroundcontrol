#include "QGeoTiledMappingManagerEngineQGCTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "QGCMapUrlEngine.h"
#include "QGeoMapIdQGC.h"
#include "QGeoServiceProviderPluginQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "SettingsManager.h"

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

void QGeoTiledMappingManagerEngineQGCTest::_testSourceChangeGivesTilesNewVersion()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    const QVariant savedUrl = customUrl->rawValue();
    const auto restoreUrl = qScopeGuard([customUrl, savedUrl] { customUrl->setRawValue(savedUrl); });
    customUrl->setRawValue(QStringLiteral("https://a.example/{z}/{x}/{y}.png"));

    QNetworkAccessManager networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGeoTiledMappingManagerEngineQGC engine({}, &error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    const int tileVersion = engine.tileVersion();
    QSignalSpy versionSpy(&engine, &QGeoTiledMappingManagerEngine::tileVersionChanged);

    // The maps and QtLocation's tile cache hold tiles of the source that was configured. On a new tile version
    // QtLocation's maps request their tiles again, and drop those of the old version.
    customUrl->setRawValue(QStringLiteral("https://b.example/{z}/{x}/{y}.png"));

    QCOMPARE(versionSpy.count(), 1);
    QCOMPARE_NE(engine.tileVersion(), tileVersion);
}

void QGeoTiledMappingManagerEngineQGCTest::_testFactoryNeedsQmlEngine()
{
    // QtLocation gives the factory the QML engine before it asks for a mapping engine, which fetches its tiles with
    // the network access manager of that engine
    const QGeoServiceProviderFactoryQGC factory;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NoError;
    QString errorString;

    expectLogMessage("QtLocationPlugin.QGeoServiceProviderFactoryQGC", QtWarningMsg,
                     QRegularExpression(QStringLiteral("QML engine")));
    QCOMPARE(factory.createMappingManagerEngine({}, &error, &errorString), nullptr);
    verifyExpectedLogMessage();
    QCOMPARE_NE(error, QGeoServiceProvider::NoError);
    QVERIFY(!errorString.isEmpty());
}

UT_REGISTER_TEST(QGeoTiledMappingManagerEngineQGCTest, TestLabel::Unit)
