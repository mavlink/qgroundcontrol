// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSTYLE_P_H
#define QSTYLE_P_H

#include "private/qobject_p.h"
#include <QtWidgets/qstyle.h>

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qstyle_*.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

// Private class

class QStyle;

class QStylePrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QStyle)
public:
    static QStylePrivate *get(QStyle *s) { return s ? s->d_func() : nullptr; }
    static const QStylePrivate *get(const QStyle *s) { return s ? s->d_func() : nullptr; }

    static bool useFullScreenForPopup();

    QStyle *proxyStyle;
    QString name;
};

inline QPixmap styleCachePixmap(const QSize &size, qreal pixelRatio)
{
    QPixmap cachePixmap = QPixmap(size * pixelRatio);
    cachePixmap.setDevicePixelRatio(pixelRatio);
    cachePixmap.fill(Qt::transparent);
    return cachePixmap;
}

// small helper to read out the pixmap to paint from QPixmapCache or re-draw
// it and put it into the QPixmapCache for later usage
class Q_WIDGETS_EXPORT QCachedPainter
{
public:
    QCachedPainter(QPainter *painter, const QString &cachePrefix,
                   const QStyleOption *option, QSize size = {}, QRect paintRect = {});
    ~QCachedPainter();
    void finish();
    bool needsPainting() const
    {
        return !m_alreadyCached;
    }
    QRect pixmapRect() const
    {
        const auto sz = m_pixmap.deviceIndependentSize();
        return QRect(0, 0, sz.width(), sz.height());
    }
    QPainter *operator->()
    {
        return painter();
    }
    QPainter *painter()
    {
        Q_ASSERT_X(m_pixmapPainter, "painter()", "Must only be called when painting on a pixmap to cache");
        return m_pixmapPainter.get();
    }

    // clean pixmap cache from all cached pixmaps (e.g. due to palette change)
    // to make sure the widgets are painted correctly afterwards
    static void cleanupPixmapCache();
private:
    QPainter *m_painter = nullptr;
    const QStyleOption *m_option = nullptr;
    std::unique_ptr<QPainter> m_pixmapPainter;
    QString m_pixmapName;
    QPixmap m_pixmap;
    QRect m_paintRect;
    bool m_alreadyCached;
    bool m_pixmapDrawn = false;
    static QSet<QString> s_pixmapCacheKeys;
};

QT_END_NAMESPACE

#endif //QSTYLE_P_H
