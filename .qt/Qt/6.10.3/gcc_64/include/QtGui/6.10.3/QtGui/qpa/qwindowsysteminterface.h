// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSYSTEMINTERFACE_H
#define QWINDOWSYSTEMINTERFACE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/QTime>
#include <QtGui/qwindowdefs.h>
#include <QtCore/QEvent>
#include <QtCore/QAbstractEventDispatcher>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtCore/QMutex>
#include <QtGui/QTouchEvent>
#include <QtCore/QEventLoop>
#include <QtGui/QVector2D>

QT_BEGIN_NAMESPACE

class QMimeData;
class QPointingDevice;
class QPlatformDragQtResponse;
class QPlatformDropQtResponse;


class Q_GUI_EXPORT QWindowSystemInterface
{
public:
    struct SynchronousDelivery {};
    struct AsynchronousDelivery {};
    struct DefaultDelivery {};

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleMouseEvent(QWindow *window, const QPointF &local, const QPointF &global,
                                 Qt::MouseButtons state, Qt::MouseButton button, QEvent::Type type,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleMouseEvent(QWindow *window,  const QPointingDevice *device,
                                 const QPointF &local, const QPointF &global,
                                 Qt::MouseButtons state, Qt::MouseButton button, QEvent::Type type,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleMouseEvent(QWindow *window, ulong timestamp, const QPointF &local,
                                 const QPointF &global, Qt::MouseButtons state,
                                 Qt::MouseButton button, QEvent::Type type,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleMouseEvent(QWindow *window, ulong timestamp, const QPointingDevice *device,
                                 const QPointF &local, const QPointF &global, Qt::MouseButtons state,
                                 Qt::MouseButton button, QEvent::Type type,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    static bool handleShortcutEvent(QWindow *window, ulong timestamp, int k, Qt::KeyboardModifiers mods, quint32 nativeScanCode,
                                      quint32 nativeVirtualKey, quint32 nativeModifiers, const QString & text = QString(), bool autorep = false, ushort count = 1);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleKeyEvent(QWindow *window, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleKeyEvent(QWindow *window, ulong timestamp, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1);

    static bool handleExtendedKeyEvent(QWindow *window, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                       quint32 nativeScanCode, quint32 nativeVirtualKey,
                                       quint32 nativeModifiers,
                                       const QString& text = QString(), bool autorep = false,
                                       ushort count = 1);
    static bool handleExtendedKeyEvent(QWindow *window, ulong timestamp, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                       quint32 nativeScanCode, quint32 nativeVirtualKey,
                                       quint32 nativeModifiers,
                                       const QString& text = QString(), bool autorep = false,
                                       ushort count = 1);
    static bool handleExtendedKeyEvent(QWindow *window, ulong timestamp, const QInputDevice *device,
                                       QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                       quint32 nativeScanCode, quint32 nativeVirtualKey,
                                       quint32 nativeModifiers,
                                       const QString& text = QString(), bool autorep = false,
                                       ushort count = 1);
    static bool handleWheelEvent(QWindow *window, const QPointF &local, const QPointF &global,
                                 QPoint pixelDelta, QPoint angleDelta,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::ScrollPhase phase = Qt::NoScrollPhase,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);
    static bool handleWheelEvent(QWindow *window, ulong timestamp, const QPointF &local, const QPointF &global,
                                 QPoint pixelDelta, QPoint angleDelta,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::ScrollPhase phase = Qt::NoScrollPhase,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized,
                                 bool inverted = false);
    static bool handleWheelEvent(QWindow *window, ulong timestamp, const QPointingDevice *device,
                                 const QPointF &local, const QPointF &global,
                                 QPoint pixelDelta, QPoint angleDelta,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::ScrollPhase phase = Qt::NoScrollPhase,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized,
                                 bool inverted = false);

    // A very-temporary QPA touchpoint which gets converted to a QEventPoint as early as possible
    // in QWindowSystemInterfacePrivate::fromNativeTouchPoints()
    struct TouchPoint {
        TouchPoint() : id(0), uniqueId(-1), pressure(0), rotation(0), state(QEventPoint::State::Stationary) { }
        int id;                 // for application use
        qint64 uniqueId;        // for TUIO: object/token ID; otherwise empty
                                // TODO for TUIO 2.0: add registerPointerUniqueID(QPointingDeviceUniqueId)
        QPointF normalPosition; // touch device coordinates, (0 to 1, 0 to 1)
        QRectF area;            // dimensions of the elliptical contact patch, unrotated, and centered at position in screen coordinates
                                // width is the horizontal diameter, height is the vertical diameter
        qreal pressure;         // 0 to 1
        qreal rotation;         // rotation applied to the elliptical contact patch
                                // 0 means pointing straight up; 0 if unknown (like QTabletEvent::rotation)
        QEventPoint::State state; // Pressed|Updated|Stationary|Released
        QVector2D velocity;     // in screen coordinate system, pixels / seconds
        QList<QPointF> rawPositions; // in screen coordinates
    };

    static void registerInputDevice(const QInputDevice *device);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleTouchEvent(QWindow *window, const QPointingDevice *device,
                                 const QList<struct TouchPoint> &points, Qt::KeyboardModifiers mods = Qt::NoModifier);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleTouchEvent(QWindow *window, ulong timestamp, const QPointingDevice *device,
                                 const QList<struct TouchPoint> &points, Qt::KeyboardModifiers mods = Qt::NoModifier);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleTouchCancelEvent(QWindow *window, const QPointingDevice *device, Qt::KeyboardModifiers mods = Qt::NoModifier);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleTouchCancelEvent(QWindow *window, ulong timestamp, const QPointingDevice *device, Qt::KeyboardModifiers mods = Qt::NoModifier);

    // rect is relative to parent
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleGeometryChange(QWindow *window, const QRect &newRect);

    // region is in local coordinates, do not confuse with geometry which is parent-relative
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleExposeEvent(QWindow *window, const QRegion &region);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handlePaintEvent(QWindow *window, const QRegion &region);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleCloseEvent(QWindow *window);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleEnterEvent(QWindow *window, const QPointF &local = QPointF(), const QPointF& global = QPointF());
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleLeaveEvent(QWindow *window);
    static void handleEnterLeaveEvent(QWindow *enter, QWindow *leave, const QPointF &local = QPointF(), const QPointF& global = QPointF());

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleFocusWindowChanged(QWindow *window, Qt::FocusReason r = Qt::OtherFocusReason);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleWindowStateChanged(QWindow *window, Qt::WindowStates newState, int oldState = -1);
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleWindowScreenChanged(QWindow *window, QScreen *newScreen);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleWindowDevicePixelRatioChanged(QWindow *window);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleSafeAreaMarginsChanged(QWindow *window);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleApplicationStateChanged(Qt::ApplicationState newState, bool forcePropagate = false);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleApplicationTermination();

#if QT_CONFIG(draganddrop)
    static QPlatformDragQtResponse handleDrag(QWindow *window, const QMimeData *dropData,
                                              const QPoint &p, Qt::DropActions supportedActions,
                                              Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    static QPlatformDropQtResponse handleDrop(QWindow *window, const QMimeData *dropData,
                                              const QPoint &p, Qt::DropActions supportedActions,
                                              Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
#endif // QT_CONFIG(draganddrop)

    static bool handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, qintptr *result);

    // Changes to the screen
    static void handleScreenAdded(QPlatformScreen *screen, bool isPrimary = false);
    static void handleScreenRemoved(QPlatformScreen *screen);
    static void handlePrimaryScreenChanged(QPlatformScreen *newPrimary);

    static void handleScreenOrientationChange(QScreen *screen, Qt::ScreenOrientation newOrientation);
    static void handleScreenGeometryChange(QScreen *screen, const QRect &newGeometry, const QRect &newAvailableGeometry);
    static void handleScreenLogicalDotsPerInchChange(QScreen *screen, qreal newDpiX, qreal newDpiY);
    static void handleScreenRefreshRateChange(QScreen *screen, qreal newRefreshRate);

    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static void handleThemeChange();

    static void handleFileOpenEvent(const QString& fileName);
    static void handleFileOpenEvent(const QUrl &url);

    static bool handleTabletEvent(QWindow *window, ulong timestamp, const QPointingDevice *device,
                                  const QPointF &local, const QPointF &global,
                                  Qt::MouseButtons buttons, qreal pressure, qreal xTilt, qreal yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    static bool handleTabletEvent(QWindow *window, const QPointingDevice *device,
                                  const QPointF &local, const QPointF &global,
                                  Qt::MouseButtons buttons, qreal pressure, qreal xTilt, qreal yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    static bool handleTabletEvent(QWindow *window, ulong timestamp, const QPointF &local, const QPointF &global,
                                  int device, int pointerType, Qt::MouseButtons buttons, qreal pressure, qreal xTilt, qreal yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                  Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    static bool handleTabletEvent(QWindow *window, const QPointF &local, const QPointF &global,
                                  int device, int pointerType, Qt::MouseButtons buttons, qreal pressure, qreal xTilt, qreal yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                  Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    static bool handleTabletEnterLeaveProximityEvent(QWindow *window, ulong timestamp, const QPointingDevice *device,
                                                     bool inProximity, const QPointF &local = QPointF(), const QPointF &global = QPointF(),
                                                     Qt::MouseButtons buttons = {}, qreal xTilt = 0, qreal yTilt = 0,
                                                     qreal tangentialPressure = 0, qreal rotation = 0, int z = 0,
                                                     Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    static bool handleTabletEnterLeaveProximityEvent(QWindow *window, const QPointingDevice *device,
                                                     bool inProximity, const QPointF &local = QPointF(), const QPointF &global = QPointF(),
                                                     Qt::MouseButtons buttons = {}, qreal xTilt = 0 , qreal yTilt = 0,
                                                     qreal tangentialPressure = 0, qreal rotation = 0, int z = 0,
                                                     Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    // The following 4 functions are deprecated (QTBUG-114560)
    static bool handleTabletEnterProximityEvent(ulong timestamp, int deviceType, int pointerType, qint64 uid);
    static void handleTabletEnterProximityEvent(int deviceType, int pointerType, qint64 uid);
    static bool handleTabletLeaveProximityEvent(ulong timestamp, int deviceType, int pointerType, qint64 uid);
    static void handleTabletLeaveProximityEvent(int deviceType, int pointerType, qint64 uid);

#ifndef QT_NO_GESTURES
    static bool handleGestureEvent(QWindow *window, ulong timestamp, const QPointingDevice *device,  Qt::NativeGestureType type,
                                   const QPointF &local, const QPointF &global, int fingerCount = 0);
    static bool handleGestureEventWithRealValue(QWindow *window, ulong timestamp, const QPointingDevice *device, Qt::NativeGestureType type,
                                                qreal value, const QPointF &local, const QPointF &global, int fingerCount = 2);
    static bool handleGestureEventWithValueAndDelta(QWindow *window, ulong timestamp, const QPointingDevice *device, Qt::NativeGestureType type, qreal value,
                                                    const QPointF &delta, const QPointF &local, const QPointF &global, int fingerCount = 2);
#endif // QT_NO_GESTURES

    static void handlePlatformPanelEvent(QWindow *window);

#ifndef QT_NO_CONTEXTMENU
#if QT_GUI_REMOVED_SINCE(6, 8)
    static void handleContextMenuEvent(QWindow *window, bool mouseTriggered,
                                       const QPoint &pos, const QPoint &globalPos,
                                       Qt::KeyboardModifiers modifiers);
#endif
    template<typename Delivery = QWindowSystemInterface::DefaultDelivery>
    static bool handleContextMenuEvent(QWindow *window, bool mouseTriggered,
                                       const QPoint &pos, const QPoint &globalPos,
                                       Qt::KeyboardModifiers modifiers);
#endif
#if QT_CONFIG(whatsthis)
    static void handleEnterWhatsThisEvent();
#endif

    // For event dispatcher implementations
    static bool sendWindowSystemEvents(QEventLoop::ProcessEventsFlags flags);
    static void setSynchronousWindowSystemEvents(bool enable);
    static bool flushWindowSystemEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    static void deferredFlushWindowSystemEvents(QEventLoop::ProcessEventsFlags flags);
    static int windowSystemEventsQueued();
    static bool nonUserInputEventsQueued();
};

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QWindowSystemInterface::TouchPoint &p);
#endif

QT_END_NAMESPACE

#endif // QWINDOWSYSTEMINTERFACE_H
