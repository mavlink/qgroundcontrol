// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGVERTEXCOLORMATERIAL_H
#define QSGVERTEXCOLORMATERIAL_H

#include <QtQuick/qsgmaterial.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGVertexColorMaterial : public QSGMaterial
{
public:
    QSGVertexColorMaterial();

    int compare(const QSGMaterial *other) const override;

protected:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
};

QT_END_NAMESPACE

#endif // VERTEXCOLORMATERIAL_H
