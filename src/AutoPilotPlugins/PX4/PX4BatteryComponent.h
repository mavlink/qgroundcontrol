#pragma once

#include "VehicleComponent.h"

class PX4BatteryComponent : public VehicleComponent
{
    Q_OBJECT

public:
    PX4BatteryComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const final;

    QString name() const final { return _name; }

    QString description() const final;
    QString iconResource() const final;
    QStringList searchKeywords() const final;

    bool requiresSetup() const final { return true; }

    int setupOrder() const final { return 3; }

    bool setupComplete() const final;

    QUrl setupSource() const final;

    QUrl summaryQmlSource() const final { return QUrl(); }

    QList<VehicleSummaryItem> summaryItems() const final;

    QStringList summaryChangedTriggerList() const final;

    void setupTriggerSignals() final;

    bool allowSetupWhileArmed() const final { return true; }

private:
    static QString _batteryParam(int batteryIndex, const char* suffix);

    /// DroneCAN battery data runs through the battery estimate, even for a battery whose source is disabled
    bool _canFiltered() const;

    const QString _name;
};
