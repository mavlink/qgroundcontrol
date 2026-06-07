// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCOMPRESSEDTEXTURE_P_H
#define QSGCOMPRESSEDTEXTURE_P_H

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

#include <private/qtexturefiledata_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgtexture_p.h>
#include <rhi/qrhi.h>
#include <QQuickTextureFactory>
#include <QOpenGLFunctions>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_TEXTUREIO);

class Q_QUICK_EXPORT QSGCompressedTexture : public QSGTexture
{
    Q_OBJECT
public:
    QSGCompressedTexture(const QTextureFileData& texData);
    virtual ~QSGCompressedTexture();

    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;

    qint64 comparisonKey() const override;
    QRhiTexture *rhiTexture() const override;
    void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) override;

    QTextureFileData textureData() const;

    struct FormatInfo
    {
        QRhiTexture::Format rhiFormat;
        bool isSRGB;
    };
    static FormatInfo formatInfo(quint32 glTextureFormat);
    static bool formatIsOpaque(quint32 glTextureFormat);

protected:
    QTextureFileData m_textureData;
    QSize m_size;
    QRhiTexture *m_texture = nullptr;
    bool m_hasAlpha = false;
    bool m_uploaded = false;
};

namespace QSGOpenGLAtlasTexture {
    class Manager;
}

class Q_QUICK_EXPORT QSGCompressedTextureFactory : public QQuickTextureFactory
{
public:
    QSGCompressedTextureFactory(const QTextureFileData& texData);
    QSGTexture *createTexture(QQuickWindow *) const override;
    int textureByteCount() const override;
    QSize textureSize() const override;

    const QTextureFileData *textureData() const { return &m_textureData; }

protected:
    QTextureFileData m_textureData;
};

QT_END_NAMESPACE

#endif // QSGCOMPRESSEDTEXTURE_P_H
