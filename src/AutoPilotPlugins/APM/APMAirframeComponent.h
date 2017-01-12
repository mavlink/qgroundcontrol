/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMAirframeComponent_H
#define APMAirframeComponent_H

#include "VehicleComponent.h"

class APMAirframeComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    APMAirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
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
    QString prerequisiteSetup(void) const final;

private:
    bool            _requiresFrameSetup; ///< true: FRAME parameter must be set
    const QString   _name;
    Fact*           _frameParamFact;
    bool            _useNewFrameParam;

    static const char* _oldFrameParam;
    static const char* _newFrameParam;
};

#endif
