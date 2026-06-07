// Copyright (C) 2016 Borgar Ovsthus
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEAMCITYLOGGER_P_H
#define QTEAMCITYLOGGER_P_H

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

#include <QtTest/private/qabstracttestlogger_p.h>

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QTeamCityLogger : public QAbstractTestLogger
{
public:
    QTeamCityLogger(const char *filename);
    ~QTeamCityLogger();

    void startLogging() override;
    void stopLogging() override;

    void enterTestFunction(const char *function) override;
    void leaveTestFunction() override;

    void addIncident(IncidentTypes type, const char *description,
                     const char *file = nullptr, int line = 0) override;
    void addBenchmarkResult(const QBenchmarkResult &result) override;

    void addMessage(MessageTypes type, const QString &message,
                    const char *file = nullptr, int line = 0) override;

private:
    QTestCharBuffer currTestFuncName;
    QTestCharBuffer pendingMessages;
    QTestCharBuffer flowID;

    void tcEscapedString(QTestCharBuffer *buf, const char *str) const;
    void escapedTestFuncName(QTestCharBuffer *buf) const;
    void addPendingMessage(const char *type, const char *msg, const char *file, int line);
};

QT_END_NAMESPACE

#endif
