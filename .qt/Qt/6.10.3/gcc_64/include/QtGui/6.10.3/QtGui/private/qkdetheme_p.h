// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QKDETHEME_P_H
#define QKDETHEME_P_H

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

#include "qgenericunixtheme_p.h"
#include <qpa/qplatformtheme.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QFont>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QKdeThemePrivate;
class Q_GUI_EXPORT QKdeTheme : public QGenericUnixTheme
{
    Q_DECLARE_PRIVATE(QKdeTheme)
public:
    explicit QKdeTheme(const QStringList& kdeDirs, int kdeVersion);
    ~QKdeTheme() override;

    static QPlatformTheme *createKdeTheme();
    QVariant themeHint(ThemeHint hint) const override;

    QIcon fileIcon(const QFileInfo &fileInfo,
                   QPlatformTheme::IconOptions iconOptions = { }) const override;

    const QPalette *palette(Palette type = SystemPalette) const override;
    Qt::ColorScheme colorScheme() const override;
    void requestColorScheme(Qt::ColorScheme scheme) override;

    const QFont *font(Font type) const override;
#if QT_CONFIG(dbus)
    QPlatformMenuBar *createPlatformMenuBar() const override;
#endif
#if QT_CONFIG(dbus) && QT_CONFIG(systemtrayicon)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    static const char *name;
};

QT_END_NAMESPACE
#endif // QKDETHEME_P_H
