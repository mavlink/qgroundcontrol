// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKUNIVERSALBUSYINDICATOR_P_H
#define QQUICKUNIVERSALBUSYINDICATOR_P_H

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

#include <QtGui/qcolor.h>
#include <QtQuick/qquickitem.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickUniversalBusyIndicator : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor FINAL)
    QML_NAMED_ELEMENT(BusyIndicatorImpl)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickUniversalBusyIndicator(QQuickItem *parent = nullptr);

    int count() const;
    void setCount(int count);

    QColor color() const;
    void setColor(const QColor &color);

    int elapsed() const;

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    int m_count = 5;
    int m_elapsed = 0;
    QColor m_color = Qt::black;
};

QT_END_NAMESPACE

#endif // QQUICKUNIVERSALBUSYINDICATOR_P_H
