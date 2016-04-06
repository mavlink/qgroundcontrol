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

#pragma once

#include <QObject>
#include <QStringListModel>
#include <QUrl>

// Hackish way to force only this translation unit to have public ctor access
#ifndef _LOG_CTOR_ACCESS_
#define _LOG_CTOR_ACCESS_ private
#endif

class AppLogModel : public QStringListModel
{
    Q_OBJECT
public:
    Q_INVOKABLE void writeMessages(const QUrl dest_file);
    static void log(const QString message);

signals:
    void emitLog(const QString message);
    void writeStarted();
    void writeFinished(bool success);

private slots:
    void threadsafeLog(const QString message);

_LOG_CTOR_ACCESS_:
    AppLogModel();
};


class AppMessages
{
public:
    static void installHandler();
    static AppLogModel* getModel();
};
