// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMGRAPHICSBUFFER_H
#define QPLATFORMGRAPHICSBUFFER_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//


#include <QtGui/qtguiglobal.h>
#include <QtCore/QSize>
#include <QtCore/QRect>
#include <QtGui/QPixelFormat>
#include <QtCore/qflags.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPlatformGraphicsBuffer : public QObject
{
Q_OBJECT
public:
    enum AccessType
    {
        None                = 0x00,
        SWReadAccess        = 0x01,
        SWWriteAccess       = 0x02,
        TextureAccess       = 0x04,
        HWCompositor        = 0x08
    };
    Q_ENUM(AccessType)
    Q_DECLARE_FLAGS(AccessTypes, AccessType)

    enum Origin {
        OriginBottomLeft,
        OriginTopLeft
    };
    Q_ENUM(Origin)

    ~QPlatformGraphicsBuffer();

    AccessTypes isLocked() const { return m_lock_access; }
    bool lock(AccessTypes access, const QRect &rect = QRect());
    void unlock();

    virtual bool bindToTexture(const QRect &rect = QRect()) const;

    virtual const uchar *data() const;
    virtual uchar *data();
    virtual int bytesPerLine() const;
    int byteCount() const;

    virtual Origin origin() const;

    QSize size() const { return m_size; }
    QPixelFormat format() const { return m_format; }

Q_SIGNALS:
    void unlocked(AccessTypes previousAccessTypes);

protected:
    QPlatformGraphicsBuffer(const QSize &size, const QPixelFormat &format);

    virtual bool doLock(AccessTypes access, const QRect &rect = QRect()) = 0;
    virtual void doUnlock() = 0;

private:
    QSize m_size;
    QPixelFormat m_format;
    AccessTypes m_lock_access;
};

QT_END_NAMESPACE

#endif //QPLATFORMGRAPHICSBUFFER_H
