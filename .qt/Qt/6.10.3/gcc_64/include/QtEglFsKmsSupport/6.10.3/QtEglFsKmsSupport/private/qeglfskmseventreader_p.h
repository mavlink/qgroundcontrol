// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKKMSEVENTREADER_H
#define QEGLFSKKMSEVENTREADER_H

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

#include "private/qeglfsglobal_p.h"
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;

struct QEglFSKmsEventHost : public QObject
{
    struct PendingFlipWait {
        void *key;
        QMutex *mutex;
        QWaitCondition *cond;
    };

    static const int MAX_FLIPS = 32;
    void *completedFlips[MAX_FLIPS] = {};
    QEglFSKmsEventHost::PendingFlipWait pendingFlipWaits[MAX_FLIPS] = {};

    bool event(QEvent *event) override;
    void updateStatus();
    void handlePageFlipCompleted(void *key);
};

class QEglFSKmsEventReaderThread : public QThread
{
public:
    QEglFSKmsEventReaderThread(int fd) : m_fd(fd) { }
    void run() override;
    QEglFSKmsEventHost *eventHost() { return &m_ev; }

private:
    int m_fd;
    QEglFSKmsEventHost m_ev;
};

class Q_EGLFS_EXPORT QEglFSKmsEventReader
{
public:
    ~QEglFSKmsEventReader();

    void create(QEglFSKmsDevice *device);
    void destroy();

    void startWaitFlip(void *key, QMutex *mutex, QWaitCondition *cond);

private:
    QEglFSKmsDevice *m_device = nullptr;
    QEglFSKmsEventReaderThread *m_thread = nullptr;
};

QT_END_NAMESPACE

#endif // QEGLFSKKMSEVENTREADER_H
