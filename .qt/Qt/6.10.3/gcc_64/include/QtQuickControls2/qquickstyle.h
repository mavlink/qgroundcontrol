// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTYLE_H
#define QQUICKSTYLE_H

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>
#include <QtQuickControls2/qtquickcontrols2global.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2_EXPORT QQuickStyle
{
public:
    static QString name();
    static void setStyle(const QString &style);
    static void setFallbackStyle(const QString &style);
};

QT_END_NAMESPACE

#endif // QQUICKSTYLE_H
