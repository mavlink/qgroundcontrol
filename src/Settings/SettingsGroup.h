/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsFact.h"

// The best way to understand these macros is to look at their use in SettingsGroup subclasses

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

#define DECLARE_SETTINGS_GROUP_ARRAY(ROOT_SETTINGS_GROUP, SETTINGS_GROUP_CLASS, SETTINGS_GROUP_ARRAY_CLASS, SIZE) \
    SETTINGS_GROUP_ARRAY_CLASS *SETTINGS_GROUP_CLASS::get ## SETTINGS_GROUP_ARRAY_CLASS ## ByIndex(int index) \
    { \
        if (index < 0 || index >= SIZE) { \
            qWarning() << "Invalid index requested for array" << #SETTINGS_GROUP_ARRAY_CLASS << ":" << index; \
            return nullptr; \
        } \
        if (!_map ## SETTINGS_GROUP_ARRAY_CLASS.contains(index)) { \
            _map ## SETTINGS_GROUP_ARRAY_CLASS[index] = new SETTINGS_GROUP_ARRAY_CLASS(#SETTINGS_GROUP_ARRAY_CLASS, QString("%1/#%2%3").arg(_settingsGroup).arg(#SETTINGS_GROUP_ARRAY_CLASS).arg(index), this); \
        } \
        return _map ## SETTINGS_GROUP_ARRAY_CLASS[index]; \
    }

#define DEFINE_SETTINGS_GROUP_ARRAY(SETTINGS_GROUP_ARRAY_CLASS) \
    private: \
        QMap<int, SETTINGS_GROUP_ARRAY_CLASS*> _map ## SETTINGS_GROUP_ARRAY_CLASS; \
    public: \
        SETTINGS_GROUP_ARRAY_CLASS *get ## SETTINGS_GROUP_ARRAY_CLASS ## ByIndex(int index);

/// Provides access to group of settings. The group is named and has a visible property associated with which can control whether the group
/// is shows in the ui.
class SettingsGroup : public QObject
{
    Q_OBJECT

public:
    /// @param name Name for this Settings group
    /// @param settingsGroup Group to place settings in for QSettings::beingGroup
    SettingsGroup(const QString &name, const QString &settingsGroup, QObject* parent = nullptr);

    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

    virtual bool    visible             () { return _visible; }
    virtual void    setVisible          (bool vis) { _visible = vis; emit visibleChanged(); }

signals:
    void            visibleChanged      ();

protected:
    SettingsFact* _createSettingsFact(const QString& factName);
    bool _visible;
    QString _name;
    QString _settingsGroup;

    QMap<QString, FactMetaData*> _nameToMetaDataMap;

private:
    static constexpr const char* kJsonFile = ":/json/%1.SettingsGroup.json";
};
