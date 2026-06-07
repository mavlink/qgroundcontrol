// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGANIMATE_P_H
#define  QSVGANIMATE_P_H

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

#include "qsvgnode_p.h"
#include "private/qsvgabstractanimation_p.h"

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgAnimateNode : public QSvgNode, public QSvgAbstractAnimation
{
public:
    enum Additive
    {
        Sum,
        Replace
    };
    enum Fill
    {
        Freeze,
        Remove
    };

public:
    QSvgAnimateNode(QSvgNode *parent = nullptr);
    virtual ~QSvgAnimateNode() {}

    void setLinkId(const QString &link) { m_linkId = link; }
    const QString &linkId() const { return m_linkId; }

    virtual AnimationType animationType() const override { return AnimationType::SMIL; }
    virtual bool isActive() const override { return !finished() || m_fill == Fill::Freeze; }

    void setRunningTime(int startMs, int durMs, int endMs, int by);
    void setRepeatCount(qreal repeatCount) { setIterationCount(repeatCount); }

    void setFill(Fill fill) { m_fill = fill; }
    Fill fill() const { return m_fill; }

    void setAdditiveType(Additive additive = Additive::Replace) { m_additive = additive; }
    Additive additiveType() const { return m_additive; }

    virtual void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    virtual bool shouldDrawNode(QPainter *p, QSvgExtraStates &states) const override;

protected:
    qreal m_end;
    Fill m_fill;
    Additive m_additive;
    QString m_linkId;
};

class Q_SVG_EXPORT QSvgAnimateColor : public QSvgAnimateNode
{
public:
    QSvgAnimateColor(QSvgNode *parent = nullptr) : QSvgAnimateNode(parent) {}
    virtual Type type() const override { return QSvgNode::AnimateColor; }
};

class Q_SVG_EXPORT QSvgAnimateTransform : public QSvgAnimateNode
{
public:
    QSvgAnimateTransform(QSvgNode *parent = nullptr) : QSvgAnimateNode(parent) {}
    virtual Type type() const override { return Type::AnimateTransform; }
};

QT_END_NAMESPACE

#endif // QSVGANIMATE_P_H
