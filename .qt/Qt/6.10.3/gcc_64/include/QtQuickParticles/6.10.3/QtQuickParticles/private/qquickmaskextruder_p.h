// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MASKEXTRUDER_H
#define MASKEXTRUDER_H

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
#include "qquickparticleextruder_p.h"
#include <private/qquickpixmap_p.h>
#include <QUrl>
#include <QImage>

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickMaskExtruder : public QQuickParticleExtruder
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    QML_NAMED_ELEMENT(MaskShape)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickMaskExtruder(QObject *parent = nullptr);
    QPointF extrude(const QRectF &) override;
    bool contains(const QRectF &bounds, const QPointF &point) override;

    QUrl source() const
    {
        return m_source;
    }

Q_SIGNALS:

    void sourceChanged(const QUrl &arg);

public Q_SLOTS:
    void setSource(const QUrl &arg);

private Q_SLOTS:
    void startMaskLoading();
    void finishMaskLoading();

private:
    QUrl m_source;

    void ensureInitialized(const QRectF &r);
    int m_lastWidth;
    int m_lastHeight;
    QQuickPixmap m_pix;
    QImage m_img;
    QList<QPointF> m_mask;//TODO: More memory efficient datastructures
    //Perhaps just the mask for the largest bounds is stored, and interpolate up
};

QT_END_NAMESPACE

#endif // MASKEXTRUDER_H
