// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef KILLAFFECTOR_H
#define KILLAFFECTOR_H

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
#include "qquickparticleaffector_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickAgeAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(int lifeLeft READ lifeLeft WRITE setLifeLeft NOTIFY lifeLeftChanged)
    Q_PROPERTY(bool advancePosition READ advancePosition WRITE setAdvancePosition NOTIFY advancePositionChanged)
    QML_NAMED_ELEMENT(Age)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickAgeAffector(QQuickItem *parent = nullptr);

    int lifeLeft() const
    {
        return m_lifeLeft;
    }

    bool advancePosition() const
    {
        return m_advancePosition;
    }

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:
    void lifeLeftChanged(int arg);
    void advancePositionChanged(bool arg);

public Q_SLOTS:
    void setLifeLeft(int arg)
    {
        if (m_lifeLeft != arg) {
            m_lifeLeft = arg;
            Q_EMIT lifeLeftChanged(arg);
        }
    }

    void setAdvancePosition(bool arg)
    {
        if (m_advancePosition != arg) {
            m_advancePosition = arg;
            Q_EMIT advancePositionChanged(arg);
        }
    }

private:
    int m_lifeLeft;
    bool m_advancePosition;
};

QT_END_NAMESPACE
#endif // KILLAFFECTOR_H
