// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLENGINECONTROLCLIENT_P_P_H
#define QQMLENGINECONTROLCLIENT_P_P_H

#include "qqmlenginecontrolclient_p.h"
#include "qqmldebugclient_p_p.h"

#include <QtCore/QHash>

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

class QQmlEngineControlClientPrivate : public QQmlDebugClientPrivate
{
    Q_DECLARE_PUBLIC(QQmlEngineControlClient)
public:
    enum MessageType {
        EngineAboutToBeAdded,
        EngineAdded,
        EngineAboutToBeRemoved,
        EngineRemoved
    };

    enum CommandType {
        StartWaitingEngine,
        StopWaitingEngine,
        InvalidCommand
    };

    QQmlEngineControlClientPrivate(QQmlDebugConnection *connection);

    void sendCommand(CommandType command, int engineId);

    struct EngineState {
        EngineState(CommandType command = InvalidCommand) : releaseCommand(command), blockers(0) {}
        CommandType releaseCommand;
        int blockers;
    };

    QHash<int, EngineState> blockedEngines;
};

QT_END_NAMESPACE

#endif // QQMLENGINECONTROLCLIENT_P_P_H
