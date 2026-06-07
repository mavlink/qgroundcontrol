// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLAINTESTLOGGER_P_H
#define QPLAINTESTLOGGER_P_H

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

class QPlainTestLogger : public QAbstractTestLogger
{
public:
    QPlainTestLogger(const char *filename);
    ~QPlainTestLogger();

    void startLogging() override;
    void stopLogging() override;

    void enterTestFunction(const char *function) override;
    void leaveTestFunction() override;

    void addIncident(IncidentTypes type, const char *description,
                     const char *file = nullptr, int line = 0) override;
    void addBenchmarkResult(const QBenchmarkResult &) final override
    { Q_UNREACHABLE(); }
    void addBenchmarkResults(const QList<QBenchmarkResult> &results) override;

    void addMessage(QtMsgType, const QMessageLogContext &,
                    const QString &) override;

    void addMessage(MessageTypes type, const QString &message,
                    const char *file = nullptr, int line = 0) override;

    bool isRepeatSupported() const override;

private:
    enum class MessageSource {
        Incident,
        Other,
    };
    void printMessage(MessageSource source, const char *type, const char *msg,
                      const char *file = nullptr, int line = 0);
    void outputMessage(const char *str);
    void printBenchmarkResultsHeader(const QBenchmarkResult &result);
    void printBenchmarkResults(const QList<QBenchmarkResult> &result);
};

QT_END_NAMESPACE

#endif
