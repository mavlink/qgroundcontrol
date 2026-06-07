// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWAREPIXMAPTEXTURE_H
#define QSGSOFTWAREPIXMAPTEXTURE_H

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

#include <private/qsgtexture_p.h>
#include <QtGui/QPixmap>

QT_BEGIN_NAMESPACE

class QSGSoftwarePixmapTexture : public QSGTexture
{
    Q_OBJECT

public:
    QSGSoftwarePixmapTexture(const QImage &image, uint flags);
    QSGSoftwarePixmapTexture(const QPixmap &pixmap);

    qint64 comparisonKey() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;

    const QPixmap &pixmap() const { return m_pixmap; }

private:
    QPixmap m_pixmap;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREPIXMAPTEXTURE_H
