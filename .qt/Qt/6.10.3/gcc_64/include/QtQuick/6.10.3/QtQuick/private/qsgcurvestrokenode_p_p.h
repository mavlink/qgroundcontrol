// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCURVESTROKENODE_P_P_H
#define QSGCURVESTROKENODE_P_P_H

#include <QtQuick/qtquickexports.h>
#include <QtQuick/qsgmaterial.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QSGCurveStrokeNode;
class QSGCurveStrokeMaterial;

class Q_QUICK_EXPORT QSGCurveStrokeMaterialShader : public QSGMaterialShader
{
public:
    QSGCurveStrokeMaterialShader(bool useDerivatives, int viewCount)
    {
        QString baseName(u":/qt-project.org/scenegraph/shaders_ng/shapestroke");
        if (useDerivatives)
            baseName += u"_derivatives";
        setShaderFileName(VertexStage, baseName + u".vert.qsb", viewCount);
        setShaderFileName(FragmentStage, baseName + u".frag.qsb", viewCount);
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
};


class Q_QUICK_EXPORT QSGCurveStrokeMaterial : public QSGMaterial
{
public:
    QSGCurveStrokeMaterial(QSGCurveStrokeNode *node)
        : m_node(node)
    {
        setFlag(Blending, true);
    }

    int compare(const QSGMaterial *other) const override;

    QSGCurveStrokeNode *node() const
    {
        return m_node;
    }

protected:
    QSGMaterialType *type() const override
    {
        static QSGMaterialType t;
        return &t;
    }
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override
    {
        return new QSGCurveStrokeMaterialShader(renderMode == QSGRendererInterface::RenderMode3D, viewCount());
    }

    QSGCurveStrokeNode *m_node;
};

QT_END_NAMESPACE

#endif // QSGCURVESTROKENODE_P_P_H
