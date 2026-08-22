#include "FlyViewSettingsTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "FlyViewSettings.h"
#include "SettingsManager.h"

void FlyViewSettingsTest::_defaults()
{
    FlyViewSettings* const settings = SettingsManager::instance()->flyViewSettings();
    QVERIFY(settings);

    // Preview feature and its debug chrome must be off by default
    Fact* const useGeoMapEngine = settings->useGeoMapEngine();
    QVERIFY(useGeoMapEngine);
    QCOMPARE(useGeoMapEngine->rawValue().toBool(), false);
    Fact* const geoMapDebugUI = settings->geoMapDebugUI();
    QVERIFY(geoMapDebugUI);
    QCOMPARE(geoMapDebugUI->rawValue().toBool(), false);
    QVERIFY(settings->userVisible());
}

void FlyViewSettingsTest::_useGeoMapEngineToggle()
{
    FlyViewSettings* const settings = SettingsManager::instance()->flyViewSettings();
    QVERIFY(settings);

    Fact* const useGeoMapEngine = settings->useGeoMapEngine();
    QVERIFY(useGeoMapEngine);
    const QVariant saved = useGeoMapEngine->rawValue();
    const auto guard = qScopeGuard([useGeoMapEngine, saved] { useGeoMapEngine->setRawValue(saved); });

    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    useGeoMapEngine->setRawValue(true);
    QCOMPARE(useGeoMapEngine->rawValue().toBool(), true);
    useGeoMapEngine->setRawValue(false);
    QCOMPARE(useGeoMapEngine->rawValue().toBool(), false);
}

UT_REGISTER_TEST(FlyViewSettingsTest, TestLabel::Unit)
