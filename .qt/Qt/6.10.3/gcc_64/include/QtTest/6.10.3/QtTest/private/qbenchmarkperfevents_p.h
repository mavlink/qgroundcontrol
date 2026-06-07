// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBENCHMARKPERFEVENTS_P_H
#define QBENCHMARKPERFEVENTS_P_H

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

#include <QtTest/private/qbenchmarkmeasurement_p.h>

QT_BEGIN_NAMESPACE

class QBenchmarkPerfEventsMeasurer : public QBenchmarkMeasurerBase
{
public:
    QBenchmarkPerfEventsMeasurer();
    ~QBenchmarkPerfEventsMeasurer();
    void start() override;
    QList<Measurement> stop() override;
    bool isMeasurementAccepted(Measurement measurement) override;
    int adjustIterationCount(int suggestion) override;
    int adjustMedianCount(int suggestion) override;
    bool needsWarmupIteration() override { return true; }

    static bool isAvailable();
    static void setCounter(const char *name);
    static void listCounters();
private:
    QList<int> fds;

    Measurement readValue(qsizetype idx = 0);
};

QT_END_NAMESPACE

#endif // QBENCHMARKPERFEVENTS_P_H
