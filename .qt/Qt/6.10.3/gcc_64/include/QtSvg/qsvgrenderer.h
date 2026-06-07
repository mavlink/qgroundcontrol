// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGRENDERER_H
#define QSVGRENDERER_H

#ifndef QT_NO_SVGRENDERER

#include <QtCore/qobject.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>
#include <QtCore/qxmlstream.h>
#include <QtSvg/qtsvgglobal.h>

QT_BEGIN_NAMESPACE


class QSvgRendererPrivate;
class QPainter;
class QByteArray;
class QTransform;

class Q_SVG_EXPORT QSvgRenderer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QRectF viewBox READ viewBoxF WRITE setViewBox)
    Q_PROPERTY(int framesPerSecond READ framesPerSecond WRITE setFramesPerSecond)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame)
    Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode)
    Q_PROPERTY(QtSvg::Options options READ options WRITE setOptions)
    Q_PROPERTY(bool animationEnabled READ isAnimationEnabled WRITE setAnimationEnabled)
public:
    QSvgRenderer(QObject *parent = nullptr);
    QSvgRenderer(const QString &filename, QObject *parent = nullptr);
    QSvgRenderer(const QByteArray &contents, QObject *parent = nullptr);
    QSvgRenderer(QXmlStreamReader *contents, QObject *parent = nullptr);
    ~QSvgRenderer();

    bool isValid() const;

    QSize defaultSize() const;

    QRect viewBox() const;
    QRectF viewBoxF() const;
    void setViewBox(const QRect &viewbox);
    void setViewBox(const QRectF &viewbox);

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    QtSvg::Options options() const;
    void setOptions(QtSvg::Options flags);

    bool animated() const;
    int framesPerSecond() const;
    void setFramesPerSecond(int num);
    int currentFrame() const;
    void setCurrentFrame(int);
    int animationDuration() const;//in seconds
    bool isAnimationEnabled() const;
    void setAnimationEnabled(bool enable);

    QRectF boundsOnElement(const QString &id) const;
    bool elementExists(const QString &id) const;
    QTransform transformForElement(const QString &id) const;

    static void setDefaultOptions(QtSvg::Options flags);

public Q_SLOTS:
    bool load(const QString &filename);
    bool load(const QByteArray &contents);
    bool load(QXmlStreamReader *contents);
    void render(QPainter *p);
    void render(QPainter *p, const QRectF &bounds);

    void render(QPainter *p, const QString &elementId,
                const QRectF &bounds=QRectF());

Q_SIGNALS:
    void repaintNeeded();

private:
    Q_DECLARE_PRIVATE(QSvgRenderer)
};

QT_END_NAMESPACE

#endif // QT_NO_SVGRENDERER
#endif // QSVGRENDERER_H
