// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWAREPIXMAPRENDERER_H
#define QSGSOFTWAREPIXMAPRENDERER_H

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

#include "qsgabstractsoftwarerenderer_p.h"

QT_BEGIN_NAMESPACE

class QSGSoftwarePixmapRenderer : public QSGAbstractSoftwareRenderer
{
public:
    QSGSoftwarePixmapRenderer(QSGRenderContext *context);
    virtual ~QSGSoftwarePixmapRenderer();

    void renderScene() final;
    void render() final;

    void render(QPaintDevice *target);
    void setProjectionRect(const QRect &projectionRect);

private:
    QRect m_projectionRect;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREPIXMAPRENDERER_H
