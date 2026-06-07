// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWIDGETREPAINTMANAGER_P_H
#define QWIDGETREPAINTMANAGER_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qwidget.h>
#include <private/qwidget_p.h>
#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

class QPlatformTextureList;
class QPlatformTextureListWatcher;
class QWidgetRepaintManager;

class Q_WIDGETS_EXPORT QWidgetRepaintManager
{
    Q_GADGET
public:
    enum UpdateTime {
        UpdateNow,
        UpdateLater
    };
    Q_ENUM(UpdateTime)

    enum BufferState{
        BufferValid,
        BufferInvalid
    };
    Q_ENUM(BufferState)

    QWidgetRepaintManager(QWidget *t);
    ~QWidgetRepaintManager();

    QBackingStore *backingStore() const { return store; }
    void setBackingStore(QBackingStore *backingStore) { store = backingStore; }

    template <class T>
    void markDirty(const T &r, QWidget *widget, UpdateTime updateTime = UpdateLater,
                   BufferState bufferState = BufferValid);

    void removeDirtyWidget(QWidget *w);

    void sync(QWidget *exposedWidget, const QRegion &exposedRegion);
    void sync();

    void markNeedsFlush(QWidget *widget, const QRegion &region, const QPoint &topLevelOffset);

    void addStaticWidget(QWidget *widget);
    void moveStaticWidgets(QWidget *reparented);
    void removeStaticWidget(QWidget *widget);
    QRegion staticContents(QWidget *widget = nullptr, const QRect &withinClipRect = QRect()) const;
    QRegion dirtyRegion() const { return dirty; }
    QList<QWidget *> dirtyWidgetList() const { return dirtyWidgets; }
    bool isDirty() const;

    bool bltRect(const QRect &rect, int dx, int dy, QWidget *widget);

private:
    void updateLists(QWidget *widget);

    void addDirtyWidget(QWidget *widget, const QRegion &rgn);
    void resetWidget(QWidget *widget);

    void addDirtyRenderToTextureWidget(QWidget *widget);

    void sendUpdateRequest(QWidget *widget, UpdateTime updateTime);

    bool syncAllowed();
    void paintAndFlush();

    void markNeedsFlush(QWidget *widget, const QRegion &region = QRegion());

    void flush();
    void flush(QWidget *widget, const QRegion &region, QPlatformTextureList *widgetTextures);

    bool hasStaticContents() const;
    void updateStaticContentsSize();

    QWidget *tlw = nullptr;
    QBackingStore *store = nullptr;

    QRegion dirty; // needsRepaint
    QList<QWidget *> dirtyWidgets;
    QList<QWidget *> dirtyRenderToTextureWidgets;

    QRegion topLevelNeedsFlush;
    QList<QWidget *> needsFlushWidgets;

    QList<QWidget *> staticWidgets;

    QPlatformTextureListWatcher *textureListWatcher = nullptr;

    bool updateRequestSent = false;

    QElapsedTimer perfTime;
    int perfFrames = 0;

    Q_DISABLE_COPY_MOVE(QWidgetRepaintManager)
};

QT_END_NAMESPACE

#endif // QWIDGETREPAINTMANAGER_P_H
