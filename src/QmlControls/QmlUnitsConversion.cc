#include "QmlUnitsConversion.h"

#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"

QGC_LOGGING_CATEGORY(QmlUnitsConversionLog, "QMLControls.QmlUnitsConversion")

QmlUnitsConversion::QmlUnitsConversion(QObject* parent)
    : QObject(parent)
{
    UnitsSettings* const unitsSettings = SettingsManager::instance()->unitsSettings();
    const QList<Fact*> unitsFacts = {
        unitsSettings->horizontalDistanceUnits(),
        unitsSettings->verticalDistanceUnits(),
        unitsSettings->areaUnits(),
        unitsSettings->weightUnits(),
        unitsSettings->speedUnits(),
    };

    for (Fact* const fact : unitsFacts) {
        (void) connect(fact, &Fact::rawValueChanged, this, [this, fact]() {
            qCDebug(QmlUnitsConversionLog) << "units setting changed:" << fact->name();
            emit unitsChanged();
        });
    }
}
