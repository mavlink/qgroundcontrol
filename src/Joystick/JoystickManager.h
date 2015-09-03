/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef JoystickManager_H
#define JoystickManager_H

#include "QGCSingleton.h"
#include "QGCLoggingCategory.h"
#include "Joystick.h"

#include <QVariantList>

Q_DECLARE_LOGGING_CATEGORY(JoystickManagerLog)

class JoystickManager : public QGCSingleton
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(JoystickManager, JoystickManager)

public:
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

signals:
    void activeJoystickChanged(Joystick* joystick);
    void activeJoystickNameChanged(const QString& name);

private slots:
    
private:
    /// All access to singleton is through JoystickManager::instance
    JoystickManager(QObject* parent = NULL);
    ~JoystickManager();
    
    void _setActiveJoystickFromSettings(void);
    
private:
    Joystick*   _activeJoystick;
    
    QMap<QString, Joystick*>    _name2JoystickMap;
    
    static const char * _settingsGroup;
    static const char * _settingsKeyActiveJoystick;
};

#endif
