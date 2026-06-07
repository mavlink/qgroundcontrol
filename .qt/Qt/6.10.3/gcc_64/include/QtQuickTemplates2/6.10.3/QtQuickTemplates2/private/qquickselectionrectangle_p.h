// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSELECTIONRECTANGLE_P_H
#define QQUICKSELECTIONRECTANGLE_P_H

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
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickSelectionRectanglePrivate;
class QQuickSelectable;
class QQuickSelectionRectangleAttached;
class QQmlComponent;

class Q_QUICKTEMPLATES2_EXPORT QQuickSelectionRectangle : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionModeChanged FINAL)
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged FINAL)
    Q_PROPERTY(QQmlComponent *topLeftHandle READ topLeftHandle WRITE setTopLeftHandle NOTIFY topLeftHandleChanged FINAL)
    Q_PROPERTY(QQmlComponent *bottomRightHandle READ bottomRightHandle WRITE setBottomRightHandle NOTIFY bottomRightHandleChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged FINAL)

    QML_NAMED_ELEMENT(SelectionRectangle)
    QML_ATTACHED(QQuickSelectionRectangleAttached)
    QML_ADDED_IN_VERSION(6, 2)

public:
    enum SelectionMode {
        Drag,
        PressAndHold,
        Auto
    };
    Q_ENUM(SelectionMode)

    explicit QQuickSelectionRectangle(QQuickItem *parent = nullptr);

    QQuickItem *target() const;
    void setTarget(QQuickItem *target);

    bool active();
    bool dragging();

    SelectionMode selectionMode() const;
    void setSelectionMode(SelectionMode selectionMode);

    QQmlComponent *topLeftHandle() const;
    void setTopLeftHandle(QQmlComponent *topLeftHandle);
    QQmlComponent *bottomRightHandle() const;
    void setBottomRightHandle(QQmlComponent *bottomRightHandle);

    static QQuickSelectionRectangleAttached *qmlAttachedProperties(QObject *obj);

Q_SIGNALS:
    void targetChanged();
    void activeChanged();
    void draggingChanged();
    void topLeftHandleChanged();
    void bottomRightHandleChanged();
    void selectionModeChanged();

private:
    Q_DISABLE_COPY(QQuickSelectionRectangle)
    Q_DECLARE_PRIVATE(QQuickSelectionRectangle)
};

class Q_QUICKTEMPLATES2_EXPORT QQuickSelectionRectangleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickSelectionRectangle *control READ control NOTIFY controlChanged FINAL)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged FINAL)

public:
    QQuickSelectionRectangleAttached(QObject *parent);

    QQuickSelectionRectangle *control() const;
    void setControl(QQuickSelectionRectangle *control);

    bool dragging() const;
    void setDragging(bool dragging);

Q_SIGNALS:
    void controlChanged();
    void draggingChanged();

private:
    QPointer<QQuickSelectionRectangle> m_control;
    bool m_dragging = false;

    friend class QQuickSelectionRectanglePrivate;
};

QT_END_NAMESPACE

#endif // QQUICKSELECTIONRECTANGLE_P_H
