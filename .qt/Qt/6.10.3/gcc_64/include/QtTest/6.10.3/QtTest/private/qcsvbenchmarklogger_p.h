// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCSVBENCHMARKLOGGER_P_H
#define QCSVBENCHMARKLOGGER_P_H

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

#include "qabstracttestlogger_p.h"

QT_BEGIN_NAMESPACE

class QCsvBenchmarkLogger : public QAbstractTestLogger
{
public:
    QCsvBenchmarkLogger(const char *filename);
    ~QCsvBenchmarkLogger();

    void startLogging() override;
    void stopLogging() override;

    void enterTestFunction(const char *function) override;
    void leaveTestFunction() override;

    void addIncident(IncidentTypes type, const char *description,
                     const char *file = nullptr, int line = 0) override;
    void addBenchmarkResult(const QBenchmarkResult &result) override;

    void addMessage(MessageTypes type, const QString &message,
                            const char *file = nullptr, int line = 0) override;
};

QT_END_NAMESPACE

#endif // QCSVBENCHMARKLOGGER_P_H
