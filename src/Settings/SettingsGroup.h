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

/// Provides access to group of settings. The group is named and has a visible property associated with it which can control whether the group
/// is shown in the UI.
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

    QString settingsGroup() const { return _settingsGroup; }

signals:
    void            visibleChanged      ();

protected:
    SettingsFact* _createSettingsFact(const QString& factName);

    bool _visible;
    QString _name;
    QString _settingsGroup;

    QMap<QString, FactMetaData*> _nameToMetaDataMap;

private:
    static constexpr const char* kJsonFileTemplate = ":/json/%1.SettingsGroup.json";
};
