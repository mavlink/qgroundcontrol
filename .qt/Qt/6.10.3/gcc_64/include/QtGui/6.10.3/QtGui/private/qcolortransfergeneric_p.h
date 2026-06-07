// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRANSFERGENERIC_P_H
#define QCOLORTRANSFERGENERIC_P_H

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

#include <QtGui/private/qtguiglobal_p.h>

#include <cmath>

QT_BEGIN_NAMESPACE

// Defines the a generic transfer function for our HDR functions
class QColorTransferGenericFunction
{
public:
    using ConverterPtr = float (*)(float);
    constexpr QColorTransferGenericFunction(ConverterPtr toLinear = nullptr, ConverterPtr fromLinear = nullptr) noexcept
        : m_toLinear(toLinear), m_fromLinear(fromLinear)
    {}

    static QColorTransferGenericFunction hlg()
    {
        return QColorTransferGenericFunction(hlgToLinear, hlgFromLinear);
    }
    static QColorTransferGenericFunction pq()
    {
        return QColorTransferGenericFunction(pqToLinear, pqFromLinear);
    }

    float apply(float x) const
    {
        return m_toLinear(x);
    }

    float applyInverse(float x) const
    {
        return m_fromLinear(x);
    }

    bool operator==(const QColorTransferGenericFunction &o) const noexcept
    {
        return m_toLinear == o.m_toLinear && m_fromLinear == o.m_fromLinear;
    }
    bool operator!=(const QColorTransferGenericFunction &o) const noexcept
    {
        return m_toLinear != o.m_toLinear || m_fromLinear != o.m_fromLinear;
    }

private:
    ConverterPtr m_toLinear = nullptr;
    ConverterPtr m_fromLinear = nullptr;

    // HLG from linear [0-12] -> [0-1]
    static float hlgFromLinear(float x)
    {
        x = std::clamp(x, 0.f, 12.f);
        if (x > 1.f)
            return m_hlg_a * std::log(x - m_hlg_b) + m_hlg_c;
        return std::sqrt(x * 0.25f);
    }

    // HLG to linear [0-1] -> [0-12]
    static float hlgToLinear(float x)
    {
        x = std::clamp(x, 0.f, 1.f);
        if (x < 0.5f)
            return (x * x) * 4.f;
        return std::exp((x - m_hlg_c) / m_hlg_a) + m_hlg_b;
    }

    constexpr static float m_hlg_a = 0.17883277f;
    constexpr static float m_hlg_b = 1.f - (4.f * m_hlg_a);
    constexpr static float m_hlg_c = 0.55991073f; // 0.5 - a * ln(4 * a)

    // BT.2100-2 Reference PQ EOTF and inverse (see Table 4)
    // PQ to linear [0-1] -> [0-64]
    static float pqToLinear(float e)
    {
        e = std::clamp(e, 0.f, 1.f);
        // m2-th root of E'
        const float eRoot = std::pow(e, 1.f / m_pq_m2);
        // rational transform
        const float yBase = (std::max)(eRoot - m_pq_c1, 0.0f) / (m_pq_c2 - m_pq_c3 * eRoot);
        // calculate Y = yBase^(1/m1)
        const float y = std::pow(yBase, 1.f / m_pq_m1);
        // scale Y to Fd
        return y * m_pq_f;
    }

    // PQ from linear [0-64] -> [0-1]
    static float pqFromLinear(float fd)
    {
        fd = std::clamp(fd, 0.f, 64.f);
        // scale Fd to Y
        const float y = fd * (1.f / m_pq_f);
        // yRoot = Y^m1 -- "root" because m1 is <1
        const float yRoot = std::pow(y, m_pq_m1);
        // rational transform
        const float eBase = (m_pq_c1 + m_pq_c2 * yRoot) / (1.f + m_pq_c3 * yRoot);
        // calculate E' = eBase^m2
        return std::pow(eBase, m_pq_m2);
    }

    constexpr static float m_pq_c1 =  107.f / 128.f; // c3 - c2 + 1
    constexpr static float m_pq_c2 = 2413.f / 128.f;
    constexpr static float m_pq_c3 = 2392.f / 128.f;
    constexpr static float m_pq_m1 = 1305.f / 8192.f;
    constexpr static float m_pq_m2 = 2523.f / 32.f;
    constexpr static float m_pq_f = 64.f; // This might need to be set based on scene metadata
};

QT_END_NAMESPACE

#endif // QCOLORTRANSFERGENERIC_P_H
