// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKBUSYINDICATOR_P_H
#define QQUICKBUSYINDICATOR_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickBusyIndicatorPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickBusyIndicator : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged FINAL)
    QML_NAMED_ELEMENT(BusyIndicator)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickBusyIndicator(QQuickItem *parent = nullptr);

    bool isRunning() const;
    void setRunning(bool running);

Q_SIGNALS:
    void runningChanged();

protected:
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickBusyIndicator)
    Q_DECLARE_PRIVATE(QQuickBusyIndicator)
};

QT_END_NAMESPACE

#endif // QQUICKBUSYINDICATOR_P_H
