// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGSERVER_P_H
#define QQMLDEBUGSERVER_P_H

#include "qqmldebugconnector_p.h"

#include <private/qtqmlglobal_p.h>
#include <QtCore/QIODevice>

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

class Q_QML_EXPORT QQmlDebugServer : public QQmlDebugConnector
{
    Q_OBJECT
public:
    ~QQmlDebugServer() override;
    virtual void setDevice(QIODevice *socket) = 0;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVER_P_H
