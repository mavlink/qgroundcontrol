// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDEFAULTBUSYINDICATOR_P_H
#define QQUICKDEFAULTBUSYINDICATOR_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtGui/qcolor.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickBasicBusyIndicator : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor pen READ pen WRITE setPen FINAL)
    Q_PROPERTY(QColor fill READ fill WRITE setFill FINAL)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning)
    QML_NAMED_ELEMENT(BusyIndicatorImpl)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickBasicBusyIndicator(QQuickItem *parent = nullptr);

    QColor pen() const;
    void setPen(const QColor &pen);

    QColor fill() const;
    void setFill(const QColor &fill);

    bool isRunning() const;
    void setRunning(bool running);

    int elapsed() const;

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    bool m_running = false;
    int m_elapsed = 0;
    QColor m_pen;
    QColor m_fill;
};

QT_END_NAMESPACE

#endif // QQUICKDEFAULTBUSYINDICATOR_P_H
