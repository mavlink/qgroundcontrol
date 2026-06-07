// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGABSTRACTRENDERER_P_P_H
#define QSGABSTRACTRENDERER_P_P_H

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

#include "qsgabstractrenderer_p.h"

#include "qsgnode.h"
#include <qcolor.h>

#include <QtCore/private/qobject_p.h>
#include <QtQuick/private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGAbstractRendererPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSGAbstractRenderer)
public:
    static const QSGAbstractRendererPrivate *get(const QSGAbstractRenderer *q) { return q->d_func(); }

    QSGAbstractRendererPrivate();
    void updateProjectionMatrix();

    QSGRootNode *m_root_node;
    QColor m_clear_color;

    QRect m_device_rect;
    QRect m_viewport_rect;

    QVarLengthArray<QMatrix4x4, 1> m_projection_matrix;
    QVarLengthArray<QMatrix4x4, 1> m_projection_matrix_native_ndc;
    uint m_invertFrontFace : 1;
};

QT_END_NAMESPACE

#endif
