// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4DEBUGCLIENT_P_H
#define QV4DEBUGCLIENT_P_H

#include "qqmldebugclient_p.h"
#include <QtCore/qjsonvalue.h>

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

class QV4DebugClientPrivate;
class QV4DebugClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QV4DebugClient)

public:
    enum StepAction
    {
        Continue,
        In,
        Out,
        Next
    };

    enum Exception
    {
        All,
        Uncaught
    };

    struct Response
    {
        QString command;
        QJsonValue body;
    };

    QV4DebugClient(QQmlDebugConnection *connection);

    void connect();
    void disconnect();

    void interrupt();
    void continueDebugging(StepAction stepAction);
    void evaluate(const QString &expr, int frame = -1, int context = -1);
    void lookup(const QList<int> &handles, bool includeSource = false);
    void backtrace(int fromFrame = -1, int toFrame = -1, bool bottom = false);
    void frame(int number = -1);
    void scope(int number = -1, int frameNumber = -1);
    void scripts(int types = 4, const QList<int> &ids = QList<int>(), bool includeSource = false);
    void setBreakpoint(const QString &target, int line = -1, int column = -1, bool enabled = true,
                       const QString &condition = QString(), int ignoreCount = -1);
    void clearBreakpoint(int breakpoint);
    void changeBreakpoint(int breakpoint, bool enabled);
    void setExceptionBreak(Exception type, bool enabled = false);
    void version();

    Response response() const;

protected:
    void messageReceived(const QByteArray &data) override;

Q_SIGNALS:
    void connected();
    void interrupted();
    void result();
    void failure();
    void stopped();
};

QT_END_NAMESPACE

#endif // QV4DEBUGCLIENT_P_H
