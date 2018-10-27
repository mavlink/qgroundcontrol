/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QObject>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "Vehicle.h"

class ViewWidgetController : public QObject
{
	Q_OBJECT
	
public:
	ViewWidgetController(void);
	
	Q_INVOKABLE void checkForVehicle(void);
	
signals:
	void pluginConnected(QVariant autopilot);
	void pluginDisconnected(void);
	
private slots:
    void _vehicleAvailable(bool available);

private:
	AutoPilotPlugin*        _autopilot;
	UASInterface*           _uas;
};

