/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef JoystickManager_H
#define JoystickManager_H

#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "MultiVehicleManager.h"
#include "QGCToolbox.h"
#include "JoystickMessageSender.h"
#include "KeyConfiguration.h"

#include <QVariantList>
#include <QQmlListProperty>

Q_DECLARE_LOGGING_CATEGORY(JoystickManagerLog)

class JoystickManager : public QGCTool
{
    Q_OBJECT
    
public:
    JoystickManager(QGCApplication* app, QGCToolbox* toolbox);
    ~JoystickManager();

    /// List of available joysticks
    Q_PROPERTY(QVariantList joysticks READ joysticks NOTIFY availableJoysticksChanged)
    Q_PROPERTY(QStringList  joystickNames READ joystickNames NOTIFY availableJoysticksChanged)
    
    /// Active joystick
    Q_PROPERTY(Joystick* activeJoystick READ activeJoystick WRITE setActiveJoystick NOTIFY activeJoystickChanged)
    Q_PROPERTY(QString activeJoystickName READ activeJoystickName WRITE setActiveJoystickName NOTIFY activeJoystickNameChanged)

    //default joystick config
    Q_PROPERTY(int                  joystickMode            READ joystickMode           WRITE setJoystickMode           NOTIFY joystickModeChanged)
    Q_PROPERTY(QStringList          joystickModes           READ joystickModes                                          CONSTANT)
    Q_PROPERTY(QStringList          joystickAction          READ joystickAction         WRITE setJoystickAction)
    Q_PROPERTY(bool                 joystickEnabled         READ joystickEnabled        WRITE setJoystickEnabled        NOTIFY joystickEnabledChanged)
    Q_PROPERTY(bool                 supportsThrottleModeCenterZero   READ supportsThrottleModeCenterZero     WRITE setSupportsThrottleModeCenterZero)
    Q_PROPERTY(bool                 supportsNegativeThrust  READ supportsNegativeThrust WRITE setSupportsNegativeThrust)
    Q_PROPERTY(bool                 supportsJSButton        READ supportsJSButton       WRITE setSupportsJSButton)
    Q_PROPERTY(int manualControlReservedButtonCount READ manualControlReservedButtonCount   WRITE setManualControlReservedButtonCount)

    Q_PROPERTY(QQmlListProperty<KeyConfiguration> keyConfigurationList READ keyConfigurationList CONSTANT)
    Q_PROPERTY(JoystickMessageSender* joystickMessageSender READ joystickMessageSender CONSTANT)

    bool supportsThrottleModeCenterZero(void);
    void setSupportsThrottleModeCenterZero(bool enabled);
    bool supportsNegativeThrust(void);
    void setSupportsNegativeThrust(bool enabled);
    bool supportsJSButton(void);
    void setSupportsJSButton(bool enabled);
    int  manualControlReservedButtonCount(void);
    void setManualControlReservedButtonCount(int count);
    
    QVariantList joysticks();
    QStringList joystickNames(void);
    
    Joystick* activeJoystick(void);
    void setActiveJoystick(Joystick* joystick);
    
    QString activeJoystickName(void);
    void setActiveJoystickName(const QString& name);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    int joystickMode(void);
    void setJoystickMode(int mode);

    QStringList joystickAction(void);
    void setJoystickAction(QStringList list);

    // List of joystick mode names
    QStringList joystickModes(void);

    bool joystickEnabled(void);
    void setJoystickEnabled(bool enabled);

    QQmlListProperty<KeyConfiguration> keyConfigurationList();
    KeyConfiguration* getKeyConfiguration(int index);
    JoystickMessageSender* joystickMessageSender(void) { return _joystickMessageSender; }

public slots:
    void init();

signals:
    void activeJoystickChanged(Joystick* joystick);
    void activeJoystickNameChanged(const QString& name);
    void availableJoysticksChanged(void);
    void joystickModeChanged(int mode);
    void joystickEnabledChanged(bool enabled);

private slots:
    void _updateAvailableJoysticks(void);
    
private:
    void _setActiveJoystickFromSettings(void);
    
private:
    void _loadSettings(void);
    void _saveSettings(void);
    void _startJoystick(bool start);

    Joystick*                   _activeJoystick;
    QMap<QString, Joystick*>    _name2JoystickMap;
    MultiVehicleManager*        _multiVehicleManager;
    JoystickMessageSender*      _joystickMessageSender;
    QList<KeyConfiguration *> _keyConfigurationList;
    
    static const char * _settingsGroup;
    static const char * _settingsKeyActiveJoystick;
    static const char * _joystickModeSettingsKey;
    static const char * _joystickEnabledSettingsKey;

    Vehicle::JoystickMode_t  _joystickMode;
    bool                     _joystickEnabled;
    bool                    _supportsThrottleModeCenterZero;
    bool                    _supportsNegativeThrust;
    bool                    _supportsJSButton;
    int                     _manualControlReservedButtonCount;
    QStringList             _joystickAction;

    QTimer _joystickCheckTimer;
};

#endif
