/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom QtQuick Interface
 *   @author Gus Grubba <gus@auterion.com>
 */

#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MAVLinkLogManager.h"
#include "QGCMapEngine.h"
#include "QGCApplication.h"
#include "PositionManager.h"

#include "CustomPlugin.h"
#include "CustomQuickInterface.h"

#include <QSettings>

static const char* kGroupName       = "CustomSettings";
static const char* kShowGimbalCtl   = "ShowGimbalCtl";
static const char* kShowAttitudeWidget = "ShowAttitudeWidget";

//-----------------------------------------------------------------------------
CustomQuickInterface::CustomQuickInterface(QObject* parent)
    : QObject(parent)
{
    qCDebug(CustomLog) << "CustomQuickInterface Created";
}

//-----------------------------------------------------------------------------
CustomQuickInterface::~CustomQuickInterface()
{
    qCDebug(CustomLog) << "CustomQuickInterface Destroyed";
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::init()
{
    QSettings settings;
    settings.beginGroup(kGroupName);
    _showGimbalControl = settings.value(kShowGimbalCtl, false).toBool();
    _showAttitudeWidget = settings.value(kShowAttitudeWidget, false).toBool();
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setShowGimbalControl(bool set)
{
    if(_showGimbalControl != set) {
        _showGimbalControl = set;
        QSettings settings;
        settings.beginGroup(kGroupName);
        settings.setValue(kShowGimbalCtl,set);
        emit showGimbalControlChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setShowAttitudeWidget(bool set)
{
    if(_showAttitudeWidget != set) {
        _showAttitudeWidget = set;
        QSettings settings;
        settings.beginGroup(kGroupName);
        settings.setValue(kShowAttitudeWidget,set);
        emit showAttitudeWidgetChanged();
    }
}
