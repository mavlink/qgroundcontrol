/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStringListModel>

Q_DECLARE_LOGGING_CATEGORY(AppMessagesLog)

class AppMessages : public QStringListModel
{
    Q_OBJECT

public:
    AppMessages(QObject *parent = nullptr);
    ~AppMessages();

    static AppMessages *instance();
    static void installHandler();
    static void log(const QString &message);

    Q_INVOKABLE void writeMessages(const QString &destinationFile);

signals:
    void emitLog(const QString &message);
    void writeStarted();
    void writeFinished(bool success);

private slots:
    void _threadsafeLog(const QString &message);

private:
    QFile _logFile;
};
