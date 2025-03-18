/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(JoystickManagerLog)

class Joystick;
class QTimer;

class JoystickManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Joystick.h")
    Q_PROPERTY(QVariantList joysticks READ joysticks NOTIFY availableJoysticksChanged)
    Q_PROPERTY(QStringList joystickNames READ joystickNames NOTIFY availableJoysticksChanged)
    Q_PROPERTY(Joystick *activeJoystick READ activeJoystick WRITE setActiveJoystick NOTIFY activeJoystickChanged)
    Q_PROPERTY(QString activeJoystickName READ activeJoystickName WRITE setActiveJoystickName NOTIFY activeJoystickNameChanged)

public:
    explicit JoystickManager(QObject *parent = nullptr);
    ~JoystickManager();

    static JoystickManager *instance();
    static void registerQmlTypes();

    QVariantList joysticks();
    QStringList joystickNames() const { return _name2JoystickMap.keys(); }

    Joystick *activeJoystick();
    void setActiveJoystick(Joystick *joystick);

    QString activeJoystickName() const;
    bool setActiveJoystickName(const QString &name);

signals:
    void activeJoystickChanged(Joystick *joystick);
    void activeJoystickNameChanged(const QString &name);
    void availableJoysticksChanged();
    void updateAvailableJoysticksSignal();

public slots:
    void init();

private slots:
    // TODO: move this to the right place: JoystickSDL.cc and JoystickAndroid.cc respectively and call through Joystick.cc
    void _updateAvailableJoysticks();

private:
    void _setActiveJoystickFromSettings();

    Joystick *_activeJoystick = nullptr;
    QMap<QString, Joystick*> _name2JoystickMap;

    int _joystickCheckTimerCounter = 0;;
    QTimer *_joystickCheckTimer = nullptr;

    static constexpr int kTimerInterval = 1000;
    static constexpr int kTimeout = 1000;
    static constexpr const char *_settingsGroup = "JoystickManager";
    static constexpr const char *_settingsKeyActiveJoystick = "ActiveJoystick";
};
