// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOPUPPOSITIONER_P_P_H
#define QQUICKPOPUPPOSITIONER_P_P_H

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

#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPopup;

class Q_QUICKTEMPLATES2_EXPORT QQuickPopupPositioner : public QQuickItemChangeListener
{
public:
    explicit QQuickPopupPositioner(QQuickPopup *popup);
    ~QQuickPopupPositioner();

    QQuickPopup *popup() const;

    QQuickItem *parentItem() const;
    void setParentItem(QQuickItem *parent);

    virtual void reposition();

protected:
    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &) override;
    void itemParentChanged(QQuickItem *, QQuickItem *parent) override;
    void itemChildRemoved(QQuickItem *, QQuickItem *child) override;

    void removeAncestorListeners(QQuickItem *item);
    void addAncestorListeners(QQuickItem *item);

    void repositionPopupWindow();

    bool m_positioning = false;
    QQuickItem *m_parentItem = nullptr;
    QQuickPopup *m_popup = nullptr;
    qreal m_popupScale = 1.0;
};

QT_END_NAMESPACE

#endif // QQUICKPOPUPPOSITIONER_P_P_H
