// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPREVIEWCLIENT_P_P_H
#define QQMLPREVIEWCLIENT_P_P_H

#include "qqmlpreviewclient_p.h"
#include "qqmldebugclient_p_p.h"

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

class QQmlPreviewClientPrivate : public QQmlDebugClientPrivate
{
    Q_DECLARE_PUBLIC(QQmlPreviewClient)
public:
    QQmlPreviewClientPrivate(QQmlDebugConnection *connection)
        : QQmlDebugClientPrivate(QLatin1String("QmlPreview"), connection)
    {}
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWCLIENT_P_P_H
