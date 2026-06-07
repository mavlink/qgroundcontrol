// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTHEME_P_P_H
#define QQUICKTHEME_P_P_H

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

#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_EXPORT QQuickThemePrivate
{
public:
    static QQuickThemePrivate *get(QQuickTheme *theme)
    {
        return theme->d_func();
    }

    static std::unique_ptr<QQuickTheme> instance;

    static const int NScopes = QQuickTheme::Tumbler + 1;

    QScopedPointer<const QFont> defaultFont;
    QScopedPointer<const QPalette> defaultPalette;
    QSharedPointer<QFont> fonts[NScopes];
    QSharedPointer<QPalette> palettes[NScopes];
};

QT_END_NAMESPACE

#endif // QQUICKTHEME_P_P_H
