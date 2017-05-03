/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
