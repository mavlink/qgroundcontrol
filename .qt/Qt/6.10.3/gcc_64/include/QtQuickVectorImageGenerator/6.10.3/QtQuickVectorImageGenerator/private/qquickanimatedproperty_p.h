// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKANIMATEDPROPERTY_P_H
#define QQUICKANIMATEDPROPERTY_P_H

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

#include <QMap>
#include <QVariant>
#include <QtGui/private/qbezier_p.h>

QT_BEGIN_NAMESPACE

class QQuickAnimatedProperty
{
public:
    struct PropertyAnimation {
        enum Flag {
            NoFlags = 0,
            FreezeAtEnd = 1,
            ReplacePreviousAnimations = 2,
        };

        QMap<int, QVariant> frames;
        QMap<int, QBezier> easingPerFrame;
        int subtype = 0;
        int repeatCount = 1;
        int startOffset = 0;
        quint8 flags = NoFlags;

        bool isConstant() const {
            for (const auto& frame : frames) {
                if (frame != frames.first())
                    return false;
            }
            return true;
        }
    };

    QQuickAnimatedProperty(const QVariant &defaultValue)
        : m_defaultValue(defaultValue)
    {
    }

    bool isAnimated() const
    {
        return !m_animations.isEmpty();
    }

    QVariant defaultValue() const
    {
        return m_defaultValue;
    }

    void setDefaultValue(const QVariant &value)
    {
        m_defaultValue = value;
    }

    qsizetype animationCount() const
    {
        return m_animations.size();
    }

    const PropertyAnimation &animation(int index) const
    {
        return m_animations.at(index);
    }

    void addAnimation(const PropertyAnimation &animation)
    {
        if (m_animationGroups.isEmpty())
            beginAnimationGroup();

        m_animations.append(animation);
    }

    qsizetype animationGroupCount() const
    {
        return m_animationGroups.size();
    }

    int animationGroup(int index) const
    {
        return m_animationGroups.at(index);
    }

    void beginAnimationGroup()
    {
        m_animationGroups.append(m_animations.size());
    }

private:
    QVariant m_defaultValue;
    QList<PropertyAnimation> m_animations;
    QList<int> m_animationGroups;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATEDPROPERTY_P_H
