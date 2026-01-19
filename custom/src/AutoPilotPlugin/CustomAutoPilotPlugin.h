/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom Autopilot Plugin
 *   @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include "APMAutoPilotPlugin.h"

class Vehicle;

class CustomAutoPilotPlugin : public APMAutoPilotPlugin
{
    Q_OBJECT
public:
    CustomAutoPilotPlugin(Vehicle* vehicle, QObject* parent);


private slots:
    void         _advancedChanged        (bool advanced);

private:
    QVariantList _components;
};
