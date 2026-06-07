// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTAPTESTLOGGER_P_H
#define QTAPTESTLOGGER_P_H

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

QT_BEGIN_NAMESPACE

class QTapTestLogger : public QAbstractTestLogger
{
public:
    QTapTestLogger(const char *filename);
    ~QTapTestLogger();

    void startLogging() override;
    void stopLogging() override;

    void enterTestFunction(const char *) override;
    void leaveTestFunction() override {}

    void enterTestData(QTestData *data) override;

    void addIncident(IncidentTypes type, const char *description,
                     const char *file = nullptr, int line = 0) override;
    void addMessage(MessageTypes type, const QString &message,
                    const char *file = nullptr, int line = 0) override;

    void addBenchmarkResult(const QBenchmarkResult &) override {}
private:
    void outputTestLine(bool ok, int testNumber, const QTestCharBuffer &directive);
    void outputBuffer(const QTestCharBuffer &buffer);
    void flushComments();
    void flushMessages();
    void beginYamlish();
    void endYamlish();
    QTestCharBuffer m_firstExpectedFail;
    QTestCharBuffer m_comments;
    QTestCharBuffer m_messages;
    bool m_gatherMessages = false;
};

QT_END_NAMESPACE

#endif // QTAPTESTLOGGER_P_H
