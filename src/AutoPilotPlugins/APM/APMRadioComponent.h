/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMRadioComponent_H
#define APMRadioComponent_H

#include "VehicleComponent.h"
#include "Fact.h"

class APMRadioComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    APMRadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const final;
    
    // Virtuals from VehicleComponent
    QString name(void) const final;
    QString description(void) const final;
    QString iconResource(void) const final;
    bool requiresSetup(void) const final;
    bool setupComplete(void) const final;
    QUrl setupSource(void) const final;
    QUrl summaryQmlSource(void) const final;

private slots:
    void _triggerChanged(void);
    
private:
    void _connectSetupTriggers(void);

    const QString   _name;
    QStringList     _mapParams;
    QList<Fact*>    _triggerFacts;
};

#endif
