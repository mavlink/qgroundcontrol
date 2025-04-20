/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

class QSocketNotifier;

Q_DECLARE_LOGGING_CATEGORY(SignalHandlerLog)

class SignalHandler : public QObject
{
    Q_OBJECT

public:
    explicit SignalHandler(QObject *parent = nullptr);

    static SignalHandler *instance();

    static int setupSignalHandlers();
    static void intSignalHandler(int signum);
    static void termSignalHandler(int signum);

private slots:
    void _onSigInt();
    void _onSigTerm();

private:
    static int sigIntFd[2];
    static int sigTermFd[2];

    QSocketNotifier *_notifierInt = nullptr;
    QSocketNotifier *_notifierTerm = nullptr;
};
