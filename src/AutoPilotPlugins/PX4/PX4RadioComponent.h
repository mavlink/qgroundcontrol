#pragma once

#include "VehicleComponent.h"

class PX4RadioComponent : public VehicleComponent
{
    Q_OBJECT

public:
    PX4RadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // Virtuals from VehicleComponent
    virtual QStringList setupCompleteChangedTriggerList(void) const;

    // Virtuals from VehicleComponent
    virtual QString name(void) const;
    virtual QString description(void) const;
    virtual QString iconResource(void) const;
    virtual bool requiresSetup(void) const;

    virtual int setupOrder() const { return 2; }
    virtual bool setupComplete(void) const;
    virtual QUrl setupSource(void) const;

    virtual QUrl summaryQmlSource(void) const { return QUrl(); }

    virtual QList<VehicleSummaryItem> summaryItems() const;
    virtual QStringList summaryChangedTriggerList() const;

    virtual void setupTriggerSignals();

private:
    const QString _name;
};
