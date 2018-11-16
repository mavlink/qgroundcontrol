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

#include "AutoPilotPlugin.h"
#include "FactPanelController.h"

class CustomCommandWidgetController : public FactPanelController
{
	Q_OBJECT
	
public:
	CustomCommandWidgetController(void);
    
    Q_PROPERTY(QString customQmlFile MEMBER _customQmlFile NOTIFY customQmlFileChanged)
	
    Q_INVOKABLE void sendCommand    (int commandId, QVariant componentId, QVariant confirm, QVariant param1, QVariant param2, QVariant param3, QVariant param4, QVariant param5, QVariant param6, QVariant param7);
    Q_INVOKABLE void selectQmlFile  (void);
    Q_INVOKABLE void clearQmlFile   (void);
    
signals:
    void customQmlFileChanged   (const QString& customQmlFile);

private:
    Vehicle*            _vehicle;
    QString             _customQmlFile;
    static const char*  _settingsKey;
};

