// Copyright (C) 2024 Christian Ehrlicher <ch.ehrlicher@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAINTERSTATEGUARD_H
#define QPAINTERSTATEGUARD_H

#include <QtCore/qtclasshelpermacros.h>
#include <QtGui/qpainter.h>

QT_BEGIN_NAMESPACE

class QPainterStateGuard
{
    Q_DISABLE_COPY(QPainterStateGuard)
public:
    enum class InitialState : quint8 {
        Save,
        NoSave,
    };

    QPainterStateGuard(QPainterStateGuard &&other) noexcept
        : m_painter(std::exchange(other.m_painter, nullptr))
        , m_level(std::exchange(other.m_level, 0))
    {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QPainterStateGuard)
    void swap(QPainterStateGuard &other) noexcept
    {
        qt_ptr_swap(m_painter, other.m_painter);
        std::swap(m_level, other.m_level);
    }

    Q_NODISCARD_CTOR
    explicit QPainterStateGuard(QPainter *painter, InitialState state = InitialState::Save)
        : m_painter(painter)
    {
        verifyPainter();
        if (state == InitialState::Save)
            save();
    }

    ~QPainterStateGuard()
    {
        while (m_level > 0)
            restore();
    }

    void save()
    {
        verifyPainter();
        m_painter->save();
        ++m_level;
    }

    void restore()
    {
        verifyPainter();
        Q_ASSERT(m_level > 0);
        --m_level;
        m_painter->restore();
    }

private:
    void verifyPainter()
    {
        Q_ASSERT(m_painter);
    }

    QPainter *m_painter;
    int m_level = 0;
};

QT_END_NAMESPACE

#endif  // QPAINTERSTATEGUARD_H
