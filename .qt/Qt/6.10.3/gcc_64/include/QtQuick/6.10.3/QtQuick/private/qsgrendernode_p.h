// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRENDERNODE_P_H
#define QSGRENDERNODE_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/qsgrendernode.h>
#include <QtQuick/private/qsgrenderer_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGRenderNodePrivate
{
public:
    QSGRenderNodePrivate();

    static QSGRenderNodePrivate *get(QSGRenderNode *node) { return node->d; }

    const QMatrix4x4 *m_matrix;
    const QSGClipNode *m_clip_list;
    qreal m_opacity;
    QSGRenderTarget m_rt;
    QVarLengthArray<QMatrix4x4, 1> m_projectionMatrix;
    QMatrix4x4 m_localMatrix; //  ### Qt 7 m_matrix should not be a pointer
};

QT_END_NAMESPACE

#endif
