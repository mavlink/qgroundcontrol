// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSSETTINGS_P_H
#define QDBUSSETTINGS_P_H

#include <QtCore/qnamespace.h>
#include <QtCore/qstring.h>
QT_REQUIRE_CONFIG(dbus);

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

QT_BEGIN_NAMESPACE

namespace QDBusSettings {
// XDG Desktop Portal Settings (Preferred)
// https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html
namespace XdgSettings {
static constexpr QLatin1StringView AppearanceNamespace{ "org.freedesktop.appearance" };
static constexpr QLatin1StringView ColorSchemeKey{ "color-scheme" };
static constexpr QLatin1StringView ContrastKey{ "contrast" };
Qt::ContrastPreference convertContrastPreference(const QVariant &value);
Qt::ColorScheme convertColorScheme(const QVariant &value);
} // namespace XdgSettings

namespace GnomeSettings {
// GNOME Destop Settings (Alternative)
// https://gitlab.gnome.org/GNOME/gnome-shell/-/merge_requests/2069
// https://gitlab.gnome.org/GNOME/gsettings-desktop-schemas/-/commit/0e97f1f571c495184f80d875c68f241261a50e30
static constexpr QLatin1StringView AllyNamespace{ "org.gnome.desktop.a11y.interface" };
static constexpr QLatin1StringView ContrastKey{ "high-contrast" };
Qt::ContrastPreference convertContrastPreference(const QVariant &value);
} // namespace GnomeSettings
} // namespace QDBusSettings

QT_END_NAMESPACE
#endif // QDBUSSETTINGS_P_H
