// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPAGE_P_P_H
#define QQUICKPAGE_P_P_H

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

#include <QtQuickTemplates2/private/qquickpane_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickPane;

class QQuickPagePrivate : public QQuickPanePrivate
{
    Q_DECLARE_PUBLIC(QQuickPage)

public:
    void relayout();
    void resizeContent() override;

    void itemVisibilityChanged(QQuickItem *item) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF & diff) override;
    void itemDestroyed(QQuickItem *item) override;

    QString title;
    QQuickItem *header = nullptr;
    QQuickItem *footer = nullptr;
    bool emittingImplicitSizeChangedSignals = false;
};

QT_END_NAMESPACE

#endif // QQUICKPAGE_P_P_H
