// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFBBACKINGSTORE_P_H
#define QFBBACKINGSTORE_P_H

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

#include <qpa/qplatformbackingstore.h>
#include <QtCore/QMutex>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QFbScreen;
class QFbWindow;
class QWindow;

class QFbBackingStore : public QPlatformBackingStore
{
public:
    QFbBackingStore(QWindow *window);
    ~QFbBackingStore();

    QPaintDevice *paintDevice() override { return &mImage; }
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;

    void resize(const QSize &size, const QRegion &region) override;

    const QImage image();
    QImage toImage() const override;

    void lock();
    void unlock();

    void beginPaint(const QRegion &) override;
    void endPaint() override;

protected:
    friend class QFbWindow;

    QImage mImage;
    QMutex mImageMutex;
};

QT_END_NAMESPACE

#endif // QFBBACKINGSTORE_P_H

