// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QPRIMEFORNUMBITS_P_H
#define QPRIMEFORNUMBITS_P_H

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

#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

/*
    The prime_deltas array is a table of selected prime values, even
    though it doesn't look like one. The primes we are using are 1,
    2, 5, 11, 17, 37, 67, 131, 257, ..., i.e. primes in the immediate
    surrounding of a power of two.

    The qPrimeForNumBits() function returns the prime associated to a
    power of two. For example, qPrimeForNumBits(8) returns 257.
*/

inline int qPrimeForNumBits(int numBits)
{
    static constexpr const uchar prime_deltas[] = {
        0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3,  9, 25,  3,
        1, 21,  3, 21,  7, 15,  9,  5,  3, 29, 15,  0,  0,  0,  0,  0
    };

    return (1 << numBits) + prime_deltas[numBits];
}

QT_END_NAMESPACE

#endif // QPRIMEFORNUMBITS_P_H
