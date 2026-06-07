// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMATERIALTHEME_P_H
#define QQUICKMATERIALTHEME_P_H

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

#include <QtQuickControls2Material/qtquickcontrols2materialexports.h>

QT_BEGIN_NAMESPACE

class QQuickTheme;
class QQuickMaterialStyle;

class Q_QUICKCONTROLS2MATERIAL_EXPORT QQuickMaterialTheme
{
public:
    static void initialize(QQuickTheme *theme);
    static void registerSystemStyle(QQuickMaterialStyle *style);
    static void unregisterSystemStyle(QQuickMaterialStyle *style);
    static void updateTheme();
};

QT_END_NAMESPACE

#endif // QQUICKMATERIALTHEME_P_H
