// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSYSTEMINTERFACE_P_H
#define QWINDOWSYSTEMINTERFACE_P_H

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

#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qtguiglobal_p.h>
#include "qwindowsysteminterface.h"

#include <QElapsedTimer>
#include <QPointer>
#include <QMutex>
#include <QList>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQpaInputDevices, Q_GUI_EXPORT)

class QWindowSystemEventHandler;

class Q_GUI_EXPORT QWindowSystemInterfacePrivate {
public:
    enum EventType {
        UserInputEvent = 0x100,
        Close = UserInputEvent | 0x01,
        GeometryChange = 0x02,
        Enter = UserInputEvent | 0x03,
        Leave = UserInputEvent | 0x04,
        FocusWindow = 0x05,
        WindowStateChanged = 0x06,
        Mouse = UserInputEvent | 0x07,
        Wheel = UserInputEvent | 0x09,
        Key = UserInputEvent | 0x0a,
        Touch = UserInputEvent | 0x0b,
        ScreenOrientation = 0x0c,
        ScreenGeometry = 0x0d,
        ScreenAvailableGeometry = 0x0e,
        ScreenLogicalDotsPerInch = 0x0f,
        ScreenRefreshRate = 0x10,
        ThemeChange = 0x11,
        Expose = 0x12,
        FileOpen = UserInputEvent | 0x13,
        Tablet = UserInputEvent | 0x14,
        TabletEnterProximity = UserInputEvent | 0x15,
        TabletLeaveProximity = UserInputEvent | 0x16,
        PlatformPanel = UserInputEvent | 0x17,
        ContextMenu = UserInputEvent | 0x18,
        EnterWhatsThisMode = UserInputEvent | 0x19,
#ifndef QT_NO_GESTURES
        Gesture = UserInputEvent | 0x1a,
#endif
        ApplicationStateChanged = 0x19,
        FlushEvents = 0x20,
        WindowScreenChanged = 0x21,
        SafeAreaMarginsChanged = 0x22,
        ApplicationTermination = 0x23,
        Paint = 0x24,
        WindowDevicePixelRatioChanged = 0x25,
    };

    class WindowSystemEvent {
    public:
        enum {
            Synthetic = 0x1,
            NullWindow = 0x2
        };

        explicit WindowSystemEvent(EventType t)
            : type(t), flags(0), eventAccepted(true) { }
        virtual ~WindowSystemEvent() { }

        bool synthetic() const  { return flags & Synthetic; }
        bool nullWindow() const { return flags & NullWindow; }

        EventType type;
        int flags;
        bool eventAccepted;
    };

    class CloseEvent : public WindowSystemEvent {
    public:
        explicit CloseEvent(QWindow *w)
            : WindowSystemEvent(Close), window(w)
            { }
        QPointer<QWindow> window;
    };

    class GeometryChangeEvent : public WindowSystemEvent {
    public:
        GeometryChangeEvent(QWindow *window, QRect requestedGeometry, QRect newGeometry);
        QPointer<QWindow> window;
        QRect requestedGeometry;
        QRect newGeometry;
    };

    class EnterEvent : public WindowSystemEvent {
    public:
        explicit EnterEvent(QWindow *enter, const QPointF &local, const QPointF &global)
            : WindowSystemEvent(Enter), enter(enter), localPos(local), globalPos(global)
        { }
        QPointer<QWindow> enter;
        const QPointF localPos;
        const QPointF globalPos;
    };

    class LeaveEvent : public WindowSystemEvent {
    public:
        explicit LeaveEvent(QWindow *leave)
            : WindowSystemEvent(Leave), leave(leave)
        { }
        QPointer<QWindow> leave;
    };

    class FocusWindowEvent : public WindowSystemEvent {
    public:
        explicit FocusWindowEvent(QWindow *focusedWindow, Qt::FocusReason r)
            : WindowSystemEvent(FocusWindow), focused(focusedWindow), reason(r)
        { }
        QPointer<QWindow> focused;
        Qt::FocusReason reason;
    };

    class WindowStateChangedEvent : public WindowSystemEvent {
    public:
        WindowStateChangedEvent(QWindow *_window, Qt::WindowStates _newState, Qt::WindowStates _oldState)
            : WindowSystemEvent(WindowStateChanged), window(_window), newState(_newState), oldState(_oldState)
        { }

