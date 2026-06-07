// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKIMAGE_P_P_H
#define QQUICKIMAGE_P_P_H

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

#include "qquickimagebase_p_p.h"
#include "qquickimage_p.h"
#include <QtQuick/qsgtextureprovider.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickImageTextureProvider : public QSGTextureProvider
{
    Q_OBJECT
public:
    QQuickImageTextureProvider();

    void updateTexture(QSGTexture *texture);

    QSGTexture *texture() const override ;

    friend class QQuickImage;

    QSGTexture *m_texture;
    bool m_smooth;
    bool m_mipmap;
};

class Q_QUICK_EXPORT QQuickImagePrivate : public QQuickImageBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickImage)

public:
    QQuickImagePrivate();
    void setImage(const QImage &img);
    void setPixmap(const QQuickPixmap &pixmap);

    bool pixmapChanged : 1;
    bool mipmap : 1;
    QQuickImage::HAlignment hAlign = QQuickImage::AlignHCenter;
    QQuickImage::VAlignment vAlign = QQuickImage::AlignVCenter;
    QQuickImage::FillMode fillMode = QQuickImage::Stretch;

    qreal paintedWidth = 0;
    qreal paintedHeight = 0;
    QQuickImageTextureProvider *provider = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKIMAGE_P_P_H
