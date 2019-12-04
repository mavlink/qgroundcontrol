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
#include <QPluginLoader>

std::unique_ptr<QSettings> CustomQuickInterface::_settings;

bool CustomQuickInterface::_showGimbalControl = false;
bool CustomQuickInterface::_useEmbeddedGimbal = true;
bool CustomQuickInterface::_showAttitudeWidget = false;
bool CustomQuickInterface::_showVirtualKeyboard = false;

static const char* kGroupName       = "CustomSettings";
static const char* kShowGimbalCtl   = "ShowGimbalCtl";
static const char* kUseEmbeddedGimbal  = "UseEmbeddedGimbal";
static const char* kShowAttitudeWidget = "ShowAttitudeWidget";
static const char* kVirtualKeyboard = "ShowVirtualKeyboard";

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
CustomQuickInterface::initSettings() {
    QSettings::setDefaultFormat(QSettings::IniFormat);
    _settings.reset(new QSettings(QString(QGC_ORG_NAME), QString(QGC_APPLICATION_NAME)));
    _settings->beginGroup(kGroupName);
    _showGimbalControl = _settings->value(kShowGimbalCtl, false).toBool();
    _useEmbeddedGimbal = _settings->value(kUseEmbeddedGimbal, false).toBool();
    _showAttitudeWidget = _settings->value(kShowAttitudeWidget, false).toBool();
    _showVirtualKeyboard = _settings->value(kVirtualKeyboard, false).toBool();
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::init()
{
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setShowGimbalControl(bool set)
{
    if(_showGimbalControl != set && _settings) {
        _showGimbalControl = set;
        _settings->setValue(kShowGimbalCtl,set);
        emit showGimbalControlChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setUseEmbeddedGimbal(bool set)
{
    if(_useEmbeddedGimbal != set && _settings) {
        _useEmbeddedGimbal = set;
        _settings->setValue(kUseEmbeddedGimbal, set);
        emit useEmbeddedGimbalChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setShowAttitudeWidget(bool set)
{
    if(_showAttitudeWidget != set && _settings) {
        _showAttitudeWidget = set;
        _settings->setValue(kShowAttitudeWidget,set);
        emit showAttitudeWidgetChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setShowVirtualKeyboard(bool set)
{
    if(_showVirtualKeyboard != set && _settings) {
        _showVirtualKeyboard = set;
        _settings->setValue(kVirtualKeyboard,set);
        emit showVirtualKeyboardChanged();
    }
}
