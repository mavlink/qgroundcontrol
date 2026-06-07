// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PLATFORMQUIRKS_P_H
#define PLATFORMQUIRKS_P_H

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

#include <private/qglobal_p.h>

#ifdef Q_OS_MACOS
#include <Carbon/Carbon.h>
#endif

QT_BEGIN_NAMESPACE

struct PlatformQuirks
{
    static inline bool isClipboardAvailable()
    {
#if !QT_CONFIG(clipboard)
        return false;
#elif defined(Q_OS_MACOS)
        PasteboardRef pasteboard;
        OSStatus status = PasteboardCreate(0, &pasteboard);
        if (status == noErr)
            CFRelease(pasteboard);
        return status == noErr;
#else
        if (QGuiApplication::platformName() != QLatin1StringView("xcb"))
            return true;

        // On XCB a clipboard may be dysfunctional due to platform restrictions
        QClipboard *clipBoard = QGuiApplication::clipboard();
        if (!clipBoard)
            return false;
        const QString &oldText = clipBoard->text();
        QScopeGuard guard([&](){ clipBoard->setText(oldText); });
        const QLatin1StringView prefix("Something to prefix ");
        const QString newText = prefix + oldText;
        clipBoard->setText(newText);
        return QTest::qWaitFor([&](){ return clipBoard->text() == newText; });
#endif
    }
};

QT_END_NAMESPACE

#endif
