// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBENCHMARKMEASUREMENT_P_H
#define QBENCHMARKMEASUREMENT_P_H

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

#include <QtTest/qbenchmark.h>
#include <QtCore/qlist.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QBenchmarkMeasurerBase
{
public:
    struct Measurement
    {
        qreal value;
        QTest::QBenchmarkMetric metric;
    };
    virtual ~QBenchmarkMeasurerBase() = default;
    virtual void start() = 0;
    virtual QList<Measurement> stop() = 0;
    virtual bool isMeasurementAccepted(Measurement m) = 0;
    virtual int adjustIterationCount(int suggestion) = 0;
    virtual int adjustMedianCount(int suggestion) = 0;
    virtual bool needsWarmupIteration() { return false; }
};

QT_END_NAMESPACE

#endif // QBENCHMARKMEASUREMENT_P_H
