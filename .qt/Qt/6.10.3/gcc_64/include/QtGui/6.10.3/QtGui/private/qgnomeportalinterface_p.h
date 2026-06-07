// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGNOMEPORTALINTERFACE_P_H
#define QGNOMEPORTALINTERFACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qtconfigmacros.h>
QT_REQUIRE_CONFIG(dbus);

#include <QtCore/QObject>
#include <QtGui/private/qdbuslistener_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QGnomePortalInterface : public QObject
{
    Q_OBJECT

public:
    QGnomePortalInterface(QObject *parent = nullptr);
    ~QGnomePortalInterface() = default;

    Qt::ColorScheme colorScheme(Qt::ColorScheme fallback = Qt::ColorScheme::Unknown) const;
    Qt::ContrastPreference contrastPreference(
            Qt::ContrastPreference fallback = Qt::ContrastPreference::NoPreference) const;

private:
    void querySettings();
    void updateColorScheme(Qt::ColorScheme colorScheme);
    void updateContrast(Qt::ContrastPreference contrast);

Q_SIGNALS:
    void colorSchemeChanged(Qt::ColorScheme);
    void contrastChanged(Qt::ContrastPreference);
    void themeNameChanged(const QString &themeName);

private Q_SLOTS:
    void dbusSettingChanged(QDBusListener::Provider, QDBusListener::Setting, const QVariant &value);

private:
    mutable uint m_version = 0; // cached version value
    std::optional<Qt::ColorScheme> m_colorScheme;
    std::optional<Qt::ContrastPreference> m_contrast;
    std::unique_ptr<QDBusListener> m_dbus;
    static constexpr QLatin1StringView s_service{ "org.freedesktop.portal.Desktop" };
    static constexpr QLatin1StringView s_path{ "/org/freedesktop/portal/desktop" };
    static constexpr QLatin1StringView s_interface{ "org.freedesktop.portal.Settings" };
};

QT_END_NAMESPACE

#endif // QGNOMEPORTALINTERFACE_P_H
