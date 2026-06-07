// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEFFECTS_P_H
#define QEFFECTS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qeffects.cpp, qcombobox.cpp, qpopupmenu.cpp and qtooltip.cpp.
// This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include "QtCore/qnamespace.h"
#include <QtWidgets/private/qtwidgetsglobal_p.h>

QT_REQUIRE_CONFIG(effects);

QT_BEGIN_NAMESPACE

class QWidget;

struct QEffects
{
    enum Direction {
        LeftScroll  = 0x0001,
        RightScroll = 0x0002,
        UpScroll    = 0x0004,
        DownScroll  = 0x0008
    };

    typedef uint DirFlags;
};

extern void Q_WIDGETS_EXPORT qScrollEffect(QWidget*, QEffects::DirFlags dir = QEffects::DownScroll, int time = -1);
extern void Q_WIDGETS_EXPORT qFadeEffect(QWidget*, int time = -1);

QT_END_NAMESPACE

#endif // QEFFECTS_P_H
