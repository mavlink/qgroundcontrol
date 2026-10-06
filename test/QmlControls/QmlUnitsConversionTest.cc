#include "QmlUnitsConversionTest.h"

#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "FactMetaData.h"
#include "QmlUnitsConversion.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"

void QmlUnitsConversionTest::_unitsChangedSignal_test()
{
    Fact* const horizontalUnitsFact = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    const QVariant savedUnits = horizontalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([horizontalUnitsFact, savedUnits] { horizontalUnitsFact->setRawValue(savedUnits); });
    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsMeters);

    QmlUnitsConversion unitsConversion;
    QCOMPARE(unitsConversion.appSettingsHorizontalDistanceUnitsString(), QStringLiteral("m"));

    QSignalSpy unitsChangedSpy(&unitsConversion, &QmlUnitsConversion::unitsChanged);
    QVERIFY(unitsChangedSpy.isValid());

    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);

    QCOMPARE(unitsChangedSpy.count(), 1);
    QCOMPARE(unitsConversion.appSettingsHorizontalDistanceUnitsString(), QStringLiteral("ft"));
}

UT_REGISTER_TEST(QmlUnitsConversionTest, TestLabel::Unit)
