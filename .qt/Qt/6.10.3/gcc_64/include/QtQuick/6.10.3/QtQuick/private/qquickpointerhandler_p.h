// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOINTERHANDLER_H
#define QQUICKPOINTERHANDLER_H

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

#include <QtCore/QObject>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtconfigmacros.h>
#include <QtGui/qeventpoint.h>
#include <QtGui/qpointingdevice.h>
#include <QtQml/QQmlParserStatus>
#include <QtQml/qqmlregistration.h>
#include <QtQuick/qtquickglobal.h>
#include <QtQuick/qtquickexports.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcPointerHandlerDispatch)

class QQuickItem;
class QQuickPointerHandlerPrivate;
class QPointerEvent;

class Q_QUICK_EXPORT QQuickPointerHandler : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QQuickItem * target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QQuickItem * parent READ parentItem WRITE setParentItem NOTIFY parentChanged)
    Q_PROPERTY(GrabPermissions grabPermissions READ grabPermissions WRITE setGrabPermissions NOTIFY grabPermissionChanged)
    Q_PROPERTY(qreal margin READ margin WRITE setMargin NOTIFY marginChanged)
    Q_PROPERTY(int dragThreshold READ dragThreshold WRITE setDragThreshold RESET resetDragThreshold NOTIFY dragThresholdChanged REVISION(2, 15))
#if QT_CONFIG(cursor)
    Q_PROPERTY(Qt::CursorShape cursorShape READ cursorShape WRITE setCursorShape RESET resetCursorShape NOTIFY cursorShapeChanged REVISION(2, 15))
#endif

    Q_CLASSINFO("ParentProperty", "parent")
    QML_NAMED_ELEMENT(PointerHandler)
    QML_UNCREATABLE("PointerHandler is an abstract base class.")
    QML_ADDED_IN_VERSION(2, 12)

public:
    explicit QQuickPointerHandler(QQuickItem *parent = nullptr);
    ~QQuickPointerHandler();

    enum GrabPermission {
        TakeOverForbidden = 0x0,
        CanTakeOverFromHandlersOfSameType = 0x01,
        CanTakeOverFromHandlersOfDifferentType= 0x02,
        CanTakeOverFromItems = 0x04,
        CanTakeOverFromAnything = 0x0F,
        ApprovesTakeOverByHandlersOfSameType = 0x10,
        ApprovesTakeOverByHandlersOfDifferentType= 0x20,
        ApprovesTakeOverByItems = 0x40,
        ApprovesCancellation = 0x80,
        ApprovesTakeOverByAnything = 0xF0
    };
    Q_DECLARE_FLAGS(GrabPermissions, GrabPermission)
    Q_FLAG(GrabPermissions)

public:
    bool enabled() const;
    void setEnabled(bool enabled);

    bool active() const;

    QQuickItem *target() const;
    void setTarget(QQuickItem *target);

    QQuickItem * parentItem() const;
    void setParentItem(QQuickItem *p);

    void handlePointerEvent(QPointerEvent *event);

    GrabPermissions grabPermissions() const;
    void setGrabPermissions(GrabPermissions grabPermissions);

    qreal margin() const;
    void setMargin(qreal pointDistanceThreshold);

    int dragThreshold() const;
    void setDragThreshold(int t);
    void resetDragThreshold();

#if QT_CONFIG(cursor)
    Qt::CursorShape cursorShape() const;
    void setCursorShape(Qt::CursorShape shape);
    void resetCursorShape();
    bool isCursorShapeExplicitlySet() const;
#endif

Q_SIGNALS:
    void enabledChanged();
    void activeChanged();
    void targetChanged();
    void marginChanged();
    Q_REVISION(2, 15) void dragThresholdChanged();
    void grabChanged(QPointingDevice::GrabTransition transition, QEventPoint point);
    void grabPermissionChanged();
    void canceled(QEventPoint point);
#if QT_CONFIG(cursor)
    Q_REVISION(2, 15) void cursorShapeChanged();
#endif
    Q_REVISION(6, 3) void parentChanged();

protected:
    QQuickPointerHandler(QQuickPointerHandlerPrivate &dd, QQuickItem *parent);

    void classBegin() override;
    void componentComplete() override;
    bool event(QEvent *) override;

    QPointerEvent *currentEvent();
    virtual bool wantsPointerEvent(QPointerEvent *event);
    virtual bool wantsEventPoint(const QPointerEvent *event, const QEventPoint &point);
    virtual void handlePointerEventImpl(QPointerEvent *event);
    void setActive(bool active);
    virtual void onTargetChanged(QQuickItem *oldTarget) { Q_UNUSED(oldTarget); }
    virtual void onActiveChanged() { }
    virtual void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition,
                               QPointerEvent *event, QEventPoint &point);
    virtual bool canGrab(QPointerEvent *event, const QEventPoint &point);
    virtual bool approveGrabTransition(QPointerEvent *event, const QEventPoint &point, QObject *proposedGrabber);
    void setPassiveGrab(QPointerEvent *event, const QEventPoint &point, bool grab = true);
    bool setExclusiveGrab(QPointerEvent *ev, const QEventPoint &point, bool grab = true);
    void cancelAllGrabs(QPointerEvent *event, QEventPoint &point);
    QPointF eventPos(const QEventPoint &point) const;
    bool parentContains(const QEventPoint &point) const;
    bool parentContains(const QPointF &scenePosition) const;

    friend class QQuickDeliveryAgentPrivate;
    friend class QQuickItemPrivate;
    friend class QQuickMenuPrivate;
    friend class QQuickWindowPrivate;

    Q_DECLARE_PRIVATE(QQuickPointerHandler)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickPointerHandler::GrabPermissions)

QT_END_NAMESPACE

#endif // QQUICKPOINTERHANDLER_H
