// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATEDSPRITE_P_P_H
#define QQUICKANIMATEDSPRITE_P_P_H

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

#include <QtQuick/qtquickglobal.h>

QT_REQUIRE_CONFIG(quick_sprite);

#include "qquickitem_p.h"
#include "qquicksprite_p.h"
#include "qquickanimatedsprite_p.h"

QT_BEGIN_NAMESPACE

class QQuickAnimatedSpritePrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickAnimatedSprite)

public:
    QQuickAnimatedSpritePrivate()
        : m_sprite(nullptr)
        , m_spriteEngine(nullptr)
        , m_curFrame(0)
        , m_pleaseReset(false)
        , m_running(true)
        , m_paused(false)
        , m_interpolate(true)
        , m_loops(-1)
        , m_curLoop(0)
        , m_pauseOffset(0)
        , m_finishBehavior(QQuickAnimatedSprite::FinishAtInitialFrame)
    {
    }

    QQuickSprite* m_sprite;
    QQuickSpriteEngine* m_spriteEngine;
    QElapsedTimer m_timestamp;
    int m_curFrame;
    bool m_pleaseReset;
    bool m_running;
    bool m_paused;
    bool m_interpolate;
    QSize m_sheetSize;
    int m_loops;
    int m_curLoop;
    int m_pauseOffset;
    QQuickAnimatedSprite::FinishBehavior m_finishBehavior;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATEDSPRITE_P_P_H