        QPointer<QWindow> window;
        Qt::WindowStates newState;
        Qt::WindowStates oldState;
    };

    class WindowScreenChangedEvent : public WindowSystemEvent {
    public:
        WindowScreenChangedEvent(QWindow *w, QScreen *s)
            : WindowSystemEvent(WindowScreenChanged), window(w), screen(s)
        { }

        QPointer<QWindow> window;
        QPointer<QScreen> screen;
    };

    class WindowDevicePixelRatioChangedEvent : public WindowSystemEvent {
    public:
        WindowDevicePixelRatioChangedEvent(QWindow *w)
            : WindowSystemEvent(WindowDevicePixelRatioChanged), window(w)
        { }

        QPointer<QWindow> window;
    };

    class SafeAreaMarginsChangedEvent : public WindowSystemEvent {
    public:
        SafeAreaMarginsChangedEvent(QWindow *w)
            : WindowSystemEvent(SafeAreaMarginsChanged), window(w)
        { }

        QPointer<QWindow> window;
    };

    class ApplicationStateChangedEvent : public WindowSystemEvent {
    public:
        ApplicationStateChangedEvent(Qt::ApplicationState newState, bool forcePropagate = false)
            : WindowSystemEvent(ApplicationStateChanged), newState(newState), forcePropagate(forcePropagate)
        { }

        Qt::ApplicationState newState;
        bool forcePropagate;
    };

    class FlushEventsEvent : public WindowSystemEvent {
    public:
        FlushEventsEvent(QEventLoop::ProcessEventsFlags f = QEventLoop::AllEvents)
            : WindowSystemEvent(FlushEvents)
            , flags(f)
        { }
        QEventLoop::ProcessEventsFlags flags;
    };

    class UserEvent : public WindowSystemEvent {
    public:
        UserEvent(QWindow * w, ulong time, EventType t)
            : WindowSystemEvent(t), window(w), timestamp(time)
        {
            if (!w)
                flags |= NullWindow;
        }
        QPointer<QWindow> window;
        unsigned long timestamp;
    };

    class InputEvent: public UserEvent {
    public:
        InputEvent(QWindow *w, ulong time, EventType t, Qt::KeyboardModifiers mods, const QInputDevice *dev)
            : UserEvent(w, time, t), modifiers(mods), device(dev) {}
        Qt::KeyboardModifiers modifiers;
        const QInputDevice *device;
    };

    class PointerEvent : public InputEvent {
    public:
        PointerEvent(QWindow * w, ulong time, EventType t, Qt::KeyboardModifiers mods, const QPointingDevice *device)
            : InputEvent(w, time, t, mods, device) {}
    };

    class MouseEvent : public PointerEvent {
    public:
        MouseEvent(QWindow *w, ulong time, const QPointF &local, const QPointF &global,
                   Qt::MouseButtons state, Qt::KeyboardModifiers mods,
                   Qt::MouseButton b, QEvent::Type type,
                   Qt::MouseEventSource src = Qt::MouseEventNotSynthesized, bool frame = false,
                   const QPointingDevice *device = QPointingDevice::primaryPointingDevice(),
                   int evPtId = -1)
            : PointerEvent(w, time, Mouse, mods, device), localPos(local), globalPos(global),
              buttons(state), source(src), nonClientArea(frame), button(b), buttonType(type),
              eventPointId(evPtId) { }

        QPointF localPos;
        QPointF globalPos;
        Qt::MouseButtons buttons;
        Qt::MouseEventSource source;
        bool nonClientArea;
        Qt::MouseButton button;
        QEvent::Type buttonType;
        int eventPointId; // from the original device if synth-mouse, otherwise -1
    };

