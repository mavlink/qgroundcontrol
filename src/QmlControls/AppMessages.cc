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

#include "AppMessages.h"
#include <QFile>
#include <QStringListModel>
#include <QtConcurrent>
#include <QTextStream>

AppLogModel AppLogModel::instance;
static QtMessageHandler old_handler;
static AppLogModel &debug_strings = AppLogModel::getModel();

static void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char symbols[] = { 'D', 'E', '!', 'X', 'I' };
    QString output = QString("[%1] at %2:%3 - \"%4\"").arg(symbols[type]).arg(context.file).arg(context.line).arg(msg);

    // Avoid recursion
    if (!QString(context.category).startsWith("qt.quick")) {
        const int line = debug_strings.rowCount();
        debug_strings.insertRows(line, 1);
        debug_strings.setData(debug_strings.index(line), output, Qt::DisplayRole);
    }

    if (old_handler != nullptr) {
        old_handler(type, context, msg);
    }
    if( type == QtFatalMsg ) abort();
}

void AppMessages::installHandler()
{
    old_handler = qInstallMessageHandler(msgHandler);
}

AppLogModel *AppMessages::getModel()
{
    return &AppLogModel::getModel();
}

AppLogModel& AppLogModel::getModel()
{
    return instance;
}

AppLogModel::AppLogModel() : QStringListModel()
{
}

void AppLogModel::writeMessages(const QUrl dest_file)
{
    const QString writebuffer(stringList().join('\n').append('\n'));

    QtConcurrent::run([dest_file, writebuffer] {
        emit instance.writeStarted();
        bool success = false;
        QFile file(dest_file.toLocalFile());
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << writebuffer;
            success = out.status() == QTextStream::Ok;
        }
        emit instance.writeFinished(success);
    });
}
