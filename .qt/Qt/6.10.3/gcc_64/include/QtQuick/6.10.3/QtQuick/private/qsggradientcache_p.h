// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGGRADIENTCACHE_P_H
#define QSGGRADIENTCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qhash.h>
#include <QtCore/qcache.h>
#include <QtGui/qbrush.h>

#include <QtQuick/qtquickexports.h>

QT_BEGIN_NAMESPACE

class QSGTexture;
class QSGPlainTexture;
class QRhi;

struct Q_QUICK_EXPORT QSGGradientCacheKey
{
    QSGGradientCacheKey(const QGradientStops &stops, QGradient::Spread spread)
        : stops(stops), spread(spread)
    { }
    QGradientStops stops;
    QGradient::Spread spread;
    bool operator==(const QSGGradientCacheKey &other) const
    {
        return spread == other.spread && stops == other.stops;
    }
};

inline size_t qHash(const QSGGradientCacheKey &v, size_t seed = 0)
{
    size_t h = qHash(size_t(v.spread), seed);
    for (int i = 0; i < 2 && i < v.stops.size(); ++i)
        h = qHash(v.stops[i].first, qHash(v.stops[i].second.rgba64(), h));
    return h;
}

class Q_QUICK_EXPORT QSGGradientCache
{
public:
    struct GradientDesc { // can fully describe a linear/radial/conical gradient
        QGradientStops stops;
        QGradient::Spread spread = QGradient::PadSpread;
        QPointF a; // start (L) or center point (R/C)
        QPointF b; // end (L) or focal point (R)
        qreal v0; // center radius (R) or start angle (C)
        qreal v1; // focal radius (R)
    };

    QSGGradientCache();
    ~QSGGradientCache();
    static QSGGradientCache *cacheForRhi(QRhi *rhi);
    QSGTexture *get(const QSGGradientCacheKey &grad);

private:
    void setTextureData(QSGPlainTexture *tx, const QSGGradientCacheKey &grad);

    struct IndexHolder {
        ~IndexHolder() { if (freeIndex && idx >= 0) *freeIndex = idx; }
        qsizetype idx = -1;
        qsizetype *freeIndex = nullptr;
    };
    QList<QSGPlainTexture *> m_textures;
    QCache<QSGGradientCacheKey, IndexHolder> m_cache;
    qsizetype m_freeIndex = -1;
};

QT_END_NAMESPACE

#endif // QSGGRADIENTCACHE_P_H
