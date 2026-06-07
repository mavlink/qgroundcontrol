// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGFLATCOLORMATERIAL_H
#define QSGFLATCOLORMATERIAL_H

#include <QtQuick/qsgmaterial.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGFlatColorMaterial : public QSGMaterial
{
public:
    QSGFlatColorMaterial();
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    void setColor(const QColor &color);
    const QColor &color() const { return m_color; }

    int compare(const QSGMaterial *other) const override;

private:
    QColor m_color;
};

QT_END_NAMESPACE

#endif // FLATCOLORMATERIAL_H
