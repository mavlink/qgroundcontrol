// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBLEQUICKWIDGET_H
#define QACCESSIBLEQUICKWIDGET_H

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

#include "qquickwidget.h"
#include <QtWidgets/qaccessiblewidget.h>

#include <private/qaccessiblequickview_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

// These classes implement the QQuickWiget accessibility switcharoo,
// where the child items of the QQuickWidgetOffscreenWindow are reported
// as child accessible interfaces of the QAccessibleQuickWidget.
class QAccessibleQuickWidget: public QAccessibleWidgetV2
{
public:
    QAccessibleQuickWidget(QQuickWidget* widget);
    ~QAccessibleQuickWidget();

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QAccessibleInterface *childAt(int x, int y) const override;

private:
    void repairWindow();

    std::unique_ptr<QAccessibleQuickWindow> m_accessibleWindow;
    QMetaObject::Connection m_connection;
    Q_DISABLE_COPY(QAccessibleQuickWidget)
};

class QAccessibleQuickWidgetOffscreenWindow: public QAccessibleQuickWindow
{
public:
    QAccessibleQuickWidgetOffscreenWindow(QQuickWindow *window);
    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QAccessibleInterface *childAt(int x, int y) const override;
};

#endif // accessibility

QT_END_NAMESPACE

#endif
