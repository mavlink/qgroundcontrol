// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTYLE_P_H
#define QQUICKSTYLE_P_H

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

#include <QtCore/qsharedpointer.h>
#include <QtQuickControls2/qtquickcontrols2global.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QSettings;

class Q_QUICKCONTROLS2_EXPORT QQuickStylePrivate
{
public:
    static QString style();
    static QString effectiveStyleName(const QString &styleName);
    static QString fallbackStyle();
    static bool isCustomStyle();
    static bool isResolved();
    static bool isUsingDefaultStyle();
    static bool exists();
    static void init();
    static void reset();
    static QString configFilePath();
    static QSharedPointer<QSettings> settings(const QString &group = QString());
    static const QFont *readFont(const QSharedPointer<QSettings> &settings);
    static const QPalette *readPalette(const QSharedPointer<QSettings> &settings);
    static bool isDarkSystemTheme();
    static QStringList builtInStyles();
};

QT_END_NAMESPACE

#endif // QQUICKSTYLE_P_H
