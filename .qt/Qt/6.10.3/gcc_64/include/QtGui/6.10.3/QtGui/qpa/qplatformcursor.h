// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPLATFORMCURSOR_H
#define QPLATFORMCURSOR_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/QList>
#include <QtGui/QImage>
#include <QtGui/QMouseEvent>
#include <QtCore/QObject>
#include <qpa/qplatformscreen.h>
#include <QtGui/QCursor>

QT_BEGIN_NAMESPACE


// Cursor graphics management
class Q_GUI_EXPORT QPlatformCursorImage {
public:
    QPlatformCursorImage(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY)
    { set(data, mask, width, height, hotX, hotY); }
    QImage * image() { return &cursorImage; }
    QPoint hotspot() const { return hot; }
    void set(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY);
    void set(const QImage &image, int hx, int hy);
    void set(Qt::CursorShape);
private:
    static void createSystemCursor(int id);
    QImage cursorImage;
    QPoint hot;
};

class Q_GUI_EXPORT QPlatformCursor : public QObject {
public:
    Q_DISABLE_COPY_MOVE(QPlatformCursor)

    enum Capability {
        OverrideCursor = 0x1
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    QPlatformCursor();

    // input methods
    virtual void pointerEvent(const QMouseEvent & event) { Q_UNUSED(event); }
#ifndef QT_NO_CURSOR
    virtual void changeCursor(QCursor * windowCursor, QWindow * window) = 0;
    virtual void setOverrideCursor(const QCursor &);
    virtual void clearOverrideCursor();
#endif // QT_NO_CURSOR
    virtual QPoint pos() const;
    virtual void setPos(const QPoint &pos);
    virtual QSize size() const;

    static Capabilities capabilities() { return m_capabilities; }
    static void setCapabilities(Capabilities c) { m_capabilities = c; }
    static void setCapability(Capability c) { m_capabilities.setFlag(c); }

private:
    friend void qt_qpa_set_cursor(QWidget * w, bool force);
    friend class QApplicationPrivate;

    static Capabilities m_capabilities;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QPlatformCursor::Capabilities)

QT_END_NAMESPACE

#endif // QPLATFORMCURSOR_H
