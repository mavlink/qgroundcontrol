#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "Fact.h"

Q_DECLARE_LOGGING_CATEGORY(SettingsFactLog)

/// A SettingsFact is Fact which holds a QSettings value.
class SettingsFact : public Fact
{
    Q_OBJECT
    /// Whether this setting should be shown in the UI. When false the setting is
    /// hidden from the user and its value is forced to the default. Controlled by
    /// QGCCorePlugin::adjustSettingMetaData and settings-override JSON files.
    Q_PROPERTY(bool userVisible MEMBER _userVisible CONSTANT)

public:
    explicit SettingsFact(QObject *parent = nullptr);
    explicit SettingsFact(const QString &settingsGroup, FactMetaData *metaData, QObject *parent = nullptr);
    explicit SettingsFact(const SettingsFact &other, QObject *parent = nullptr);
    ~SettingsFact();

    const SettingsFact &operator=(const SettingsFact &other);

    // Must be called before any references to fact
    void setUserVisible(bool userVisible) { _userVisible = userVisible; }

private slots:
    void _rawValueChanged(const QVariant &value);

private:
    QString _settingsGroup;
    bool _userVisible = true;
};
