// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGGENERATOR_H
#define QSVGGENERATOR_H

#include <QtGui/qpaintdevice.h>

#ifndef QT_NO_SVGGENERATOR

#include <QtCore/qnamespace.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qscopedpointer.h>
#include <QtSvg/qtsvgglobal.h>

QT_BEGIN_NAMESPACE


class QSvgGeneratorPrivate;

class Q_SVG_EXPORT QSvgGenerator : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QSvgGenerator)

    // Not a QObject; just a hack to get documentation from qdoc
    Q_PROPERTY(QSize size READ size WRITE setSize)
    Q_PROPERTY(QRectF viewBox READ viewBoxF WRITE setViewBox)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
    Q_PROPERTY(QIODevice* outputDevice READ outputDevice WRITE setOutputDevice)
    Q_PROPERTY(int resolution READ resolution WRITE setResolution)
public:
    enum class SvgVersion {
        SvgTiny12,
        Svg11,
    };

    QSvgGenerator();
    explicit QSvgGenerator(SvgVersion version);
    ~QSvgGenerator();

    QString title() const;
    void setTitle(const QString &title);

    QString description() const;
    void setDescription(const QString &description);

    QSize size() const;
    void setSize(const QSize &size);

    QRect viewBox() const;
    QRectF viewBoxF() const;
    void setViewBox(const QRect &viewBox);
    void setViewBox(const QRectF &viewBox);

    QString fileName() const;
    void setFileName(const QString &fileName);

    QIODevice *outputDevice() const;
    void setOutputDevice(QIODevice *outputDevice);

    void setResolution(int dpi);
    int resolution() const;

    SvgVersion svgVersion() const;
protected:
    QPaintEngine *paintEngine() const override;
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;

private:
    QScopedPointer<QSvgGeneratorPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QT_NO_SVGGENERATOR
#endif // QSVGGENERATOR_H
