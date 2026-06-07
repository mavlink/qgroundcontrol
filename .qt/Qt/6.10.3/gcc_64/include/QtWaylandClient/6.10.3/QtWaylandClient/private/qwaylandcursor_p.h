// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDCURSOR_H
#define QWAYLANDCURSOR_H

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
#include <QtCore/QMap>
#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwayland-cursor-shape-v1.h>
#include <QtCore/private/qglobal_p.h>

#if QT_CONFIG(cursor)

#include <memory>

struct wl_cursor;
struct wl_cursor_image;
struct wl_cursor_theme;

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandBuffer;
class QWaylandDisplay;
class QWaylandScreen;
class QWaylandShm;

class Q_WAYLANDCLIENT_EXPORT QWaylandCursorTheme
{
public:
    static std::unique_ptr<QWaylandCursorTheme> create(QWaylandShm *shm, int size, const QString &themeName);
    ~QWaylandCursorTheme();
    ::wl_cursor *cursor(Qt::CursorShape shape);

protected:
    enum WaylandCursor {
        ArrowCursor = Qt::ArrowCursor,
        UpArrowCursor,
        CrossCursor,
        WaitCursor,
        IBeamCursor,
        SizeVerCursor,
        SizeHorCursor,
        SizeBDiagCursor,
        SizeFDiagCursor,
        SizeAllCursor,
        BlankCursor,
        SplitVCursor,
        SplitHCursor,
        PointingHandCursor,
        ForbiddenCursor,
        WhatsThisCursor,
        BusyCursor,
        OpenHandCursor,
        ClosedHandCursor,
        DragCopyCursor,
        DragMoveCursor,
        DragLinkCursor,
        // The following are used for cursors that don't have equivalents in Qt
        ResizeNorthCursor = Qt::CustomCursor + 1,
        ResizeSouthCursor,
        ResizeEastCursor,
        ResizeWestCursor,
        ResizeNorthWestCursor,
        ResizeSouthEastCursor,
        ResizeNorthEastCursor,
        ResizeSouthWestCursor,

        NumWaylandCursors
    };

    explicit QWaylandCursorTheme(struct ::wl_cursor_theme *theme) : m_theme(theme) {}
    struct ::wl_cursor *requestCursor(WaylandCursor shape);
    struct ::wl_cursor_theme *m_theme = nullptr;
    wl_cursor *m_cursors[NumWaylandCursors] = {};
};

class Q_WAYLANDCLIENT_EXPORT QWaylandCursorShape : public QtWayland::wp_cursor_shape_device_v1
{
public:
    QWaylandCursorShape(struct ::wp_cursor_shape_device_v1 *object);
    ~QWaylandCursorShape();
    void setShape(uint32_t serial, Qt::CursorShape shape);
};

class Q_WAYLANDCLIENT_EXPORT QWaylandCursor : public QPlatformCursor
{
public:
    explicit QWaylandCursor(QWaylandDisplay *display);

    void changeCursor(QCursor *cursor, QWindow *window) override;
    void pointerEvent(const QMouseEvent &event) override;
    QPoint pos() const override;
    void setPos(const QPoint &pos) override;
    void setPosFromEnterEvent(const QPoint &pos);

    QSize size() const override;

    static QSharedPointer<QWaylandBuffer> cursorBitmapBuffer(QWaylandDisplay *display, const QCursor *cursor);

protected:
    QWaylandDisplay *mDisplay = nullptr;
    QPoint mLastPos;
};

}

QT_END_NAMESPACE

#endif // cursor
#endif // QWAYLANDCURSOR_H
