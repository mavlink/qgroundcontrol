// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGMATERIAL_H
#define QSGMATERIAL_H

#include <QtQuick/qtquickglobal.h>
#include <QtQuick/qsgmaterialshader.h>
#include <QtQuick/qsgmaterialtype.h>
#include <QtQuick/qsgrendererinterface.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGMaterial
{
public:
    enum Flag {
        Blending            = 0x0001,
        RequiresDeterminant = 0x0002, // Allow precalculated translation and 2D rotation
        RequiresFullMatrixExceptTranslate = 0x0004 | RequiresDeterminant, // Allow precalculated translation
        RequiresFullMatrix  = 0x0008 | RequiresFullMatrixExceptTranslate,
        NoBatching          = 0x0010,

        MultiView2          = 0x10000,
        MultiView3          = 0x20000,
        MultiView4          = 0x40000,

#if QT_DEPRECATED_SINCE(6, 3)
        CustomCompileStep Q_DECL_ENUMERATOR_DEPRECATED_X(
            "Qt 6 does not have custom shader compilation support. If the intention is to just disable batching, use NoBatching instead."
        ) = NoBatching
#endif

    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QSGMaterial();
    virtual ~QSGMaterial();

    virtual QSGMaterialType *type() const = 0;
    virtual QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const = 0;
    virtual int compare(const QSGMaterial *other) const;

    QSGMaterial::Flags flags() const { return m_flags; }
    void setFlag(Flags flags, bool on = true);

    int viewCount() const;

private:
    Flags m_flags;
    void *m_reserved;
    Q_DISABLE_COPY(QSGMaterial)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGMaterial::Flags)

QT_END_NAMESPACE

#endif
