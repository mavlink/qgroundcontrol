/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef SettingsGroup_H
#define SettingsGroup_H

#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "MultiVehicleManager.h"
#include "QGCToolbox.h"

#include <QVariantList>

#define DEFINE_SETTING_NAME_GROUP() \
    static const char* name; \
    static const char* settingsGroup;

#define DECLARE_SETTINGGROUP(NAME, GROUP) \
    const char* NAME ## Settings::name = #NAME; \
    const char* NAME ## Settings::settingsGroup = GROUP; \
    NAME ## Settings::NAME ## Settings(QObject* parent) \
        : SettingsGroup(name, settingsGroup, parent)

#define DECLARE_SETTINGSFACT(CLASS, NAME) \
    const char* CLASS::NAME ## Name = #NAME; \
    Fact* CLASS::NAME() \
    { \
        if (!_ ## NAME ## Fact) { \
            _ ## NAME ## Fact = _createSettingsFact(NAME ## Name); \
        } \
        return _ ## NAME ## Fact; \
    }

#define DECLARE_SETTINGSFACT_NO_FUNC(CLASS, NAME) \
    const char* CLASS::NAME ## Name = #NAME; \
    Fact* CLASS::NAME()

#define DEFINE_SETTINGFACT(NAME) \
    private: \
    SettingsFact* _ ## NAME ## Fact = nullptr; \
    public: \
    Q_PROPERTY(Fact* NAME READ NAME CONSTANT) \
    Fact* NAME(); \
    static const char* NAME ## Name;

/// Provides access to group of settings. The group is named and has a visible property associated with which can control whether the group
/// is shows in the ui.
class SettingsGroup : public QObject
{
    Q_OBJECT

public:
    /// @param name Name for this Settings group
    /// @param settingsGroup Group to place settings in for QSettings::setGroup
    SettingsGroup(const QString &name, const QString &settingsGroup, QObject* parent = nullptr);

    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

    virtual bool    visible             () { return _visible; }
    virtual void    setVisible          (bool vis) { _visible = vis; emit visibleChanged(); }

signals:
    void            visibleChanged      ();

protected:
    SettingsFact*   _createSettingsFact(const QString& factName);
    bool            _visible;
    QString         _name;
    QString         _settingsGroup;

    QMap<QString, FactMetaData*> _nameToMetaDataMap;
};

#endif
