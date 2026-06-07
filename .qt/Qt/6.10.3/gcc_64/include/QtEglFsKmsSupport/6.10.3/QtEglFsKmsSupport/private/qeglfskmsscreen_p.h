// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSSCREEN_H
#define QEGLFSKMSSCREEN_H

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

#include "private/qeglfsscreen_p.h"
#include <QtCore/QList>
#include <QtCore/QMutex>

#include <QtKmsSupport/private/qkmsdevice_p.h>
#include <QtGui/private/qedidparser_p.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;
class QEglFSKmsInterruptHandler;

class Q_EGLFS_EXPORT QEglFSKmsScreen : public QEglFSScreen
{
public:
    QEglFSKmsScreen(QEglFSKmsDevice *device, const QKmsOutput &output, bool headless = false);
    ~QEglFSKmsScreen();

    void setVirtualPosition(const QPoint &pos);

    QRect rawGeometry() const override;

    int depth() const override;
    QImage::Format format() const override;

    QSizeF physicalSize() const override;
    QDpi logicalDpi() const override;
    QDpi logicalBaseDpi() const override;
    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;

    QString name() const override;

    QString manufacturer() const override;
    QString model() const override;
    QString serialNumber() const override;

    qreal refreshRate() const override;

    QList<QPlatformScreen *> virtualSiblings() const override { return m_siblings; }
    void setVirtualSiblings(QList<QPlatformScreen *> sl) { m_siblings = sl; }

    QList<QPlatformScreen::Mode> modes() const override;

    int currentMode() const override;
    int preferredMode() const override;

    QEglFSKmsDevice *device() const { return m_device; }

    virtual void waitForFlip();

    QKmsOutput &output() { return m_output; }
    void restoreMode();

    SubpixelAntialiasingType subpixelAntialiasingTypeHint() const override;

    QPlatformScreen::PowerState powerState() const override;
    void setPowerState(QPlatformScreen::PowerState state) override;

    bool isCursorOutOfRange() const { return m_cursorOutOfRange; }
    void setCursorOutOfRange(bool b) { m_cursorOutOfRange = b; }

    virtual void pageFlipped(unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec);
protected:
    QEglFSKmsDevice *m_device;

    QKmsOutput m_output;
    QEdidParser m_edid;
    QPoint m_pos;
    bool m_cursorOutOfRange;

    QList<QPlatformScreen *> m_siblings;

    PowerState m_powerState;

    QEglFSKmsInterruptHandler *m_interruptHandler;

    bool m_headless;
};

QT_END_NAMESPACE

#endif
