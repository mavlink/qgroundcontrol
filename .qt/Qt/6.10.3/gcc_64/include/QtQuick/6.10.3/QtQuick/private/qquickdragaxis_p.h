// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDRAGAXIS_P_H
#define QQUICKDRAGAXIS_P_H

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

#include <QtQml/qqml.h>
#include <QtQml/qqmlproperty.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPointerHandler;

class Q_QUICK_EXPORT QQuickDragAxis : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(qreal maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal activeValue READ activeValue NOTIFY activeValueChanged REVISION(6, 5))
    QML_NAMED_ELEMENT(DragAxis)
    QML_ADDED_IN_VERSION(2, 12)
    QML_UNCREATABLE("DragAxis is only available as a grouped property of DragHandler or PinchHandler.")

public:
    QQuickDragAxis(QQuickPointerHandler *handler, const QString &propertyName,
                   qreal initValue = 0);

    qreal minimum() const { return m_minimum; }
    void setMinimum(qreal minimum);

    qreal maximum() const { return m_maximum; }
    void setMaximum(qreal maximum);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    qreal activeValue() const { return m_activeValue; }

    qreal persistentValue() const { return m_accumulatedValue; }

protected:
    void onActiveChanged(bool active, qreal initActiveValue);
    qreal targetValue();
    void updateValue(qreal activeValue, qreal accumulatedValue, qreal delta = 0);

Q_SIGNALS:
    void minimumChanged();
    void maximumChanged();
    void enabledChanged();
    Q_REVISION(6, 5) void activeValueChanged(qreal delta);

private:
    qreal m_minimum = std::numeric_limits<qreal>::lowest();
    qreal m_maximum = std::numeric_limits<qreal>::max();
    qreal m_startValue = 0;
    qreal m_activeValue = 0;
    qreal m_accumulatedValue = 0;
    QString m_propertyName;
    bool m_enabled = true;

    friend class QQuickDragHandler;
    friend class QQuickPinchHandler;
};

QT_END_NAMESPACE

#endif // QQUICKDRAGAXIS_P_H
