// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKIMAGEPROVIDER_H
#define QQUICKIMAGEPROVIDER_H

#include <QtQuick/qtquickglobal.h>
#include <QtGui/qimage.h>
#include <QtGui/qpixmap.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE


class QQuickImageProviderPrivate;
class QQuickAsyncImageProviderPrivate;
class QQuickImageProviderOptionsPrivate;
class QSGTexture;
class QQuickWindow;

class Q_QUICK_EXPORT QQuickTextureFactory : public QObject
{
    Q_OBJECT
public:
    QQuickTextureFactory();
    ~QQuickTextureFactory() override;

    virtual QSGTexture *createTexture(QQuickWindow *window) const = 0;
    virtual QSize textureSize() const = 0;
    virtual int textureByteCount() const = 0;
    virtual QImage image() const;

    static QQuickTextureFactory *textureFactoryForImage(const QImage &image);
};

class QQuickImageResponsePrivate;

class Q_QUICK_EXPORT QQuickImageResponse : public QObject
{
Q_OBJECT
public:
    QQuickImageResponse();
    ~QQuickImageResponse() override;

    virtual QQuickTextureFactory *textureFactory() const = 0;
    virtual QString errorString() const;

public Q_SLOTS:
    virtual void cancel();

Q_SIGNALS:
    void finished();

private:
    Q_DECLARE_PRIVATE(QQuickImageResponse)
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
};

class QQuickImageProviderOptions;

class Q_QUICK_EXPORT QQuickImageProvider : public QQmlImageProviderBase
{
    friend class QQuickImageProviderWithOptions;
    Q_OBJECT
public:
    QQuickImageProvider(ImageType type, Flags flags = Flags());
    ~QQuickImageProvider() override;

    ImageType imageType() const override;
    Flags flags() const override;

    virtual QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize);
    virtual QPixmap requestPixmap(const QString &id, QSize *size, const QSize& requestedSize);
    virtual QQuickTextureFactory *requestTexture(const QString &id, QSize *size, const QSize &requestedSize);

private:
    QQuickImageProviderPrivate *d;
};

class Q_QUICK_EXPORT QQuickAsyncImageProvider : public QQuickImageProvider
{
public:
    QQuickAsyncImageProvider();
    ~QQuickAsyncImageProvider() override;

    virtual QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) = 0;

private:
    QQuickAsyncImageProviderPrivate *d;
};

QT_END_NAMESPACE

#endif // QQUICKIMAGEPROVIDER_H
