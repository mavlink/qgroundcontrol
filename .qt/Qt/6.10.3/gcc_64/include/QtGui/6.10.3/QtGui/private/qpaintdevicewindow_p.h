// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAINTDEVICEWINDOW_P_H
#define QPAINTDEVICEWINDOW_P_H

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
#include <QtGui/QPaintDeviceWindow>
#include <QtCore/QCoreApplication>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/QPaintEvent>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPaintDeviceWindowPrivate : public QWindowPrivate
{
    Q_DECLARE_PUBLIC(QPaintDeviceWindow)

public:
    QPaintDeviceWindowPrivate();
    ~QPaintDeviceWindowPrivate() override;

    virtual void handleResizeEvent() {}

    virtual void beginPaint(const QRegion &region)
    {
        Q_UNUSED(region);
    }

    virtual void endPaint()
    {
    }

    virtual void flush(const QRegion &region)
    {
        Q_UNUSED(region);
    }

    bool paint(const QRegion &region)
    {
        Q_Q(QPaintDeviceWindow);
        QRegion toPaint = region & dirtyRegion;
        if (toPaint.isEmpty())
            return false;

        // Clear the region now. The overridden functions may call update().
        dirtyRegion -= toPaint;

        beginPaint(toPaint);

        QPaintEvent paintEvent(toPaint);
        q->paintEvent(&paintEvent);

        endPaint();

        return true;
    }

    void doFlush(const QRegion &region)
    {
        QRegion toFlush = region;
        if (paint(toFlush))
            flush(toFlush);
    }

    void handleUpdateEvent()
    {
        if (dirtyRegion.isEmpty())
            return;
        doFlush(dirtyRegion);
    }

    void markWindowAsDirty()
    {
        Q_Q(QPaintDeviceWindow);
        dirtyRegion = QRect(QPoint(0, 0), q->size());
    }

private:
    QRegion dirtyRegion;
};


QT_END_NAMESPACE

#endif //QPAINTDEVICEWINDOW_P_H
