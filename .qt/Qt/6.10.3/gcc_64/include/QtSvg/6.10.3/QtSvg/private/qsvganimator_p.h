// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGANIMATOR_P_H
#define QSVGANIMATOR_P_H

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
#include <QtSvg/private/qsvgnode_p.h>
#include "qsvgabstractanimation_p.h"

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgAbstractAnimator
{
public:
    QSvgAbstractAnimator();
    virtual ~QSvgAbstractAnimator();

    void appendAnimation(const QSvgNode *node, QSvgAbstractAnimation *anim);
    QList<QSvgAbstractAnimation *> animationsForNode(const QSvgNode *node) const;

    void advanceAnimations();
    virtual void restartAnimation() = 0;
    virtual qint64 currentElapsed() = 0;
    virtual void setAnimatorTime(qint64 time) = 0;

    void setAnimationDuration(qint64 dur);
    qint64 animationDuration() const;

protected:
    qint64 m_time;
    qint64 m_animationDuration;

private:
    QList<QSvgAbstractAnimation *> combinedAnimationsForNode(const QSvgNode *node) const;

private:
    QHash<const QSvgNode *, QList<QSvgAbstractAnimation *>> m_animationsSMIL;
    QHash<const QSvgNode *, QList<QSvgAbstractAnimation *>> m_animationsCSS;
};

class Q_SVG_EXPORT QSvgAnimator : public QSvgAbstractAnimator
{
public:
    QSvgAnimator();
    ~QSvgAnimator();

    virtual void restartAnimation() override;
    virtual qint64 currentElapsed() override;
    virtual void setAnimatorTime(qint64 time) override;
};

class Q_SVG_EXPORT QSvgAnimationController : public QSvgAbstractAnimator
{
public:
    QSvgAnimationController();
    ~QSvgAnimationController();

    virtual void restartAnimation() override;
    virtual qint64 currentElapsed() override;
    virtual void setAnimatorTime(qint64 time) override;
};


QT_END_NAMESPACE

#endif // QSVGANIMATOR_P_H
