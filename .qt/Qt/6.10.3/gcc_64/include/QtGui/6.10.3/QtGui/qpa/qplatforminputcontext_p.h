// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINPUTCONTEXT_P_H
#define QPLATFORMINPUTCONTEXT_P_H

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
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QPlatformInputContextPrivate: public QObjectPrivate
{
public:
    QPlatformInputContextPrivate() {}
    ~QPlatformInputContextPrivate() {}

    static void setInputMethodAccepted(bool accepted);
    static bool inputMethodAccepted();

    static bool s_inputMethodAccepted;
};

QT_END_NAMESPACE

#endif
