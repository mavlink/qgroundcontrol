/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once
#include "SafetyComponent.h"

class YuneecSafetyComponent : public SafetyComponent
{
    Q_OBJECT
public:
    YuneecSafetyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    QString     description         (void) const override;
    QUrl        setupSource         (void) const override;
    QUrl        summaryQmlSource    (void) const override;
};
