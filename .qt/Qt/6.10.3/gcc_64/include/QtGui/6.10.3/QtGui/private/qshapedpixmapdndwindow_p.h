// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHAPEDPIXMAPDNDWINDOW_H
#define QSHAPEDPIXMAPDNDWINDOW_H

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
#include <QtGui/QRasterWindow>
#include <QtGui/QPixmap>

QT_REQUIRE_CONFIG(draganddrop);

QT_BEGIN_NAMESPACE

class QShapedPixmapWindow : public QRasterWindow
{
    Q_OBJECT
public:
    explicit QShapedPixmapWindow(QScreen *screen = nullptr);
    ~QShapedPixmapWindow();

    void setUseCompositing(bool on) { m_useCompositing = on; }
    void setPixmap(const QPixmap &pixmap);
    void setHotspot(const QPoint &hotspot);

    void updateGeometry(const QPoint &pos);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QPixmap m_pixmap;
    QPoint m_hotSpot;
    bool m_useCompositing;
};

QT_END_NAMESPACE

#endif // QSHAPEDPIXMAPDNDWINDOW_H