    class WheelEvent : public PointerEvent {
    public:
        WheelEvent(QWindow *w, ulong time, const QPointF &local, const QPointF &global, QPoint pixelD, QPoint angleD, int qt4D, Qt::Orientation qt4O,
                   Qt::KeyboardModifiers mods, Qt::ScrollPhase phase = Qt::NoScrollPhase, Qt::MouseEventSource src = Qt::MouseEventNotSynthesized,
                   bool inverted = false, const QPointingDevice *device = QPointingDevice::primaryPointingDevice())
            : PointerEvent(w, time, Wheel, mods, device), pixelDelta(pixelD), angleDelta(angleD), qt4Delta(qt4D),
              qt4Orientation(qt4O), localPos(local), globalPos(global), phase(phase), source(src), inverted(inverted) { }
        QPoint pixelDelta;
        QPoint angleDelta;
        int qt4Delta;
        Qt::Orientation qt4Orientation;
        QPointF localPos;
        QPointF globalPos;
        Qt::ScrollPhase phase;
        Qt::MouseEventSource source;
        bool inverted;
    };

    class KeyEvent : public InputEvent {
    public:
        KeyEvent(QWindow *w, ulong time, QEvent::Type t, int k, Qt::KeyboardModifiers mods,
                 const QString & text = QString(), bool autorep = false, ushort count = 1,
                 const QInputDevice *device = QInputDevice::primaryKeyboard())
            : InputEvent(w, time, Key, mods, device), source(nullptr), key(k), unicode(text),
             repeat(autorep), repeatCount(count), keyType(t),
             nativeScanCode(0), nativeVirtualKey(0), nativeModifiers(0) { }
        KeyEvent(QWindow *w, ulong time, QEvent::Type t, int k, Qt::KeyboardModifiers mods,
                 quint32 nativeSC, quint32 nativeVK, quint32 nativeMods,
                 const QString & text = QString(), bool autorep = false, ushort count = 1,
                 const QInputDevice *device = QInputDevice::primaryKeyboard())
            : InputEvent(w, time, Key, mods, device), source(nullptr), key(k), unicode(text),
             repeat(autorep), repeatCount(count), keyType(t),
             nativeScanCode(nativeSC), nativeVirtualKey(nativeVK), nativeModifiers(nativeMods) { }
        const QInputDevice *source;
        int key;
        QString unicode;
        bool repeat;
        ushort repeatCount;
        QEvent::Type keyType;
        quint32 nativeScanCode;
        quint32 nativeVirtualKey;
        quint32 nativeModifiers;
    };

    class TouchEvent : public PointerEvent {
    public:
        TouchEvent(QWindow *w, ulong time, QEvent::Type t, const QPointingDevice *device,
                   const QList<QEventPoint> &p, Qt::KeyboardModifiers mods)
            : PointerEvent(w, time, Touch, mods, device), points(p), touchType(t) { }
        QList<QEventPoint> points;
        QEvent::Type touchType;
    };

    class ScreenOrientationEvent : public WindowSystemEvent {
    public:
        ScreenOrientationEvent(QScreen *s, Qt::ScreenOrientation o)
            : WindowSystemEvent(ScreenOrientation), screen(s), orientation(o) { }
        QPointer<QScreen> screen;
        Qt::ScreenOrientation orientation;
    };

    class ScreenGeometryEvent : public WindowSystemEvent {
    public:
        ScreenGeometryEvent(QScreen *s, const QRect &g, const QRect &ag)
            : WindowSystemEvent(ScreenGeometry), screen(s), geometry(g), availableGeometry(ag) { }
        QPointer<QScreen> screen;
        QRect geometry;
        QRect availableGeometry;
    };

    class ScreenLogicalDotsPerInchEvent : public WindowSystemEvent {
    public:
        ScreenLogicalDotsPerInchEvent(QScreen *s, qreal dx, qreal dy)
            : WindowSystemEvent(ScreenLogicalDotsPerInch), screen(s), dpiX(dx), dpiY(dy) { }
        QPointer<QScreen> screen;
        qreal dpiX;
        qreal dpiY;
    };

    class ScreenRefreshRateEvent : public WindowSystemEvent {
    public:
        ScreenRefreshRateEvent(QScreen *s, qreal r)
            : WindowSystemEvent(ScreenRefreshRate), screen(s), rate(r) { }
        QPointer<QScreen> screen;
        qreal rate;
    };

    class ThemeChangeEvent : public WindowSystemEvent {
    public:
        explicit ThemeChangeEvent()
            : WindowSystemEvent(ThemeChange) { }
    };

    class ExposeEvent : public WindowSystemEvent {
    public:
        ExposeEvent(QWindow *window, const QRegion &region);
        QPointer<QWindow> window;
        bool isExposed;
        QRegion region;
    };

