// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPIXMAPCACHE_H
#define QQUICKPIXMAPCACHE_H

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

#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <QtQuick/qquickimageprovider.h>
#include <private/qquickpixmap_p.h>

QT_BEGIN_NAMESPACE

class QQuickPixmapData;

/*! \internal
    A composite key to identify a QQuickPixmapData instance in a QHash.
*/
struct Q_AUTOTEST_EXPORT QQuickPixmapKey
{
    const QUrl *url;
    const QRect *region;
    const QSize *size;
    int frame;
    QQuickImageProviderOptions options;
};

/*! \internal
    A singleton managing the storage and garbage collection of QQuickPixmapData
    instances. It's exported only for the sake of autotests, and should not be
    manipulated outside qquickpixmap.cpp and qquickpixmapcache.cpp.
*/
class Q_AUTOTEST_EXPORT QQuickPixmapCache : public QObject
{
    Q_OBJECT

public:
    static QQuickPixmapCache *instance();
    ~QQuickPixmapCache() override;

    void unreferencePixmap(QQuickPixmapData *);
    void referencePixmap(QQuickPixmapData *);

    void purgeCache();

protected:
    void timerEvent(QTimerEvent *) override;

private:
    QQuickPixmapCache() = default;
    Q_DISABLE_COPY(QQuickPixmapCache)

    void shrinkCache(int remove);
    int destroyCache();
    qsizetype referencedCost() const;

private:
    QHash<QQuickPixmapKey, QQuickPixmapData *> m_cache;
    mutable QMutex m_cacheMutex; // avoid simultaneous iteration and modification

    QQuickPixmapData *m_unreferencedPixmaps = nullptr;
    QQuickPixmapData *m_lastUnreferencedPixmap = nullptr;

    int m_unreferencedCost = 0;
    int m_timerId = -1;
    bool m_destroying = false;

    friend class QQuickPixmap;
    friend class QQuickPixmapData;
    friend class tst_qquickpixmapcache;
    friend class tst_qquicktext;
    friend class tst_qquicktextedit;
};

QT_END_NAMESPACE

#endif // QQUICKPIXMAPCACHE_H
