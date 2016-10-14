/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/



// Allows QGlobalStatic to work on this translation unit
#define _LOG_CTOR_ACCESS_ public
#include "AppMessages.h"
#include <QFile>
#include <QStringListModel>
#include <QtConcurrent>
#include <QTextStream>

Q_GLOBAL_STATIC(AppLogModel, debug_model)

static QtMessageHandler old_handler;

static void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char symbols[] = { 'D', 'E', '!', 'X', 'I' };
    QString output = QString("[%1] at %2:%3 - \"%4\"").arg(symbols[type]).arg(context.file).arg(context.line).arg(msg);

    // Avoid recursion
    if (!QString(context.category).startsWith("qt.quick")) {
        debug_model->log(output);
    }

    if (old_handler != nullptr) {
        old_handler(type, context, msg);
    }
    if( type == QtFatalMsg ) abort();
}

void AppMessages::installHandler()
{
    old_handler = qInstallMessageHandler(msgHandler);

    // Force creation of debug model on installing thread
    Q_UNUSED(*debug_model);
}

AppLogModel *AppMessages::getModel()
{
    return debug_model;
}

AppLogModel::AppLogModel() : QStringListModel()
{
#ifdef __mobile__
    Qt::ConnectionType contype = Qt::QueuedConnection;
#else
    Qt::ConnectionType contype = Qt::AutoConnection;
#endif
    connect(this, &AppLogModel::emitLog, this, &AppLogModel::threadsafeLog, contype);
}

void AppLogModel::writeMessages(const QUrl dest_file)
{
    const QString writebuffer(stringList().join('\n').append('\n'));

    QtConcurrent::run([dest_file, writebuffer] {
        emit debug_model->writeStarted();
        bool success = false;
        QFile file(dest_file.toLocalFile());
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << writebuffer;
            success = out.status() == QTextStream::Ok;
        }
        emit debug_model->writeFinished(success);
    });
}

void AppLogModel::log(const QString message)
{
    emit debug_model->emitLog(message);
}

void AppLogModel::threadsafeLog(const QString message)
{
    const int line = rowCount();
    insertRows(line, 1);
    setData(index(line), message, Qt::DisplayRole);
}
