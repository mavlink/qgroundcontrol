// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGNOMETHEME_P_H
#define QGNOMETHEME_P_H

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
#include <qpa/qplatformtheme_p.h>
#include <QtGui/QFont>
#if QT_CONFIG(dbus)
#  include "qgnomeportalinterface_p.h"
#endif // QT_CONFIG(dbus)

QT_BEGIN_NAMESPACE

class QGnomeThemePrivate;

class Q_GUI_EXPORT QGnomeTheme : public QGenericUnixTheme
{
protected:
    Q_DECLARE_PRIVATE(QGnomeTheme)

public:
    QGnomeTheme();
    QVariant themeHint(ThemeHint hint) const override;
    QIcon fileIcon(const QFileInfo &fileInfo,
                   QPlatformTheme::IconOptions = { }) const override;
    const QFont *font(Font type) const override;
    QString standardButtonText(int button) const override;

    virtual QString gtkFontName() const;

    virtual void requestColorScheme(Qt::ColorScheme) override;
    virtual Qt::ColorScheme colorScheme() const override;

#if QT_CONFIG(dbus)
protected:
    virtual void updateColorScheme(Qt::ColorScheme);
    virtual void updateHighContrast(Qt::ContrastPreference);

public:
    QPlatformMenuBar *createPlatformMenuBar() const override;
    Qt::ContrastPreference contrastPreference() const override;

#  if QT_CONFIG(systemtrayicon)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#  endif // QT_CONFIG(systemtrayicon)
#endif // QT_CONFIG(dbus)

    static const char *name;
};

class Q_GUI_EXPORT QGnomeThemePrivate : public QGenericUnixThemePrivate
{
    friend QGnomeTheme;

public:
    QGnomeThemePrivate();
    ~QGnomeThemePrivate();

    void configureFonts(const QString &gtkFontName) const;

    Qt::ColorScheme colorScheme() const;
    bool hasRequestedColorScheme() const;

private:
    mutable QFont *systemFont = nullptr;
    mutable QFont *fixedFont = nullptr;

    Qt::ColorScheme m_requestedColorScheme = Qt::ColorScheme::Unknown;

#if QT_CONFIG(dbus)
    QGnomePortalInterface m_gnomePortal;
    QString m_themeName;
#endif // QT_CONFIG(dbus)
};

QT_END_NAMESPACE
#endif // QGNOMETHEME_P_H
