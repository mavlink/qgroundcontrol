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
bool CustomQuickInterface::_showVirtualKeyboard = true;
bool CustomQuickInterface::_enableNewGimbalControls = true;
bool CustomQuickInterface::_gimbalPitchInverted = false;
bool CustomQuickInterface::_gimbalYawInverted = false;
bool CustomQuickInterface::_gimbalPitchPidEnabled = false;
bool CustomQuickInterface::_gimbalYawPidEnabled = false;

static const char* kGroupName       = "CustomSettings";
static const char* kShowGimbalCtl   = "ShowGimbalCtl";
static const char* kUseEmbeddedGimbal  = "UseEmbeddedGimbal";
static const char* kShowAttitudeWidget = "ShowAttitudeWidget";
static const char* kVirtualKeyboard = "ShowVirtualKeyboard";
static const char* kEnableNewGimbalControls = "EnableNewGimbalControls";
static const char* kGimbalPitchInverted = "GimbalPitchInverted";
static const char* kGimbalYawInverted = "GimbalYawInverted";
static const char* kGimbalPitchPidEnabled = "GimbalPitchPidEnabled";
static const char* kGimbalYawPidEnabled = "GimbalYawPidEnabled";

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
    _settings.reset(new QSettings(QSettings::IniFormat, QSettings::UserScope, QString(QGC_ORG_NAME), QString(QGC_APPLICATION_NAME)));
    _settings->beginGroup(kGroupName);
    _showGimbalControl = _settings->value(kShowGimbalCtl, false).toBool();
    _useEmbeddedGimbal = _settings->value(kUseEmbeddedGimbal, true).toBool();
    _showAttitudeWidget = _settings->value(kShowAttitudeWidget, false).toBool();
    _showVirtualKeyboard = _settings->value(kVirtualKeyboard, true).toBool();
    _enableNewGimbalControls = _settings->value(kEnableNewGimbalControls, true).toBool();
    _gimbalPitchInverted = _settings->value(kGimbalPitchInverted, false).toBool();
    _gimbalYawInverted = _settings->value(kGimbalYawInverted, false).toBool();
    _gimbalPitchPidEnabled = _settings->value(kGimbalPitchPidEnabled, false).toBool();
    _gimbalYawPidEnabled = _settings->value(kGimbalYawPidEnabled, false).toBool();
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

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setEnableNewGimbalControls(bool set)
{
    if(_enableNewGimbalControls != set && _settings) {
        _enableNewGimbalControls = set;
        _settings->setValue(kEnableNewGimbalControls,set);
        emit enableNewGimbalControlsChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setGimbalPitchInverted(bool set)
{
    if(_gimbalPitchInverted != set && _settings) {
        _gimbalPitchInverted = set;
        _settings->setValue(kGimbalPitchInverted,set);
        emit gimbalPitchInvertedChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setGimbalYawInverted(bool set)
{
    if(_gimbalYawInverted != set && _settings) {
        _gimbalYawInverted = set;
        _settings->setValue(kGimbalYawInverted,set);
        emit gimbalYawInvertedChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setGimbalPitchPidEnabled(bool set)
{
    if(_gimbalPitchPidEnabled != set && _settings) {
        _gimbalPitchPidEnabled = set;
        _settings->setValue(kGimbalPitchPidEnabled,set);
        emit gimbalPitchPidEnabledChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setGimbalYawPidEnabled(bool set)
{
    if(_gimbalYawPidEnabled != set && _settings) {
        _gimbalYawPidEnabled = set;
        _settings->setValue(kGimbalYawPidEnabled,set);
        emit gimbalYawPidEnabledChanged();
    }
}