#include "TelemetryBarValues.h"

#include <QtCore/QMap>

#include "Fact.h"
#include "FactValueGrid.h"
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

void TelemetryBarValues::readLayout()
{
    _fields.clear();
    for (const FactValueGrid::LayoutFact& fact :
         FactValueGrid::layoutFacts(HorizontalFactValueGrid::telemetryBarSettingsGroup)) {
        _fields.append({fact.factGroupName, fact.factName});
    }
}

std::optional<QList<TelemetryBarValues::Value>> TelemetryBarValues::read() const
{
    // Resolved on every read: the active vehicle, and the Facts it owns, can change at any time.
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (!vehicle) {
        return std::nullopt;
    }

    QList<Value> values;
    for (const Field& field : _fields) {
        FactGroup* factGroup = nullptr;
        if (field.factGroupName == QLatin1StringView(InstrumentValueData::vehicleFactGroupName)) {
            factGroup = vehicle;
        } else {
            // Like getFactGroup(), which camel-cases the name, but without its warning for a group this vehicle lacks.
            const QMap<QString, FactGroup*>& factGroups = vehicle->factGroups();
            for (auto it = factGroups.cbegin(); it != factGroups.cend(); ++it) {
                if (it.key().compare(field.factGroupName, Qt::CaseInsensitive) == 0) {
                    factGroup = it.value();
                    break;
                }
            }
        }
        if (!factGroup || !factGroup->factExists(field.factName)) {
            continue;
        }
        const Fact* const fact = factGroup->getFact(field.factName);
        values.append(
            {fact->shortDescription(), QStringLiteral("%1 %2").arg(fact->cookedValueString(), fact->cookedUnits())});
    }
    return values;
}
