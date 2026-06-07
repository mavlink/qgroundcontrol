// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMATERIALBUSYINDICATOR_P_H
#define QQUICKMATERIALBUSYINDICATOR_P_H

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

class QQuickMaterialBusyIndicator : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor FINAL)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning FINAL)
    QML_NAMED_ELEMENT(BusyIndicatorImpl)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickMaterialBusyIndicator(QQuickItem *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);

    bool isRunning() const;
    void setRunning(bool running);

    int elapsed() const;

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    bool m_running = false;
    int m_elapsed = 0;
    QColor m_color = Qt::black;
};

QT_END_NAMESPACE

#endif // QQUICKMATERIALBUSYINDICATOR_P_H
