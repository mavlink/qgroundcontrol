// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSGBMCURSOR_H
#define QEGLFSKMSGBMCURSOR_H

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
#include <QtCore/QList>
#include <QtGui/QImage>
#include <QtGui/private/qinputdevicemanager_p.h>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsGbmScreen;
class QEglFSKmsGbmCursor;

class QEglFSKmsGbmCursorDeviceListener : public QObject
{
    Q_OBJECT

public:
    QEglFSKmsGbmCursorDeviceListener(QEglFSKmsGbmCursor *cursor) : m_cursor(cursor) { }
    bool hasMouse() const;

public slots:
    void onDeviceListChanged(QInputDeviceManager::DeviceType type);

private:
    QEglFSKmsGbmCursor *m_cursor;
};

class QEglFSKmsGbmCursor : public QPlatformCursor
{
    Q_OBJECT

public:
    QEglFSKmsGbmCursor(QEglFSKmsGbmScreen *screen);
    ~QEglFSKmsGbmCursor();

    // input methods
    void pointerEvent(const QMouseEvent & event) override;
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor * windowCursor, QWindow * window) override;
#endif
    QPoint pos() const override;
    void setPos(const QPoint &pos) override;

    void updateMouseStatus();

    void reevaluateVisibilityForScreens() { setPos(pos()); }

private:
    void initCursorAtlas();

    enum CursorState {
        CursorDisabled,
        CursorPendingHidden,
        CursorHidden,
        CursorPendingVisible,
        CursorVisible
    };

    QEglFSKmsGbmScreen *m_screen;
    QSize m_cursorSize;
    gbm_bo *m_bo;
    QPoint m_pos;
    QPlatformCursorImage m_cursorImage;
    CursorState m_state;
    QEglFSKmsGbmCursorDeviceListener *m_deviceListener;

    // cursor atlas information
    struct CursorAtlas {
        CursorAtlas() : cursorsPerRow(0), cursorWidth(0), cursorHeight(0) { }
        int cursorsPerRow;
        int width, height; // width and height of the atlas
        int cursorWidth, cursorHeight; // width and height of cursors inside the atlas
        QList<QPoint> hotSpots;
        QImage image;
    } m_cursorAtlas;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMCURSOR_H
