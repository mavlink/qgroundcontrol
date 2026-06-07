// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFBVTHANDLER_H
#define QFBVTHANDLER_H

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
#include <QObject>

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QFbVtHandler : public QObject
{
    Q_OBJECT

public:
    QFbVtHandler(QObject *parent = nullptr);
    ~QFbVtHandler();

signals:
    void interrupted();
    void aboutToSuspend();
    void resumed();

private slots:
    void handleSignal();

private:
    void setKeyboardEnabled(bool enable);
    void handleInt();
    static void signalHandler(int sigNo);

    int m_tty;
    int m_oldKbdMode;
    int m_sigFd[2];
    QSocketNotifier *m_signalNotifier;
};

QT_END_NAMESPACE

#endif
