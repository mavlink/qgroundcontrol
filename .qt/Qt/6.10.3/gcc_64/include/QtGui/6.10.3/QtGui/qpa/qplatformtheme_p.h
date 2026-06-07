// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMTHEME_P_H
#define QPLATFORMTHEME_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#if QT_CONFIG(shortcut)
#  include "private/qkeysequence_p.h"
#endif

QT_BEGIN_NAMESPACE

class QPalette;

class Q_GUI_EXPORT QPlatformThemePrivate
{
public:
    QPlatformThemePrivate();

    virtual ~QPlatformThemePrivate();

    void initializeSystemPalette();

#if QT_CONFIG(shortcut)
    static const QKeyBinding keyBindings[];
    static const uint numberOfKeyBindings;
#endif

    static unsigned currentKeyPlatforms();

    QPalette *systemPalette;

    QString name;
};

QT_END_NAMESPACE

#endif // QPLATFORMTHEME_P_H
