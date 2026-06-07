// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QICON_P_H
#define QICON_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qsize.h>
#include <QtCore/qlist.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qicon.h>
#include <QtGui/qiconengine.h>

#ifndef QT_NO_ICON
QT_BEGIN_NAMESPACE

class QIconPrivate
{
public:
    explicit QIconPrivate(QIconEngine *e);

    ~QIconPrivate() {
        delete engine;
    }

    static QIconPrivate *get(QIcon *icon) { return icon->d; }
    static const QIconPrivate *get(const QIcon *icon) { return icon->d; }

    enum IconEngineHook { PlatformIconHook = 1000 };

    static qreal pixmapDevicePixelRatio(qreal displayDevicePixelRatio, const QSize &requestedSize, const QSize &actualSize);

    QIconEngine *engine;

    QAtomicInt ref;
    int serialNum;
    int detach_no;
    bool is_mask;

    static void clearIconCache();
};


struct QPixmapIconEngineEntry
{
    QPixmapIconEngineEntry() = default;
    QPixmapIconEngineEntry(const QPixmap &pm, QIcon::Mode m, QIcon::State s)
        : pixmap(pm), size(pm.size()), mode(m), state(s) {}
    QPixmapIconEngineEntry(const QString &file, const QSize &sz, QIcon::Mode m, QIcon::State s)
        : fileName(file), size(sz), mode(m), state(s) {}
    QPixmapIconEngineEntry(const QString &file, const QImage &image, QIcon::Mode m, QIcon::State s);
    QPixmap pixmap;
    QString fileName;
    QSize size;
    QIcon::Mode mode = QIcon::Normal;
    QIcon::State state = QIcon::Off;
};
Q_DECLARE_TYPEINFO(QPixmapIconEngineEntry, Q_RELOCATABLE_TYPE);

inline QPixmapIconEngineEntry::QPixmapIconEngineEntry(const QString &file, const QImage &image, QIcon::Mode m, QIcon::State s)
    : fileName(file), size(image.size()), mode(m), state(s)
{
    pixmap.convertFromImage(image);
}

class Q_GUI_EXPORT QPixmapIconEngine : public QIconEngine {
public:
    QPixmapIconEngine();
    QPixmapIconEngine(const QPixmapIconEngine &);
    ~QPixmapIconEngine();
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    QPixmapIconEngineEntry *bestMatch(const QSize &size, qreal scale, QIcon::Mode mode, QIcon::State state);
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) override;
    void addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state) override;
    void addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    bool isNull() override;

    QString key() const override;
    QIconEngine *clone() const override;
    bool read(QDataStream &in) override;
    bool write(QDataStream &out) const override;

    static inline QSize adjustSize(const QSize &expectedSize, QSize size)
    {
        if (!size.isNull() && (size.width() > expectedSize.width() || size.height() > expectedSize.height()))
            size.scale(expectedSize, Qt::KeepAspectRatio);
        return size;
    }

private:
    void removePixmapEntry(QPixmapIconEngineEntry *pe)
    {
        auto idx = pixmaps.size();
        while (--idx >= 0) {
            if (pe == &pixmaps.at(idx)) {
                pixmaps.remove(idx);
                return;
            }
        }
    }
    QPixmapIconEngineEntry *tryMatch(const QSize &size, qreal scale, QIcon::Mode mode, QIcon::State state);
    QList<QPixmapIconEngineEntry> pixmaps;

    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &s, const QIcon &icon);
    friend class QIconThemeEngine;
};

QT_END_NAMESPACE
#endif //QT_NO_ICON
#endif // QICON_P_H
