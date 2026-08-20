#include "GeoViewSettingsTest.h"

#include <QtCore/QScopeGuard>

#include "GeoViewSettings.h"
#include "SettingsManager.h"

void GeoViewSettingsTest::_defaults()
{
    GeoViewSettings* const settings = SettingsManager::instance()->geoViewSettings();
    QVERIFY(settings);

    // Preview feature must be off by default
    Fact* const enabled = settings->enabled();
    QVERIFY(enabled);
    QCOMPARE(enabled->rawValue().toBool(), false);
    QVERIFY(settings->userVisible());
}

void GeoViewSettingsTest::_enabledToggle()
{
    GeoViewSettings* const settings = SettingsManager::instance()->geoViewSettings();
    QVERIFY(settings);

    Fact* const enabled = settings->enabled();
    const QVariant saved = enabled->rawValue();
    const auto guard = qScopeGuard([enabled, saved] { enabled->setRawValue(saved); });

    enabled->setRawValue(true);
    QCOMPARE(enabled->rawValue().toBool(), true);
    enabled->setRawValue(false);
    QCOMPARE(enabled->rawValue().toBool(), false);
}

UT_REGISTER_TEST(GeoViewSettingsTest, TestLabel::Unit)
