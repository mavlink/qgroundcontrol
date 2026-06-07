// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGNODEUPDATER_P_H
#define QSGNODEUPDATER_P_H

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

#include <private/qtquickglobal_p.h>
#include <QtGui/private/qdatabuffer_p.h>

QT_BEGIN_NAMESPACE

class QSGNode;
class QSGTransformNode;
class QSGClipNode;
class QSGOpacityNode;
class QSGGeometryNode;
class QMatrix4x4;
class QSGRenderNode;

class Q_QUICK_EXPORT QSGNodeUpdater
{
public:
    QSGNodeUpdater();
    virtual ~QSGNodeUpdater();

    virtual void updateStates(QSGNode *n);
    virtual bool isNodeBlocked(QSGNode *n, QSGNode *root) const;

protected:
    virtual void enterTransformNode(QSGTransformNode *);
    virtual void leaveTransformNode(QSGTransformNode *);
    void enterClipNode(QSGClipNode *c);
    void leaveClipNode(QSGClipNode *c);
    void enterOpacityNode(QSGOpacityNode *o);
    void leaveOpacityNode(QSGOpacityNode *o);
    void enterGeometryNode(QSGGeometryNode *);
    void leaveGeometryNode(QSGGeometryNode *);
    void enterRenderNode(QSGRenderNode *);
    void leaveRenderNode(QSGRenderNode *);

    void visitNode(QSGNode *n);
    void visitChildren(QSGNode *n);


    QDataBuffer<const QMatrix4x4 *> m_combined_matrix_stack;
    QDataBuffer<qreal> m_opacity_stack;
    const QSGClipNode *m_current_clip;

    int m_force_update;
};

QT_END_NAMESPACE

#endif
