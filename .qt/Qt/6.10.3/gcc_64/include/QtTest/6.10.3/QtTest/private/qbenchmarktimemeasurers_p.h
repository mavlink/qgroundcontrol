// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBENCHMARKTIMEMEASURERS_P_H
#define QBENCHMARKTIMEMEASURERS_P_H

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
#include <QtCore/qelapsedtimer.h>
#include <QtTest/private/cycle_include_p.h>

QT_BEGIN_NAMESPACE

class QBenchmarkTimeMeasurer : public QBenchmarkMeasurerBase
{
public:
    void start() override;
    QList<Measurement> stop() override;
    bool isMeasurementAccepted(Measurement measurement) override;
    int adjustIterationCount(int sugestion) override;
    int adjustMedianCount(int suggestion) override;
    bool needsWarmupIteration() override;
private:
    QElapsedTimer time;
};

#ifdef HAVE_TICK_COUNTER // defined in 3rdparty/cycle/cycle_p.h

class QBenchmarkTickMeasurer : public QBenchmarkMeasurerBase
{
public:
    void start() override;
    QList<Measurement> stop() override;
    bool isMeasurementAccepted(Measurement measurement) override;
    int adjustIterationCount(int) override;
    int adjustMedianCount(int suggestion) override;
    bool needsWarmupIteration() override;
private:
    CycleCounterTicks startTicks;
};

#endif // HAVE_TICK_COUNTER

QT_END_NAMESPACE

#endif // QBENCHMARKTIMEMEASURERS_P_H
