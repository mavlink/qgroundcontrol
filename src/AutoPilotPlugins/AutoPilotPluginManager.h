/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef AUTOPILOTPLUGINMANAGER_H
#define AUTOPILOTPLUGINMANAGER_H

#include <QObject>
#include <QList>
#include <QString>

#include "AutoPilotPlugin.h"
#include "Vehicle.h"
#include "QGCToolbox.h"

class QGCApplication;

class AutoPilotPluginManager : public QGCTool
{
    Q_OBJECT

public:
    AutoPilotPluginManager(QGCApplication* app) : QGCTool(app) { }

    AutoPilotPlugin* newAutopilotPluginForVehicle(Vehicle* vehicle);
};

#endif
