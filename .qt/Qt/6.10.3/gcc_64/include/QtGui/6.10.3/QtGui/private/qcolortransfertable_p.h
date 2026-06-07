// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRANSFERTABLE_P_H
#define QCOLORTRANSFERTABLE_P_H

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
#include "qcolortransferfunction_p.h"

#include <QList>

#include <algorithm>
#include <cmath>

QT_BEGIN_NAMESPACE

// Defines either an ICC TRC 'curve' or a lut8/lut16 A or B table
class Q_GUI_EXPORT QColorTransferTable
{
public:
    enum Type : uint8_t {
        TwoWay = 0,
        OneWay,
    };
    QColorTransferTable() noexcept = default;
    QColorTransferTable(uint32_t size, const QList<uint8_t> &table, Type type = TwoWay) noexcept
        : m_type(type), m_tableSize(size), m_table8(table)
    {
        Q_ASSERT(qsizetype(size) <= table.size());
    }
    QColorTransferTable(uint32_t size, const QList<uint16_t> &table, Type type = TwoWay) noexcept
        : m_type(type), m_tableSize(size), m_table16(table)
    {
        Q_ASSERT(qsizetype(size) <= table.size());
    }

    bool isEmpty() const noexcept
    {
        return m_tableSize == 0;
    }

    bool isIdentity() const
    {
        if (isEmpty())
            return true;
        if (m_tableSize != 2)
            return false;
        if (!m_table8.isEmpty())
            return m_table8[0] == 0 && m_table8[1] == 255;
        return m_table16[0] == 0 && m_table16[1] == 65535;
    }

    bool checkValidity() const
    {
        if (isEmpty())
            return true;
        // Only one table can be set
        if (!m_table8.isEmpty() && !m_table16.isEmpty())
            return false;
        // At least 2 elements
        if (m_tableSize < 2)
            return false;
        return (m_type == OneWay) || checkInvertibility();
    }
    bool checkInvertibility() const
    {
        // The two-way tables must describe an injective curve:
        if (!m_table8.isEmpty()) {
            uint8_t val = 0;
            for (uint i = 0; i < m_tableSize; ++i) {
                if (m_table8[i] < val)
                    return false;
                val = m_table8[i];
            }
        }
        if (!m_table16.isEmpty()) {
            uint16_t val = 0;
            for (uint i = 0; i < m_tableSize; ++i) {
                if (m_table16[i] < val)
                    return false;
                val = m_table16[i];
            }
        }
        return true;
    }

    float apply(float x) const
    {
        if (isEmpty())
            return x;
        x = std::clamp(x, 0.0f, 1.0f);
        x *= m_tableSize - 1;
        const uint32_t lo = static_cast<uint32_t>(x);
        const uint32_t hi = std::min(lo + 1, m_tableSize - 1);
        const float frac = x - lo;
        if (!m_table16.isEmpty())
            return (m_table16[lo] + (m_table16[hi] - m_table16[lo]) * frac) * (1.0f/65535.0f);
        if (!m_table8.isEmpty())
            return (m_table8[lo] + (m_table8[hi] - m_table8[lo]) * frac) * (1.0f/255.0f);
        return x;
    }

    // Apply inverse, optimized by giving a previous result for a value < x.
    float applyInverse(float x, float resultLargerThan = 0.0f) const
    {
        Q_ASSERT(resultLargerThan >= 0.0f && resultLargerThan <= 1.0f);
        Q_ASSERT(m_type == TwoWay);
        if (x <= 0.0f)
            return 0.0f;
        if (x >= 1.0f)
            return 1.0f;
        if (!m_table16.isEmpty())
            return inverseLookup(x * 65535.0f, resultLargerThan, m_table16, m_tableSize - 1);
        if (!m_table8.isEmpty())
            return inverseLookup(x * 255.0f, resultLargerThan, m_table8, m_tableSize - 1);
        return x;
    }

