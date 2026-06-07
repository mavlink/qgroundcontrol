// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVXKEYCODEACTION_H
#define QVXKEYCODEACTION_H

#include <QtCore/qtconfigmacros.h>

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

QT_BEGIN_NAMESPACE

enum class QKeycodeAction {
    None               = 0,

    CapsLockOff        = 0x01000000,
    CapsLockOn         = 0x01000001,
    NumLockOff         = 0x02000000,
    NumLockOn          = 0x02000001,
    ScrollLockOff      = 0x03000000,
    ScrollLockOn       = 0x03000001,

    Reboot             = 0x04000000,

    PreviousConsole    = 0x05000000,
    NextConsole        = 0x05000001,
    SwitchConsoleFirst = 0x06000000,
    SwitchConsoleLast  = 0x0600007f,
    SwitchConsoleMask  = 0x0000007f
};

QT_END_NAMESPACE

#endif // QVXKEYCODEACTION_H
