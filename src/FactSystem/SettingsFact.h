/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
    Q_PROPERTY(bool visible MEMBER _visible CONSTANT)

public:
    explicit SettingsFact(QObject *parent = nullptr);
    explicit SettingsFact(const QString &settingsGroup, FactMetaData *metaData, QObject *parent = nullptr);
    explicit SettingsFact(const SettingsFact &other, QObject *parent = nullptr);
    ~SettingsFact();

    const SettingsFact &operator=(const SettingsFact &other);

    // Must be called before any references to fact
    void setVisible(bool visible) { _visible = visible; }

private slots:
    void _rawValueChanged(const QVariant &value);

private:
    QString _settingsGroup;
    bool _visible = true;
};
