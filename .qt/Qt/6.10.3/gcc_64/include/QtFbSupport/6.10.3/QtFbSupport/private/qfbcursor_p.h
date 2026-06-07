// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFBCURSOR_P_H
#define QFBCURSOR_P_H

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

#include <qpa/qplatformcursor.h>
#include <QtGui/private/qinputdevicemanager_p.h>

QT_BEGIN_NAMESPACE

class QFbScreen;
class QFbCursor;

class QFbCursorDeviceListener : public QObject
{
    Q_OBJECT

public:
    QFbCursorDeviceListener(QFbCursor *cursor) : m_cursor(cursor) { }
    bool hasMouse() const;

public slots:
    void onDeviceListChanged(QInputDeviceManager::DeviceType type);

private:
    QFbCursor *m_cursor;
};

class QFbCursor : public QPlatformCursor
{
    Q_OBJECT

public:
    QFbCursor(QFbScreen *screen);
    ~QFbCursor();

    // output methods
    QRect dirtyRect();
    virtual QRect drawCursor(QPainter &painter);

    // input methods
    void pointerEvent(const QMouseEvent &event) override;
    QPoint pos() const override;
    void setPos(const QPoint &pos) override;
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor *widgetCursor, QWindow *window) override;
#endif

    virtual void setDirty();
    virtual bool isDirty() const { return mDirty; }
    virtual bool isOnScreen() const { return mOnScreen; }
    virtual QRect lastPainted() const { return mPrevRect; }

    void updateMouseStatus();

private:
    void setCursor(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY);
    void setCursor(Qt::CursorShape shape);
    void setCursor(const QImage &image, int hotx, int hoty);
    QRect getCurrentRect() const;

    bool mVisible;
    QFbScreen *mScreen;
    QRect mCurrentRect;      // next place to draw the cursor
    QRect mPrevRect;         // last place the cursor was drawn
    bool mDirty;
    bool mOnScreen;
    QScopedPointer<QPlatformCursorImage> mCursorImage;
    QFbCursorDeviceListener *mDeviceListener;
    QPoint m_pos;
};

QT_END_NAMESPACE

#endif // QFBCURSOR_P_H
