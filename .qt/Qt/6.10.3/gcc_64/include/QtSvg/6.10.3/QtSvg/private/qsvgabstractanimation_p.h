// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGABSTRACTANIMATION_P_H
#define QSVGABSTRACTANIMATION_P_H

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

#include <QtSvg/private/qtsvgglobal_p.h>
#include "qsvganimatedproperty_p.h"
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgAbstractAnimation
{
public:
    enum AnimationType
    {
        CSS,
        SMIL,
    };

    QSvgAbstractAnimation();
    virtual ~QSvgAbstractAnimation();

    virtual AnimationType animationType() const = 0;
    void evaluateAnimation(qreal elapsedTime);

    void setRunningTime(int startMs, int durationMs);
    int start() const;
    int duration() const;

    void setIterationCount(int count);
    int iterationCount() const;

    virtual void appendProperty(QSvgAbstractAnimatedProperty *property);
    QList<QSvgAbstractAnimatedProperty *> properties() const;

    bool finished() const;
    virtual bool isActive() const;

protected:
    int m_start;
    int m_duration;
    bool m_finished;
    int m_iterationCount;
    QList<QSvgAbstractAnimatedProperty *> m_properties;
};

QT_END_NAMESPACE

#endif // QSVGABSTRACTANIMATION_P_H
