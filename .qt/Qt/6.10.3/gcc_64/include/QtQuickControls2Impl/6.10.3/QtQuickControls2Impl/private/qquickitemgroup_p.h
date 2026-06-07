// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMGROUP_P_H
#define QQUICKITEMGROUP_P_H

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

#include <QtQuick/private/qquickimplicitsizeitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickItemGroup : public QQuickImplicitSizeItem, protected QQuickItemChangeListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ItemGroup)
    QML_ADDED_IN_VERSION(2, 2)

public:
    explicit QQuickItemGroup(QQuickItem *parent = nullptr);
    ~QQuickItemGroup();

protected:
    void watch(QQuickItem *item);
    void unwatch(QQuickItem *item);

    QSizeF calculateImplicitSize() const;
    void updateImplicitSize();

    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
};

QT_END_NAMESPACE

#endif // QQUICKITEMGROUP_P_H
