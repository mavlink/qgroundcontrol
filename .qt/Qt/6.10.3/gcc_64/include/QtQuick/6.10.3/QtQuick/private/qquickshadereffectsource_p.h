// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSHADEREFFECTSOURCE_P_H
#define QQUICKSHADEREFFECTSOURCE_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_shadereffect);

#include "qquickitem.h"
#include <QtQuick/qsgtextureprovider.h>
#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgdefaultinternalimagenode_p.h>
#include <private/qquickitemchangelistener_p.h>

#include "qpointer.h"
#include "qsize.h"
#include "qrect.h"

QT_BEGIN_NAMESPACE

class QSGNode;
class UpdatePaintNodeData;
class QOpenGLFramebufferObject;
class QSGSimpleRectNode;

class QQuickShaderEffectSourceTextureProvider;

class Q_QUICK_EXPORT QQuickShaderEffectSource : public QQuickItem,
                                                public QSafeQuickItemChangeListener<QQuickShaderEffectSource>
{
    Q_OBJECT
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    Q_PROPERTY(QQuickItem *sourceItem READ sourceItem WRITE setSourceItem NOTIFY sourceItemChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect WRITE setSourceRect NOTIFY sourceRectChanged)
    Q_PROPERTY(QSize textureSize READ textureSize WRITE setTextureSize NOTIFY textureSizeChanged)
    Q_PROPERTY(Format format READ format WRITE setFormat NOTIFY formatChanged)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged)
    Q_PROPERTY(bool hideSource READ hideSource WRITE setHideSource NOTIFY hideSourceChanged)
    Q_PROPERTY(bool mipmap READ mipmap WRITE setMipmap NOTIFY mipmapChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(TextureMirroring textureMirroring READ textureMirroring WRITE setTextureMirroring NOTIFY textureMirroringChanged REVISION(2, 6))
    Q_PROPERTY(int samples READ samples WRITE setSamples NOTIFY samplesChanged REVISION(2, 9))
    QML_NAMED_ELEMENT(ShaderEffectSource)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum WrapMode {
        ClampToEdge,
        RepeatHorizontally,
        RepeatVertically,
        Repeat
    };
    Q_ENUM(WrapMode)

    enum Format {
        RGBA8 = 1,
        RGBA16F,
        RGBA32F,

        // Qt 5 legacy values that were ignored starting from Qt 6.0
        Alpha = RGBA8,
        RGB = RGBA8,
        RGBA = RGBA8
    };
    Q_ENUM(Format)

    enum TextureMirroring {
        NoMirroring        = 0x00,
        MirrorHorizontally = 0x01,
        MirrorVertically   = 0x02
    };
    Q_ENUM(TextureMirroring)

    QQuickShaderEffectSource(QQuickItem *parent = nullptr);
    ~QQuickShaderEffectSource() override;

    WrapMode wrapMode() const;
    void setWrapMode(WrapMode mode);

    QQuickItem *sourceItem() const;
    void setSourceItem(QQuickItem *item);

    QRectF sourceRect() const;
    void setSourceRect(const QRectF &rect);

    QSize textureSize() const;
    void setTextureSize(const QSize &size);

    Format format() const;
    void setFormat(Format format);

    bool live() const;
    void setLive(bool live);

    bool hideSource() const;
    void setHideSource(bool hide);

    bool mipmap() const;
    void setMipmap(bool enabled);

    bool recursive() const;
    void setRecursive(bool enabled);

    TextureMirroring textureMirroring() const;
    void setTextureMirroring(TextureMirroring mirroring);

    bool isTextureProvider() const override { return true; }
    QSGTextureProvider *textureProvider() const override;

    Q_INVOKABLE void scheduleUpdate();

    int samples() const;
    void setSamples(int count);

Q_SIGNALS:
    void wrapModeChanged();
    void sourceItemChanged();
    void sourceRectChanged();
    void textureSizeChanged();
    void formatChanged();
    void liveChanged();
    void hideSourceChanged();
    void mipmapChanged();
    void recursiveChanged();
    void textureMirroringChanged();
    void samplesChanged();

    void scheduledUpdateCompleted();

private Q_SLOTS:
    void sourceItemDestroyed(QObject *item);
    void invalidateSceneGraph();

protected:
    void releaseResources() override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    void ensureTexture();

    QQuickShaderEffectSourceTextureProvider *m_provider;
    QSGLayer *m_texture;
    WrapMode m_wrapMode;
    QQuickItem *m_sourceItem;
    QRectF m_sourceRect;
    QSize m_textureSize;
    Format m_format;
    int m_samples;
    uint m_live : 1;
    uint m_hideSource : 1;
    uint m_mipmap : 1;
    uint m_recursive : 1;
    uint m_grab : 1;
    uint m_textureMirroring : 2; // Stores TextureMirroring enum
};

QT_END_NAMESPACE

#endif // QQUICKSHADEREFFECTSOURCE_P_H
