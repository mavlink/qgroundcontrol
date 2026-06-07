// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLENGINEDEBUGCLIENT_P_P_H
#define QQMLENGINEDEBUGCLIENT_P_P_H

#include "qqmlenginedebugclient_p.h"
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

class QQmlEngineDebugClientPrivate : public QQmlDebugClientPrivate
{
    Q_DECLARE_PUBLIC(QQmlEngineDebugClient)
public:
    QQmlEngineDebugClientPrivate(QQmlDebugConnection *connection);

    qint32 nextId = 0;
    bool valid = false;
    QList<QQmlEngineDebugEngineReference> engines;
    QQmlEngineDebugContextReference rootContext;
    QQmlEngineDebugObjectReference object;
    QList<QQmlEngineDebugObjectReference> objects;
    QVariant exprResult;
};

QT_END_NAMESPACE

#endif // QQMLENGINEDEBUGCLIENT_P_P_H
