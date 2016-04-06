/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/


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
