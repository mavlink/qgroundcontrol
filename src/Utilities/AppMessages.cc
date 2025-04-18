/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AppMessages.h"
#include "AppSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/qapplicationstatic.h>
#include <QtCore/QStringListModel>
#include <QtCore/QTextStream>

QGC_LOGGING_CATEGORY(AppMessagesLog, "AppMessagesLog")

Q_APPLICATION_STATIC(AppMessages, _appMessages)

static QtMessageHandler defaultHandler = nullptr;

static void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const QString message = qFormatLogMessage(type, context, msg);
    AppMessages::instance()->log(output);
    if (defaultHandler) {
        defaultHandler(type, context, msg);
    }
}

AppMessages::AppMessages(QObject *parent)
    : QStringListModel(parent)
{
    // qCDebug(AppMessagesLog) << Q_FUNC_INFO << this;

    (void) connect(this, &AppMessages::emitLog, this, &AppMessages::_threadsafeLog, Qt::AutoConnection);
}

AppMessages::~AppMessages()
{
    // qCDebug(AppMessagesLog) << Q_FUNC_INFO << this;
}

AppMessages *AppMessages::instance()
{
    return _appMessages();
}

void AppMessages::installHandler()
{
    qSetMessagePattern("%{time process} - %{type}: %{message} (%{function}:%{line}) %{category}");
    defaultHandler = qInstallMessageHandler(msgHandler);
}

void AppMessages::writeMessages(const QString &destinationFile)
{
    (void) QtConcurrent::run([this, destinationFile] {
        emit AppMessages::instance()->writeStarted();
        bool success = false;
        QFile file(destinationFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            const QString writebuffer(stringList().join('\n').append('\n'));
            out << writebuffer;
            success = (out.status() == QTextStream::Ok);
        } else {
            qCWarning(AppMessagesLog) << "write failed:" << file.errorString();
        }
        emit writeFinished(success);
    });
}

void AppMessages::log(const QString &message)
{
    emit AppMessages::instance()->emitLog(message);
}

void AppMessages::_threadsafeLog(const QString &message)
{
    const int line = rowCount();
    (void) insertRows(line, 1);
    (void) setData(index(line), message, Qt::DisplayRole);

    if (qgcApp()->logOutput() && _logFile.fileName().isEmpty()) {
        const QString saveDirPath = SettingsManager::instance()->appSettings()->crashSavePath();
        const QDir saveDir(saveDirPath);
        const QString saveFilePath = saveDir.absoluteFilePath(QStringLiteral("QGCConsole.log"));

        _logFile.setFileName(saveFilePath);
        if (!_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showAppMessage(tr("Open console log output file failed %1 : %2").arg(_logFile.fileName(), _logFile.errorString()));
        }
    }

    if (_logFile.isOpen()) {
        QTextStream out(&_logFile);
        out << message << "\n";
        _logFile.flush();
    }
}
