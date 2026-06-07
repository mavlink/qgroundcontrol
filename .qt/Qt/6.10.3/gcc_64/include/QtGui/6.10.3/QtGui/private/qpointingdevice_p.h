// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPOINTINGDEVICE_P_H
#define QPOINTINGDEVICE_P_H

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

#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qevent_p.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/private/qinputdevice_p.h>

#include <QtCore/qpointer.h>
#include <QtCore/private/qflatmap_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcPointerGrab);

class Q_GUI_EXPORT QPointingDevicePrivate : public QInputDevicePrivate
{
    Q_DECLARE_PUBLIC(QPointingDevice)
public:
    QPointingDevicePrivate(const QString &name, qint64 id, QInputDevice::DeviceType type,
                           QPointingDevice::PointerType pType, QPointingDevice::Capabilities caps,
                           int maxPoints, int buttonCount,
                           const QString &seatName = QString(),
                           QPointingDeviceUniqueId uniqueId = QPointingDeviceUniqueId())
      : QInputDevicePrivate(name, id, type, caps, seatName),
        uniqueId(uniqueId),
        maximumTouchPoints(qint8(maxPoints)), buttonCount(qint8(buttonCount)),
        pointerType(pType)
    {
        pointingDeviceType = true;
        activePoints.reserve(maxPoints);
    }
    ~QPointingDevicePrivate() override;

    void sendTouchCancelEvent(QTouchEvent *cancelEvent);

    /*! \internal
        This struct (stored in activePoints) holds persistent state between event deliveries.
    */
    struct EventPointData {
        QEventPoint eventPoint;
        QPointer<QObject> exclusiveGrabber;
        QPointer<QObject> exclusiveGrabberContext;          // extra info about where the grab happened
        QList<QPointer <QObject> > passiveGrabbers;
        QList<QPointer <QObject> > passiveGrabbersContext;  // parallel list: extra info about where the grabs happened
    };
    EventPointData *queryPointById(int id) const;
    EventPointData *pointById(int id) const;
    void removePointById(int id);
    QObject *firstActiveTarget() const;
    QWindow *firstActiveWindow() const;

    QObject *firstPointExclusiveGrabber() const;
    void setExclusiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *exclusiveGrabber);
    bool removeExclusiveGrabber(const QPointerEvent *event, const QObject *grabber);
    bool addPassiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *grabber);
    static bool setPassiveGrabberContext(EventPointData *epd, QObject *grabber, QObject *context);
    bool removePassiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *grabber);
    void clearPassiveGrabbers(const QPointerEvent *event, const QEventPoint &point);
    void removeGrabber(QObject *grabber, bool cancel = false);

    using EventPointMap = QVarLengthFlatMap<int, EventPointData, 20>;
    mutable EventPointMap activePoints;

    QPointingDeviceUniqueId uniqueId;
    quint32 toolId = 0;         // only for Wacom tablets
    qint8 maximumTouchPoints = 0;
    qint8 buttonCount = 0;
    QPointingDevice::PointerType pointerType = QPointingDevice::PointerType::Unknown;
    bool toolProximity = false;  // only for Wacom tablets

    inline static QPointingDevicePrivate *get(QPointingDevice *q)
    {
        return static_cast<QPointingDevicePrivate *>(QObjectPrivate::get(q));
    }

    inline static const QPointingDevicePrivate *get(const QPointingDevice *q)
    {
        return static_cast<const QPointingDevicePrivate *>(QObjectPrivate::get(q));
    }

    static const QPointingDevice *tabletDevice(QInputDevice::DeviceType deviceType,
                                               QPointingDevice::PointerType pointerType,
                                               QPointingDeviceUniqueId uniqueId);

    static const QPointingDevice *queryTabletDevice(QInputDevice::DeviceType deviceType,
                                                    QPointingDevice::PointerType pointerType,
                                                    QPointingDeviceUniqueId uniqueId,
                                                    QInputDevice::Capabilities capabilities = QInputDevice::Capability::None,
                                                    qint64 systemId = 0);

    static const QPointingDevice *pointingDeviceById(qint64 systemId);
};

QT_END_NAMESPACE

#endif // QPOINTINGDEVICE_P_H