    bool asColorTransferFunction(QColorTransferFunction *transferFn)
    {
        Q_ASSERT(transferFn);
        if (isEmpty()) {
            *transferFn = QColorTransferFunction();
            return true;
        }
        if (m_tableSize < 2)
            return false;
        if (!m_table8.isEmpty() && (m_table8[0] != 0 || m_table8[m_tableSize - 1] != 255))
            return false;
        if (!m_table16.isEmpty() && (m_table16[0] != 0 || m_table16[m_tableSize - 1] != 65535))
            return false;
        if (m_tableSize == 2) {
            *transferFn = QColorTransferFunction(); // Linear
            return true;
        }
        // The following heuristics are based on those from Skia:
        if (m_tableSize == 26 && !m_table16.isEmpty()) {
            // code.facebook.com/posts/411525055626587/under-the-hood-improving-facebook-photos
            if (m_table16[6] != 3062)
                return false;
            if (m_table16[12] != 12824)
                return false;
            if (m_table16[18] != 31237)
                return false;
            *transferFn = QColorTransferFunction::fromSRgb();
            return true;
        }
        if (m_tableSize == 1024 && !m_table16.isEmpty()) {
            // HP and Canon sRGB gamma tables:
            if (m_table16[257] != 3366)
                return false;
            if (m_table16[513] != 14116)
                return false;
            if (m_table16[768] != 34318)
                return false;
            *transferFn = QColorTransferFunction::fromSRgb();
            return true;
        }
        if (m_tableSize == 4096 && !m_table16.isEmpty()) {
            // Nikon, Epson, and lcms2 sRGB gamma tables:
            if (m_table16[515] != 960)
                return false;
            if (m_table16[1025] != 3342)
                return false;
            if (m_table16[2051] != 14079)
                return false;
            *transferFn = QColorTransferFunction::fromSRgb();
            return true;
        }
        return false;
    }
    friend inline bool operator!=(const QColorTransferTable &t1, const QColorTransferTable &t2);
    friend inline bool operator==(const QColorTransferTable &t1, const QColorTransferTable &t2);

    Type m_type = TwoWay;
    uint32_t m_tableSize = 0;
    QList<uint8_t> m_table8;
    QList<uint16_t> m_table16;
private:
    template<typename T>
    static float inverseLookup(float needle, float resultLargerThan, const QList<T> &table, quint32 tableMax)
    {
        uint32_t i = qMax(static_cast<uint32_t>(resultLargerThan * tableMax), 1U) - 1;
        auto it = std::lower_bound(table.cbegin() + i, table.cend(), needle);
        i = it - table.cbegin();
        if (i == 0)
            return 0.0f;
        if (i >= tableMax)
            return 1.0f;
        const float y1 = table[i - 1];
        const float y2 = table[i];
        Q_ASSERT(needle >= y1 && needle <= y2);
        const float fr = (needle - y1) / (y2 - y1);
        return (i + fr) * (1.0f / tableMax);
    }

};

inline bool operator!=(const QColorTransferTable &t1, const QColorTransferTable &t2)
{
    if (t1.m_tableSize != t2.m_tableSize)
        return true;
    if (t1.m_type != t2.m_type)
        return true;
    if (t1.m_table8.isEmpty() != t2.m_table8.isEmpty())
        return true;
    if (t1.m_table16.isEmpty() != t2.m_table16.isEmpty())
        return true;
    if (!t1.m_table8.isEmpty()) {
        for (uint32_t i = 0; i < t1.m_tableSize; ++i) {
            if (t1.m_table8[i] != t2.m_table8[i])
                return true;
        }
    }
    if (!t1.m_table16.isEmpty()) {
        for (uint32_t i = 0; i < t1.m_tableSize; ++i) {
            if (t1.m_table16[i] != t2.m_table16[i])
                return true;
        }
    }
    return false;
}

inline bool operator==(const QColorTransferTable &t1, const QColorTransferTable &t2)
{
    return !(t1 != t2);
}

QT_END_NAMESPACE

#endif // QCOLORTRANSFERTABLE_P_H
