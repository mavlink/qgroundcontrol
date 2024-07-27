/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AppMessages.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QStringListModel>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QTextStream>

QGC_LOGGING_CATEGORY(AppLogModelLog, "AppLogModelLog")

Q_GLOBAL_STATIC(AppLogModel, _appLogModel)

static void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    (void) fprintf(stderr, "%s", qPrintable(qFormatLogMessage(type, context, msg)));
}

AppLogModel::AppLogModel(QObject *parent)
    : QStringListModel(parent)
{
    // qCDebug(AppLogModelLog) << Q_FUNC_INFO << this;

    (void) connect(this, &AppLogModel::emitLog, this, &AppLogModel::threadsafeLog, Qt::AutoConnection);
}

AppLogModel::~AppLogModel()
{
    // qCDebug(AppLogModelLog) << Q_FUNC_INFO << this;
}

AppLogModel *AppLogModel::instance()
{
    return _appLogModel();
}

void AppLogModel::installHandler()
{
    qSetMessagePattern("%{category} %{type}: %{message} (%{file}:%{line})\n");
    #ifndef Q_OS_ANDROID
        (void) qInstallMessageHandler(msgHandler);
    #endif
}

void AppLogModel::writeMessages(const QString &destinationFile)
{
    const QString writebuffer(stringList().join('\n').append('\n'));

    const QFuture<void> future = QtConcurrent::run([destinationFile, writebuffer] {
        emit _appLogModel->writeStarted();
        bool success = false;
        QFile file(destinationFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << writebuffer;
            success = (out.status() == QTextStream::Ok);
        } else {
            qCWarning(AppLogModelLog) << "AppLogModel::writeMessages write failed:" << file.errorString();
        }
        emit _appLogModel->writeFinished(success);
    });
}

void AppLogModel::log(const QString &message)
{
    emit _appLogModel->emitLog(message);
}

void AppLogModel::threadsafeLog(const QString &message)
{
    const int line = rowCount();
    (void) insertRows(line, 1);
    (void) setData(index(line), message, Qt::DisplayRole);

    if (qgcApp() && qgcApp()->logOutput() && _logFile.fileName().isEmpty()) {
        qCDebug(AppLogModelLog) << _logFile.fileName().isEmpty() << qgcApp()->logOutput();
        QGCToolbox* const toolbox = qgcApp()->toolbox();
        if (toolbox) {
            const QString saveDirPath = toolbox->settingsManager()->appSettings()->crashSavePath();
            const QDir saveDir(saveDirPath);
            const QString saveFilePath = saveDir.absoluteFilePath(QStringLiteral("QGCConsole.log"));

            _logFile.setFileName(saveFilePath);
            if (!_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qgcApp()->showAppMessage(tr("Open console log output file failed %1 : %2").arg(_logFile.fileName(), _logFile.errorString()));
            }
        }
    }

    if (_logFile.isOpen()) {
        QTextStream out(&_logFile);
        out << message << "\n";
        _logFile.flush();
    }
}
