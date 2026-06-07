// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QUUID_P_H
#define QUUID_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "quuid.h"

#include <QtCore/qrandom.h>

#include <chrono>

QT_BEGIN_NAMESPACE

#ifndef QT_BOOTSTRAPPED
static inline QUuid createUuidV7_internal(
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> tp)
{
    QUuid result;

    using namespace std::chrono;
    const nanoseconds nsecSinceEpoch = tp.time_since_epoch();
    const auto msecSinceEpoch = floor<milliseconds>(nsecSinceEpoch);
    const quint64 frac = (nsecSinceEpoch - msecSinceEpoch).count();
    // Lower 48 bits of the timestamp
    const quint64 msecs = quint64(msecSinceEpoch.count()) & 0xffffffffffff;
    result.data1 = uint(msecs >> 16);
    result.data2 = ushort(msecs);
    // rand_a: use a 12-bit sub-millisecond timestamp for additional monotonicity
    // https://datatracker.ietf.org/doc/html/rfc9562#monotonicity_counters (Method 3)

    // "frac" is a number between 0 and 999,999, so the lowest 20 bits
    // should be roughly random. Use the high 12 of those for additional
    // monotonicity.
    result.data3 = frac >> 8;
    result.data3 &= 0x0FFF;
    result.data3 |= ushort(7) << 12;

    // rand_b: 62 bits of random data (64 - 2 bits for the variant)
    const quint64 random = QRandomGenerator::system()->generate64();
    memcpy(result.data4, &random, sizeof(quint64));
    result.data4[0] = (result.data4[0] & 0x3F) | 0x80; // UV_DCE
    return result;
}
#endif

QT_END_NAMESPACE

#endif // QUUID_P_H
