/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
#pragma once

#include "PX4AutoPilotPlugin.h"

class Vehicle;

class CustomAutoPilotPlugin : public PX4AutoPilotPlugin
{
    Q_OBJECT

public:
    explicit CustomAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);

    /// This allows us to hide most Vehicle Setup pages unless we are in Advanced Mmode
    const QVariantList &vehicleComponents() final;

private slots:
    /// This signals that when Advanced Mode changes the list of Vehicle Settings page also changed
    void _advancedChanged(bool advanced);

private:
    QVariantList _components;
};
