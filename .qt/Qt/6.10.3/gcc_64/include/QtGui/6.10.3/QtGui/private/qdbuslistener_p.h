// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSLISTENER_P_H
#define QDBUSLISTENER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformtheme.h>
#include <private/qflatmap_p.h>
#include <QDBusVariant>

QT_BEGIN_NAMESPACE

class QDBusListener : public QObject
{
    Q_OBJECT

public:

    enum class Provider {
        Kde,
        Gtk,
        Gnome,
    };
    Q_ENUM(Provider)

    enum class Setting {
        Theme,
        ApplicationStyle,
        ColorScheme,
        Contrast,
    };
    Q_ENUM(Setting)

    QDBusListener();
    QDBusListener(const QString &service, const QString &path,
                                  const QString &interface, const QString &signal);

private Q_SLOTS:
    void onSettingChanged(const QString &location, const QString &key, const QDBusVariant &value);

Q_SIGNALS:
    void settingChanged(QDBusListener::Provider provider,
                        QDBusListener::Setting setting,
                        const QVariant &value);

private:
    struct DBusKey
    {
        QString location;
        QString key;
        DBusKey(const QString &loc, const QString &k) : location(loc), key(k) {};
        bool operator<(const DBusKey &other) const
        {
            return location + key < other.location + other.key;
        }
    };

    struct ChangeSignal
    {
        Provider provider;
        Setting setting;
        ChangeSignal(Provider p, Setting s) : provider(p), setting(s) {}
    };

    QFlatMap <DBusKey, ChangeSignal> m_signalMap;

    void init(const QString &service, const QString &path,
              const QString &interface, const QString &signal);

    std::optional<ChangeSignal> findSignal(const QString &location, const QString &key) const;
    void populateSignalMap();
    void loadJson(const QString &fileName);
    void saveJson(const QString &fileName) const;
};

QT_END_NAMESPACE
#endif // QDBUSLISTENER_P_H
