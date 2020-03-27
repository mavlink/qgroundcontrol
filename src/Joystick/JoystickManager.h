/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief  Joystick Manager

#pragma once

#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "MultiVehicleManager.h"
#include "QGCToolbox.h"

#include <QVariantList>

Q_DECLARE_LOGGING_CATEGORY(JoystickManagerLog)

/// Joystick Manager
class JoystickManager : public QGCTool
{
    Q_OBJECT

public:
    JoystickManager(QGCApplication* app, QGCToolbox* toolbox);
    ~JoystickManager();

    Q_PROPERTY(QVariantList joysticks READ joysticks NOTIFY availableJoysticksChanged)
    Q_PROPERTY(QStringList  joystickNames READ joystickNames NOTIFY availableJoysticksChanged)

    Q_PROPERTY(Joystick* activeJoystick READ activeJoystick WRITE setActiveJoystick NOTIFY activeJoystickChanged)
    Q_PROPERTY(QString activeJoystickName READ activeJoystickName WRITE setActiveJoystickName NOTIFY activeJoystickNameChanged)

    /// List of available joysticks
    QVariantList joysticks();
    /// List of available joystick names
    QStringList joystickNames(void);

    /// Get active joystick
    Joystick* activeJoystick(void);
    /// Set active joystick
    void setActiveJoystick(Joystick* joystick);

    QString activeJoystickName(void);
    void setActiveJoystickName(const QString& name);

    void restartJoystickCheckTimer(void);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

public slots:
    void init();

signals:
    void activeJoystickChanged(Joystick* joystick);
    void activeJoystickNameChanged(const QString& name);
    void availableJoysticksChanged(void);
    void updateAvailableJoysticksSignal();

private slots:
    void _updateAvailableJoysticks(void);

private:
    void _setActiveJoystickFromSettings(void);

private:
    Joystick*                   _activeJoystick;
    QMap<QString, Joystick*>    _name2JoystickMap;
    MultiVehicleManager*        _multiVehicleManager;

    static const char * _settingsGroup;
    static const char * _settingsKeyActiveJoystick;

    int _joystickCheckTimerCounter;
    QTimer _joystickCheckTimer;
};
