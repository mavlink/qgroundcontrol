// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCONTEXT2DTILE_P_H
#define QQUICKCONTEXT2DTILE_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_canvas);

#include "qquickcontext2d_p.h"
QT_BEGIN_NAMESPACE

class QQuickContext2DTexture;
class QQuickContext2DCommandBuffer;

class QQuickContext2DTile
{
public:
    QQuickContext2DTile();
    virtual ~QQuickContext2DTile();

    bool dirty() const {return m_dirty;}
    void markDirty(bool dirty) {m_dirty = dirty;}

    QRect rect() const {return m_rect;}

    virtual void setRect(const QRect& r) = 0;
    virtual QPainter* createPainter(bool smooth, bool antialiasing);
    virtual void drawFinished() {}

protected:
    virtual void aboutToDraw() {}
    uint m_dirty : 1;
    QRect m_rect;
    QPaintDevice* m_device;
    QPainter m_painter;
};

class QQuickContext2DImageTile : public QQuickContext2DTile
{
public:
    QQuickContext2DImageTile();
    ~QQuickContext2DImageTile();
    void setRect(const QRect& r) override;
    const QImage& image() const {return m_image;}
private:
    QImage m_image;
};
QT_END_NAMESPACE

#endif // QQUICKCONTEXT2DTILE_P_H
