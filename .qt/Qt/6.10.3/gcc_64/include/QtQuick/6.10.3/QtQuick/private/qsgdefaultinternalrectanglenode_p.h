// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QSGDEFAULTINTERNALRECTANGLENODE_P_H
#define QSGDEFAULTINTERNALRECTANGLENODE_P_H

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

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgbasicinternalrectanglenode_p.h>
#include <QtQuick/qsgvertexcolormaterial.h>

QT_BEGIN_NAMESPACE

class QSGContext;

class Q_QUICK_EXPORT QSGSmoothColorMaterial : public QSGMaterial
{
public:
    QSGSmoothColorMaterial();

    int compare(const QSGMaterial *other) const override;

protected:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
};

class Q_QUICK_EXPORT QSGDefaultInternalRectangleNode : public QSGBasicInternalRectangleNode
{
public:
    QSGDefaultInternalRectangleNode();

private:
    void updateMaterialAntialiasing() override;
    void updateMaterialBlending(QSGNode::DirtyState *state) override;

    QSGVertexColorMaterial m_material;
    QSGSmoothColorMaterial m_smoothMaterial;
};

QT_END_NAMESPACE

#endif
