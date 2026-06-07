// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPANE_P_P_H
#define QQUICKPANE_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickPane;

class Q_QUICKTEMPLATES2_EXPORT QQuickPanePrivate : public QQuickControlPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickPane)

    void init();

    virtual QQmlListProperty<QObject> contentData();
    virtual QQmlListProperty<QQuickItem> contentChildren();
    virtual QList<QQuickItem *> contentChildItems() const;
    virtual QQuickItem *getFirstChild() const;

    QQuickItem *getContentItem() override;

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    void contentChildrenChange();

    void updateContentWidth();
    void updateContentHeight();

    bool handlePress(const QPointF &point, ulong timestamp) override;

    bool hasContentWidth = false;
    bool hasContentHeight = false;
    qreal contentWidth = 0;
    qreal contentHeight = 0;
    QQuickItem *firstChild = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPANE_P_P_H
