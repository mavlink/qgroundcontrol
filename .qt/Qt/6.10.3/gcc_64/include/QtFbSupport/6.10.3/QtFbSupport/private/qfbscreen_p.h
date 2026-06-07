// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFBSCREEN_P_H
#define QFBSCREEN_P_H

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

#include <qpa/qplatformscreen.h>
#include <QtCore/QSize>
#include "qfbcursor_p.h"

QT_BEGIN_NAMESPACE

class QFbWindow;
class QFbCursor;
class QPainter;
class QFbBackingStore;

class QFbScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT

public:
    enum Flag {
        DontForceFirstWindowToFullScreen = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QFbScreen();
    ~QFbScreen();

    virtual bool initialize();

    QRect geometry() const override { return mGeometry; }
    int depth() const override { return mDepth; }
    QImage::Format format() const override { return mFormat; }
    QSizeF physicalSize() const override { return mPhysicalSize; }
    QPlatformCursor *cursor() const override { return mCursor; }

    QWindow *topWindow() const;
    QWindow *topLevelAt(const QPoint & p) const override;

    // compositor api
    virtual void addWindow(QFbWindow *window);
    virtual void removeWindow(QFbWindow *window);
    virtual void raise(QFbWindow *window);
    virtual void lower(QFbWindow *window);
    virtual void topWindowChanged(QWindow *) {}
    virtual int windowCount() const;
    virtual Flags flags() const;

    void addPendingBackingStore(QFbBackingStore *bs) { mPendingBackingStores << bs; }

    void scheduleUpdate();

public slots:
    virtual void setDirty(const QRect &rect);
    void setPhysicalSize(const QSize &size);
    void setGeometry(const QRect &rect);

protected:
    virtual QRegion doRedraw();

    void initializeCompositor();
    bool event(QEvent *event) override;

    QFbWindow *windowForId(WId wid) const;

    QList<QFbWindow *> mWindowStack;
    QRegion mRepaintRegion;
    bool mUpdatePending;

    QFbCursor *mCursor;
    QRect mGeometry;
    int mDepth;
    QImage::Format mFormat;
    QSizeF mPhysicalSize;
    QImage mScreenImage;

private:
    QPainter *mPainter;
    QList<QFbBackingStore*> mPendingBackingStores;

    friend class QFbWindow;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFbScreen::Flags)

QT_END_NAMESPACE

#endif // QFBSCREEN_P_H
