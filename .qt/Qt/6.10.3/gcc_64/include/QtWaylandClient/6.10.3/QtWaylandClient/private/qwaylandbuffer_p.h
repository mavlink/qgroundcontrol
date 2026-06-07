// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDBUFFER_H
#define QWAYLANDBUFFER_H

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

#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtCore/QSize>
#include <QtCore/QRect>

#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class Q_WAYLANDCLIENT_EXPORT QWaylandBuffer {
public:
    QWaylandBuffer();
    virtual ~QWaylandBuffer();
    void init(wl_buffer *buf);

    wl_buffer *buffer() {return mBuffer;}
    virtual QSize size() const = 0;
    virtual int scale() const { return 1; }

    void setBusy(bool busy) { mBusy = busy; }
    bool busy() const { return mBusy; }

    void setCommitted() { mCommitted = true; }
    bool committed() const { return mCommitted; }

    void setDeleteOnRelease(bool deleteOnRelease);

protected:
    struct wl_buffer *mBuffer = nullptr;

private:
    bool mBusy = false;
    bool mCommitted = false;
    bool mDeleteOnRelease = false;

    static void release(void *data, wl_buffer *);
    static const wl_buffer_listener listener;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDBUFFER_H