    class PaintEvent : public WindowSystemEvent {
    public:
        PaintEvent(QWindow *window, const QRegion &region)
            :  WindowSystemEvent(Paint), window(window), region(region) {}
        QPointer<QWindow> window;
        QRegion region;
    };

    class FileOpenEvent : public WindowSystemEvent {
    public:
        FileOpenEvent(const QString& fileName)
            : WindowSystemEvent(FileOpen), url(QUrl::fromLocalFile(fileName))
        { }
        FileOpenEvent(const QUrl &url)
            : WindowSystemEvent(FileOpen), url(url)
        { }
        QUrl url;
    };

    class Q_GUI_EXPORT TabletEvent : public PointerEvent {
    public:
        // TODO take QPointingDevice* instead of types and IDs
        static void handleTabletEvent(QWindow *w, const QPointF &local, const QPointF &global,
                                      int device, int pointerType, Qt::MouseButtons buttons, qreal pressure, qreal xTilt, qreal yTilt,
                                      qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                      Qt::KeyboardModifiers modifiers = Qt::NoModifier);
        static void setPlatformSynthesizesMouse(bool v);

        TabletEvent(QWindow *w, ulong time, const QPointF &local, const QPointF &global,
                    const QPointingDevice *device, Qt::MouseButtons b, qreal pressure, qreal xTilt, qreal yTilt, qreal tpressure,
                    qreal rotation, int z, Qt::KeyboardModifiers mods)
            : PointerEvent(w, time, Tablet, mods, device),
              buttons(b), local(local), global(global),
              pressure(pressure), xTilt(xTilt), yTilt(yTilt), tangentialPressure(tpressure),
              rotation(rotation), z(z) { }
        Qt::MouseButtons buttons;
        QPointF local;
        QPointF global;
        qreal pressure;
        qreal xTilt;
        qreal yTilt;
        qreal tangentialPressure;
        qreal rotation;
        int z;
        static bool platformSynthesizesMouse;
    };

    class TabletEnterProximityEvent : public PointerEvent {
    public:
        // TODO store more info: position and whatever else we can get on most platforms
        TabletEnterProximityEvent(ulong time, const QPointingDevice *device)
            : PointerEvent(nullptr, time, TabletEnterProximity, Qt::NoModifier, device) { }
    };

    class TabletLeaveProximityEvent : public PointerEvent {
    public:
        // TODO store more info: position and whatever else we can get on most platforms
        TabletLeaveProximityEvent(ulong time, const QPointingDevice *device)
            : PointerEvent(nullptr, time, TabletLeaveProximity, Qt::NoModifier, device) { }
    };

    class PlatformPanelEvent : public WindowSystemEvent {
    public:
        explicit PlatformPanelEvent(QWindow *w)
            : WindowSystemEvent(PlatformPanel), window(w) { }
        QPointer<QWindow> window;
    };

#ifndef QT_NO_CONTEXTMENU
    class ContextMenuEvent : public WindowSystemEvent {
    public:
        explicit ContextMenuEvent(QWindow *w, bool mouseTriggered, const QPoint &pos,
                                  const QPoint &globalPos, Qt::KeyboardModifiers modifiers)
            : WindowSystemEvent(ContextMenu), window(w), mouseTriggered(mouseTriggered), pos(pos),
              globalPos(globalPos), modifiers(modifiers) { }
        QPointer<QWindow> window;
        bool mouseTriggered;
        QPoint pos;       // Only valid if triggered by mouse
        QPoint globalPos; // Only valid if triggered by mouse
        Qt::KeyboardModifiers modifiers;
    };
#endif

#ifndef QT_NO_GESTURES
    class GestureEvent : public PointerEvent {
    public:
        GestureEvent(QWindow *window, ulong time, Qt::NativeGestureType type, const QPointingDevice *dev,
                     int fingerCount, QPointF pos, QPointF globalPos, qreal realValue, QPointF delta)
            : PointerEvent(window, time, Gesture, Qt::NoModifier, dev), type(type), pos(pos), globalPos(globalPos),
              delta(delta), fingerCount(fingerCount), realValue(realValue), sequenceId(0), intValue(0) { }
        Qt::NativeGestureType type;
        QPointF pos;
        QPointF globalPos;
        QPointF delta;
        int fingerCount;
        // Mac
        qreal realValue;
        // Windows
        ulong sequenceId;
        quint64 intValue;
    };
#endif

