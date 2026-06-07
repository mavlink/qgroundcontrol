// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKOVERLAY_P_P_H
#define QQUICKOVERLAY_P_P_H

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

#include <QtQuickTemplates2/private/qquickoverlay_p.h>

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickPopup;

class Q_AUTOTEST_EXPORT QQuickOverlayPrivate : public QQuickItemPrivate,
                                               public QSafeQuickItemChangeListener<QQuickOverlayPrivate>
{
public:
    Q_DECLARE_PUBLIC(QQuickOverlay)

    static QQuickOverlayPrivate *get(QQuickOverlay *overlay)
    {
        return overlay->d_func();
    }

    bool startDrag(QEvent *event, const QPointF &pos);
    bool handlePress(QQuickItem *source, QEvent *event, QQuickPopup *target);
    bool handleMove(QQuickItem *source, QEvent *event, QQuickPopup *target);
    bool handleRelease(QQuickItem *source, QEvent *event, QQuickPopup *target);

    bool handleMouseEvent(QQuickItem *source, QMouseEvent *event, QQuickPopup *target = nullptr);
    bool handleHoverEvent(QQuickItem *source, QHoverEvent *event, QQuickPopup *target = nullptr);
#if QT_CONFIG(quicktemplates2_multitouch)
    bool handleTouchEvent(QQuickItem *source, QTouchEvent *event, QQuickPopup *target = nullptr);
#endif

    void addPopup(QQuickPopup *popup);
    void removePopup(QQuickPopup *popup);
    void setMouseGrabberPopup(QQuickPopup *popup);

    QList<QQuickPopup *> stackingOrderPopups() const;
    QList<QQuickPopup *> stackingOrderDrawers() const;

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemRotationChanged(QQuickItem *item) override;

    void updateGeometry();

    QQmlComponent *modal = nullptr;
    QQmlComponent *modeless = nullptr;
    QList<QQuickPopup *> allPopups;
    // Store drawers as QQuickPopup instead of QQuickDrawer because they're no longer
    // QQuickDrawer by the time removePopup is called.
    QList<QQuickPopup *> allDrawers;
    QPointer<QQuickPopup> mouseGrabberPopup;
    QPointer<QQuickItem> lastActiveFocusItem;
    QPointer<QQuickPopup> lastActiveFocusItemPopup;
};

QT_END_NAMESPACE

#endif // QQUICKOVERLAY_P_P_H
