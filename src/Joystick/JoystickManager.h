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

#include <QVariantList>

Q_DECLARE_LOGGING_CATEGORY(JoystickManagerLog)

class JoystickManager : public QGCTool
{
    Q_OBJECT
    
public:
    JoystickManager(QGCApplication* app);
    ~JoystickManager();

    /// List of available joysticks
    Q_PROPERTY(QVariantList joysticks READ joysticks CONSTANT)
    Q_PROPERTY(QStringList  joystickNames READ joystickNames CONSTANT)
    
    /// Active joystick
    Q_PROPERTY(Joystick* activeJoystick READ activeJoystick WRITE setActiveJoystick NOTIFY activeJoystickChanged)
    Q_PROPERTY(QString activeJoystickName READ activeJoystickName WRITE setActiveJoystickName NOTIFY activeJoystickNameChanged)
    
    QVariantList joysticks();
    QStringList joystickNames(void);
    
    Joystick* activeJoystick(void);
    void setActiveJoystick(Joystick* joystick);
    
    QString activeJoystickName(void);
    void setActiveJoystickName(const QString& name);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

signals:
    void activeJoystickChanged(Joystick* joystick);
    void activeJoystickNameChanged(const QString& name);

private slots:
    
private:
    void _setActiveJoystickFromSettings(void);
    
private:
    Joystick*                   _activeJoystick;
    QMap<QString, Joystick*>    _name2JoystickMap;
    MultiVehicleManager*        _multiVehicleManager;
    
    static const char * _settingsGroup;
    static const char * _settingsKeyActiveJoystick;
};

#endif