    class WindowSystemEventList {
        QList<WindowSystemEvent *> impl;
        mutable QMutex mutex;
    public:
        WindowSystemEventList() : impl(), mutex() {}
        ~WindowSystemEventList() { clear(); }

        void clear()
        { const QMutexLocker locker(&mutex); qDeleteAll(impl); impl.clear(); }
        void prepend(WindowSystemEvent *e)
        { const QMutexLocker locker(&mutex); impl.prepend(e); }
        WindowSystemEvent *takeFirstOrReturnNull()
        { const QMutexLocker locker(&mutex); return impl.empty() ? nullptr : impl.takeFirst(); }
        WindowSystemEvent *takeFirstNonUserInputOrReturnNull()
        {
            const QMutexLocker locker(&mutex);
            for (int i = 0; i < impl.size(); ++i)
                if (!(impl.at(i)->type & QWindowSystemInterfacePrivate::UserInputEvent))
                    return impl.takeAt(i);
            return nullptr;
        }
        bool nonUserInputEventsQueued()
        {
            const QMutexLocker locker(&mutex);
            for (int i = 0; i < impl.size(); ++i)
                if (!(impl.at(i)->type & QWindowSystemInterfacePrivate::UserInputEvent))
                    return true;
            return false;
        }
        void append(WindowSystemEvent *e)
        { const QMutexLocker locker(&mutex); impl.append(e); }
        qsizetype count() const
        { const QMutexLocker locker(&mutex); return impl.size(); }
        WindowSystemEvent *peekAtFirstOfType(EventType t) const
        {
            const QMutexLocker locker(&mutex);
            for (int i = 0; i < impl.size(); ++i) {
                if (impl.at(i)->type == t)
                    return impl.at(i);
            }
            return nullptr;
        }
        void remove(const WindowSystemEvent *e)
        {
            const QMutexLocker locker(&mutex);
            for (int i = 0; i < impl.size(); ++i) {
                if (impl.at(i) == e) {
                    delete impl.takeAt(i);
                    break;
                }
            }
        }
    private:
        Q_DISABLE_COPY_MOVE(WindowSystemEventList)
    };

    static WindowSystemEventList windowSystemEventQueue;

    static qsizetype windowSystemEventsQueued();
    static bool nonUserInputEventsQueued();
    static WindowSystemEvent *getWindowSystemEvent();
    static WindowSystemEvent *getNonUserInputWindowSystemEvent();
    static WindowSystemEvent *peekWindowSystemEvent(EventType t);
    static void removeWindowSystemEvent(WindowSystemEvent *event);

public:
    static QElapsedTimer eventTime;
    static bool synchronousWindowSystemEvents;
    static bool platformFiltersEvents;

    static QWaitCondition eventsFlushed;
    static QMutex flushEventMutex;
    static QAtomicInt eventAccepted;

    static QList<QEventPoint>
        fromNativeTouchPoints(const QList<QWindowSystemInterface::TouchPoint> &points,
                              const QWindow *window, QEvent::Type *type = nullptr);
    template<class EventPointList>
    static QList<QWindowSystemInterface::TouchPoint>
        toNativeTouchPoints(const EventPointList &pointList, const QWindow *window)
    {
        QList<QWindowSystemInterface::TouchPoint> newList;
        newList.reserve(pointList.size());
        for (const auto &point : pointList) {
            newList.append(toNativeTouchPoint(point, window));
        }
        return newList;
    }
    static QWindowSystemInterface::TouchPoint
        toNativeTouchPoint(const QEventPoint &point, const QWindow *window);

    static void installWindowSystemEventHandler(QWindowSystemEventHandler *handler);
    static void removeWindowSystemEventhandler(QWindowSystemEventHandler *handler);
    static QWindowSystemEventHandler *eventHandler;
};

class Q_GUI_EXPORT QWindowSystemEventHandler
{
public:
    virtual ~QWindowSystemEventHandler();
    virtual bool sendEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *event);
};

QT_END_NAMESPACE

#endif // QWINDOWSYSTEMINTERFACE_P_H
