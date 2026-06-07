// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCMYK_P_H
#define QCMYK_P_H

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
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QCmyk32
{
private:
    uint m_cmyk = 0;
    friend constexpr bool comparesEqual(const QCmyk32 &lhs, const QCmyk32 &rhs) noexcept
    {
        return lhs.m_cmyk == rhs.m_cmyk;
    }

public:
    QCmyk32() = default;

    constexpr QCmyk32(int cyan, int magenta, int yellow, int black) :
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        m_cmyk(uint(cyan) << 24 | magenta << 16 | yellow << 8 | black)
#else
        m_cmyk(cyan | magenta << 8 | yellow << 16 | uint(black) << 24)
#endif
    {
    }

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    constexpr int cyan() const noexcept    { return (m_cmyk >> 24) & 0xff; }
    constexpr int magenta() const noexcept { return (m_cmyk >> 16) & 0xff; }
    constexpr int yellow() const noexcept  { return (m_cmyk >>  8) & 0xff; }
    constexpr int black() const noexcept   { return (m_cmyk      ) & 0xff; }
#else
    constexpr int cyan() const noexcept    { return (m_cmyk      ) & 0xff; }
    constexpr int magenta() const noexcept { return (m_cmyk >>  8) & 0xff; }
    constexpr int yellow() const noexcept  { return (m_cmyk >> 16) & 0xff; }
    constexpr int black() const noexcept   { return (m_cmyk >> 24) & 0xff; }
#endif

    QColor toColor() const noexcept
    {
        return QColor::fromCmyk(cyan(), magenta(), yellow(), black());
    }

    constexpr uint toUint() const noexcept
    {
        return m_cmyk;
    }

    constexpr static QCmyk32 fromCmyk32(uint cmyk) noexcept
    {
        QCmyk32 result;
        result.m_cmyk = cmyk;
        return result;
    }

    static QCmyk32 fromRgba(QRgb rgba) noexcept
    {
        const QColor c = QColor(rgba).toCmyk();
        return QCmyk32(c.cyan(), c.magenta(), c.yellow(), c.black());
    }

    static QCmyk32 fromColor(const QColor &color) noexcept
    {
        QColor c = color.toCmyk();
        return QCmyk32(c.cyan(), c.magenta(), c.yellow(), c.black());
    }

    Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(QCmyk32)
};

static_assert(sizeof(QCmyk32) == sizeof(int));
static_assert(alignof(QCmyk32) == alignof(int));
static_assert(std::is_standard_layout_v<QCmyk32>);

QT_END_NAMESPACE

#endif // QCMYK_P_H
