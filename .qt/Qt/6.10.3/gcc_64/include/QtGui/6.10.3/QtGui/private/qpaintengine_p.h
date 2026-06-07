// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAINTENGINE_P_H
#define QPAINTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qpainter.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qregion.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

class QPaintDevice;

class Q_GUI_EXPORT QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPaintEngine)
public:
    QPaintEnginePrivate() : pdev(nullptr), q_ptr(nullptr), currentClipDevice(nullptr), hasSystemTransform(0),
                            hasSystemViewport(0) {}
    virtual ~QPaintEnginePrivate();

    QPaintDevice *pdev;
    QPaintEngine *q_ptr;
    QRegion baseSystemClip;
    QRegion systemClip;
    QRect systemRect;
    QRegion systemViewport;
    QTransform systemTransform;
    QPaintDevice *currentClipDevice;
    uint hasSystemTransform : 1;
    uint hasSystemViewport : 1;

    inline void updateSystemClip()
    {
        systemClip = baseSystemClip;
        if (systemClip.isEmpty())
            return;

        if (hasSystemTransform) {
            if (systemTransform.type() <= QTransform::TxTranslate)
                systemClip.translate(qRound(systemTransform.dx()), qRound(systemTransform.dy()));
            else
                systemClip = systemTransform.map(systemClip);
        }

        // Make sure we're inside the viewport.
        if (hasSystemViewport) {
            systemClip &= systemViewport;
            if (systemClip.isEmpty()) {
                // We don't want to paint without system clip, so set it to 1 pixel :)
                systemClip = QRect(systemViewport.boundingRect().topLeft(), QSize(1, 1));
            }
        }
    }

    inline void setSystemTransform(const QTransform &xform)
    {
        systemTransform = xform;
        hasSystemTransform = !xform.isIdentity();
        updateSystemClip();
        if (q_ptr->state)
            systemStateChanged();
    }

    inline void setSystemViewport(const QRegion &region)
    {
        systemViewport = region;
        hasSystemViewport = !systemViewport.isEmpty();
        updateSystemClip();
        if (q_ptr->state)
            systemStateChanged();
    }

    inline void setSystemTransformAndViewport(const QTransform &xform, const QRegion &region)
    {
        systemTransform = xform;
        hasSystemTransform = !xform.isIdentity();
        systemViewport = region;
        hasSystemViewport = !systemViewport.isEmpty();
        updateSystemClip();
        if (q_ptr->state)
            systemStateChanged();
    }

    virtual void systemStateChanged() { }

    void drawBoxTextItem(const QPointF &p, const QTextItemInt &ti);

    static QPaintEnginePrivate *get(QPaintEngine *paintEngine) { return paintEngine->d_func(); }

    virtual QPaintEngine *aggregateEngine() { return nullptr; }
    virtual Qt::HANDLE nativeHandle() { return nullptr; }
};

QT_END_NAMESPACE

#endif // QPAINTENGINE_P_H
