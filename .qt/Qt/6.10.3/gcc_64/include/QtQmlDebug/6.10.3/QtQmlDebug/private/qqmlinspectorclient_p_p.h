// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINSPECTORCLIENT_P_P_H
#define QQMLINSPECTORCLIENT_P_P_H

#include "qqmlinspectorclient_p.h"
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

class QQmlInspectorClientPrivate : public QQmlDebugClientPrivate
{
    Q_DECLARE_PUBLIC(QQmlInspectorClient)
public:
    QQmlInspectorClientPrivate(QQmlDebugConnection *connection);
    int m_lastRequestId = -1;
};

QT_END_NAMESPACE

#endif // QQMLINSPECTORCLIENT_P_P_H
