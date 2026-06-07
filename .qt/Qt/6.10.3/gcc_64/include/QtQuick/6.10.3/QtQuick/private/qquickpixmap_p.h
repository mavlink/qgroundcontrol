// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPIXMAP_H
#define QQUICKPIXMAP_H

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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qstring.h>
#include <QtGui/qpixmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <private/qtquickglobal_p.h>
#include <QtQuick/qquickimageprovider.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQuickPixmapData;
class QQuickTextureFactory;
class QQuickImageProviderOptionsPrivate;

class QQuickDefaultTextureFactory : public QQuickTextureFactory
{
    Q_OBJECT
public:
    QQuickDefaultTextureFactory(const QImage &i);
    QSGTexture *createTexture(QQuickWindow *window) const override;
    QSize textureSize() const override { return size; }
    int textureByteCount() const override { return size.width() * size.height() * 4; }
    QImage image() const override { return im; }

private:
    QImage im;
    QSize size;
};

class QQuickImageProviderPrivate
{
public:
    QQuickImageProvider::ImageType type;
    QQuickImageProvider::Flags flags;
    bool isProviderWithOptions;
};

// ### Qt 6: Make public moving to qquickimageprovider.h
class Q_QUICK_EXPORT QQuickImageProviderOptions
{
public:
    enum AutoTransform {
        UsePluginDefaultTransform = -1,
        ApplyTransform = 0,
        DoNotApplyTransform = 1
    };

    QQuickImageProviderOptions();
    ~QQuickImageProviderOptions();

    QQuickImageProviderOptions(const QQuickImageProviderOptions&);
    QQuickImageProviderOptions& operator=(const QQuickImageProviderOptions&);

    bool operator==(const QQuickImageProviderOptions&) const;

    AutoTransform autoTransform() const;
    void setAutoTransform(AutoTransform autoTransform);

    bool preserveAspectRatioCrop() const;
    void setPreserveAspectRatioCrop(bool preserveAspectRatioCrop);

    bool preserveAspectRatioFit() const;
    void setPreserveAspectRatioFit(bool preserveAspectRatioFit);

    QColorSpace targetColorSpace() const;
    void setTargetColorSpace(const QColorSpace &colorSpace);

    QRectF sourceClipRect() const;
    void setSourceClipRect(const QRectF &rect);

private:
    QSharedDataPointer<QQuickImageProviderOptionsPrivate> d;
};

/*! \internal
    A class that encapsulates the action of fetching a pixmap, as well as the
    pixmap itself (indirectly via QQuickPixmapData::textureFactory) and the
    responsibility of canceling outstanding requests. Rather than relying on
    QPixmapCache which doesn't cache all the information Qt Quick needs,
    QQuickPixmap implements its own cache, that correctly degrades over time.
    (QQuickPixmapData::release() marks it as being not-currently-used, and
    QQuickPixmapCache::shrinkCache() sweeps away the least-recently-released
    instances until the remaining bytes are less than cache_limit.)
*/
class Q_QUICK_EXPORT QQuickPixmap
{
    Q_DECLARE_TR_FUNCTIONS(QQuickPixmap)
public:
    enum Status { Null, Ready, Error, Loading };

    enum Option {
        Asynchronous = 0x00000001,
        Cache        = 0x00000002
    };
    Q_DECLARE_FLAGS(Options, Option)

    QQuickPixmap();
    QQuickPixmap(QQmlEngine *, const QUrl &);
    QQuickPixmap(QQmlEngine *, const QUrl &, Options options);
    QQuickPixmap(QQmlEngine *, const QUrl &, const QRect &region, const QSize &);
    QQuickPixmap(const QUrl &, const QImage &image);
    ~QQuickPixmap();

    bool isNull() const;
    bool isReady() const;
    bool isError() const;
    bool isLoading() const;

    Status status() const;
    QString error() const;
    const QUrl &url() const;
    const QSize &implicitSize() const;
    const QRect &requestRegion() const;
    const QSize &requestSize() const;
    QQuickImageProviderOptions::AutoTransform autoTransform() const;
    int frameCount() const;
    QImage image() const;
    void setImage(const QImage &);
    void setPixmap(const QQuickPixmap &other);

    QColorSpace colorSpace() const;

    QQuickTextureFactory *textureFactory() const;

    QRect rect() const;
    int width() const;
    int height() const;

    void load(QQmlEngine *, const QUrl &);
    void load(QQmlEngine *, const QUrl &, QQuickPixmap::Options options);
    void load(QQmlEngine *, const QUrl &, const QRect &requestRegion, const QSize &requestSize);
    void load(QQmlEngine *, const QUrl &, const QRect &requestRegion, const QSize &requestSize, QQuickPixmap::Options options);
    void load(QQmlEngine *, const QUrl &, const QRect &requestRegion, const QSize &requestSize,
              QQuickPixmap::Options options, const QQuickImageProviderOptions &providerOptions, int frame = 0, int frameCount = 1,
              qreal devicePixelRatio = 1.0);
    void loadImageFromDevice(QQmlEngine *engine, QIODevice *device, const QUrl &url,
                             const QRect &requestRegion, const QSize &requestSize,
                             const QQuickImageProviderOptions &providerOptions, int frame = 0, int frameCount = 1);

    void clear();
    void clear(QObject *);

    bool connectFinished(QObject *, const char *);
    bool connectFinished(QObject *, int);
    bool connectDownloadProgress(QObject *, const char *);
    bool connectDownloadProgress(QObject *, int);

    static void purgeCache();
    static bool isCached(const QUrl &url, const QRect &requestRegion, const QSize &requestSize,
                         const int frame, const QQuickImageProviderOptions &options);
    static bool isScalableImageFormat(const QUrl &url);

    static const QLatin1String itemGrabberScheme;

private:
    Q_DISABLE_COPY(QQuickPixmap)
    QQuickPixmapData *d;
    friend class QQuickPixmapData;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickPixmap::Options)

// ### Qt 6: This should be made public in Qt 6. It's functionality can't be merged into
// QQuickImageProvider without breaking source compatibility.
class Q_QUICK_EXPORT QQuickImageProviderWithOptions : public QQuickAsyncImageProvider
{
public:
    QQuickImageProviderWithOptions(ImageType type, Flags flags = Flags());

    QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize) override;
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize& requestedSize) override;
    QQuickTextureFactory *requestTexture(const QString &id, QSize *size, const QSize &requestedSize) override;
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    virtual QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize, const QQuickImageProviderOptions &options);
    virtual QPixmap requestPixmap(const QString &id, QSize *size, const QSize& requestedSize, const QQuickImageProviderOptions &options);
    virtual QQuickTextureFactory *requestTexture(const QString &id, QSize *size, const QSize &requestedSize, const QQuickImageProviderOptions &options);
    virtual QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize, const QQuickImageProviderOptions &options);

    static QSize loadSize(const QSize &originalSize, const QSize &requestedSize, const QByteArray &format, const QQuickImageProviderOptions &options,
                          qreal devicePixelRatio = 1.0);
    static QQuickImageProviderWithOptions *checkedCast(QQuickImageProvider *provider);
};

QT_END_NAMESPACE

#endif // QQUICKPIXMAP_H
