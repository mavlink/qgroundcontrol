// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVDEVTABLETHANDLER_P_H
#define QEVDEVTABLETHANDLER_P_H

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

#include <QObject>
#include <QString>
#include <QThread>
#include <QLoggingCategory>
#include <QtCore/private/qthread_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEvdevTablet)

class QSocketNotifier;
class QEvdevTabletData;

class QEvdevTabletHandler : public QObject
{
public:
    explicit QEvdevTabletHandler(const QString &device, const QString &spec = QString(), QObject *parent = nullptr);
    ~QEvdevTabletHandler();

    qint64 deviceId() const;

    void readData();

private:
    bool queryLimits();

    int m_fd;
    QString m_device;
    QSocketNotifier *m_notifier;
    QEvdevTabletData *d;
};

class QEvdevTabletHandlerThread : public QDaemonThread
{
public:
    explicit QEvdevTabletHandlerThread(const QString &device, const QString &spec, QObject *parent = nullptr);
    ~QEvdevTabletHandlerThread();
    void run() override;
    QEvdevTabletHandler *handler() { return m_handler; }

private:
    QString m_device;
    QString m_spec;
    QEvdevTabletHandler *m_handler;
};

QT_END_NAMESPACE

#endif // QEVDEVTABLETHANDLER_P_H
