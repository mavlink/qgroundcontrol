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

    Q_PROPERTY(QList<Joystick*> activePeripherals READ activePeripherals WRITE setActivePeripherals NOTIFY activePeripheralsNamesChanged)
    Q_PROPERTY(QString activePeripheralName READ activePeripheralName WRITE setActivePeripheralName NOTIFY activePeripheralsNamesChanged)
    /// List of available joysticks
    QVariantList joysticks();
    /// List of available joystick names
    QStringList joystickNames(void);

    /// Get active joystick
    Joystick* activeJoystick(void);
    QList<Joystick*> activePeripherals(void);

    /// Get list of actives joystick
    QList<Joystick*> getActivesJoysticks(QList<Joystick*>);
    /// Set active joystick
    void setActiveJoystick(Joystick* joystick);
    void setActivePeripherals(Joystick* joystick);
    void setActivePeripherals(QList<Joystick*> peripherals);


    QString activeJoystickName(void);
    QString activePeripheralName(void);

    bool setActiveJoystickName(const QString& name);
    bool setActivePeripheralName(const QString& name);

    QList<QString> activeJoysticksNames(void);

    void restartJoystickCheckTimer(void);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

public slots:
    void init();

signals:
    void activeJoystickChanged(Joystick* joystick);
    void activePeripheralsChanged(QList<Joystick*> joysticksList);
    void activePeripheralsNamesChanged( QList<QString> peripheralNamesList);
    void activeJoystickNameChanged(const QString& name);
    void activeJoysticksNamesChanged(const QList<QString>& name);
    void availableJoysticksChanged(void);
    void updateAvailableJoysticksSignal();

private slots:
    void _updateAvailableJoysticks(void);

private:
    void _setActiveJoystickFromSettings(void);
    void _setActiveJoysticksFromSettings(void);

private:
    Joystick*                   _activeJoystick;
    QList<Joystick*>            _activeJoysticksList;
    QMap<QString, Joystick*>    _name2JoystickMap;
    MultiVehicleManager*        _multiVehicleManager;

    static const char * _settingsGroup;
    static const char * _settingsKeyActiveJoystick;

    int _joystickCheckTimerCounter;
    QTimer _joystickCheckTimer;
};
