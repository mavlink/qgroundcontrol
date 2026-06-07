// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPLINK_H
#define QHELPLINK_H

#include <QtHelp/qhelp_global.h>

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

struct QHELP_EXPORT QHelpLink final
{
    QUrl url;
    QString title;
};

QT_END_NAMESPACE

#endif // QHELPLINK_H
