// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKVELOCITYCALCULATOR_P_P_H
#define QQUICKVELOCITYCALCULATOR_P_P_H

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

#include <QtCore/qpoint.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickVelocityCalculator
{
public:
    void startMeasuring(const QPointF &point1, qint64 timestamp);
    void stopMeasuring(const QPointF &m_point2, qint64 timestamp);
    inline void reset() { *this = {}; }
    QPointF velocity() const;

private:
    QPointF m_point1;
    QPointF m_point2;
    qint64 m_point1Timestamp = 0;
    qint64 m_point2Timestamp = 0;
};

QT_END_NAMESPACE

#endif // QQUICKVELOCITYCALCULATOR_P_P_H
