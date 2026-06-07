// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Jie Liu <liujie01@kylinos.cn>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDINPUTDEVICE_H
#define QWAYLANDINPUTDEVICE_H

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

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include <QtCore/QScopedPointer>
#include <QSocketNotifier>
#include <QObject>
#include <QTimer>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qwayland-pointer-gestures-unstable-v1.h>

#if QT_CONFIG(xkbcommon)
#include <QtGui/private/qxkbcommon_p.h>
#endif

#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>

#if QT_CONFIG(cursor)
struct wl_cursor_image;
#endif

QT_BEGIN_NAMESPACE

namespace QtWayland {
class zwp_primary_selection_device_v1;
} //namespace QtWayland

namespace QtWaylandClient {

class QWaylandDataDevice;
class QWaylandDisplay;
#if QT_CONFIG(clipboard)
class QWaylandDataControlDeviceV1;
#endif
#if QT_CONFIG(wayland_client_primary_selection)
class QWaylandPrimarySelectionDeviceV1;
#endif
#if QT_CONFIG(tabletevent)
class QWaylandTabletSeatV2;
#endif
class QWaylandPointerGestures;
class QWaylandPointerGestureSwipe;
class QWaylandPointerGesturePinch;
class QWaylandTextInputInterface;
class QWaylandTextInputMethod;
#if QT_CONFIG(cursor)
class QWaylandCursorTheme;
class QWaylandCursorShape;
template <typename T>
class CursorSurface;
#endif

Q_DECLARE_LOGGING_CATEGORY(lcQpaWaylandInput);

class Q_WAYLANDCLIENT_EXPORT QWaylandInputDevice
                            : public QObject
                            , public QtWayland::wl_seat
{
    Q_OBJECT
public:
    class Keyboard;
    class Pointer;
    class Touch;

    QWaylandInputDevice(QWaylandDisplay *display, int version, uint32_t id);
    ~QWaylandInputDevice() override;

    uint32_t id() const { return mId; }
    uint32_t capabilities() const { return mCaps; }
    QString seatname()  const { return mSeatName; }

    QWaylandDisplay *display() const { return mQDisplay; }
    struct ::wl_seat *wl_seat() { return QtWayland::wl_seat::object(); }

#if QT_CONFIG(cursor)
    void setCursor(const QCursor *cursor, const QSharedPointer<QWaylandBuffer> &cachedBuffer = {}, int fallbackOutputScale = 1);
#endif
    void handleStartDrag();
    void handleEndDrag();

#if QT_CONFIG(wayland_datadevice)
    void setDataDevice(QWaylandDataDevice *device);
    QWaylandDataDevice *dataDevice() const;
#endif

#if QT_CONFIG(clipboard)
    void setDataControlDevice(QWaylandDataControlDeviceV1 *dataControlDevice);
    QWaylandDataControlDeviceV1 *dataControlDevice() const;
#endif

#if QT_CONFIG(wayland_client_primary_selection)
    void setPrimarySelectionDevice(QWaylandPrimarySelectionDeviceV1 *primarySelectionDevice);
    QWaylandPrimarySelectionDeviceV1 *primarySelectionDevice() const;
#endif

#if QT_CONFIG(tabletevent)
    void setTabletSeat(QWaylandTabletSeatV2 *tabletSeat);
    QWaylandTabletSeatV2* tabletSeat() const;
#endif

    void setTextInput(QWaylandTextInputInterface *textInput);
    QWaylandTextInputInterface *textInput() const;

    void setTextInputMethod(QWaylandTextInputMethod *textInputMethod);
    QWaylandTextInputMethod *textInputMethod() const;

    void removeMouseButtonFromState(Qt::MouseButton button);

    QWaylandWindow *pointerFocus() const;
    QWaylandWindow *keyboardFocus() const;
    QWaylandWindow *touchFocus() const;

    QList<int> possibleKeys(const QKeyEvent *event) const;

    QPointF pointerSurfacePosition() const;

    Qt::KeyboardModifiers modifiers() const;

    uint32_t serial() const;

    virtual Keyboard *createKeyboard(QWaylandInputDevice *device);
    virtual Pointer *createPointer(QWaylandInputDevice *device);
    virtual Touch *createTouch(QWaylandInputDevice *device);

    Keyboard *keyboard() const;
    Pointer *pointer() const;
    QWaylandPointerGestureSwipe *pointerGestureSwipe() const;
    QWaylandPointerGesturePinch *pointerGesturePinch() const;
    Touch *touch() const;

protected:
    QWaylandDisplay *mQDisplay = nullptr;
    struct wl_display *mDisplay = nullptr;

    uint32_t mId = -1;
    uint32_t mCaps = 0;
    QString mSeatName;
    bool mSeatNameKnown = false;

#if QT_CONFIG(cursor)
    struct CursorState {
        QSharedPointer<QWaylandBuffer> bitmapBuffer; // not used with shape cursors
        int bitmapScale = 1;
        Qt::CursorShape shape = Qt::ArrowCursor;
        int fallbackOutputScale = 1;
        QPoint hotspot;
        QElapsedTimer animationTimer;
    } mCursor;
#endif

#if QT_CONFIG(wayland_datadevice)
    QWaylandDataDevice *mDataDevice = nullptr;
#endif

#if QT_CONFIG(clipboard)
    QScopedPointer<QWaylandDataControlDeviceV1> mDataControlDevice;
#endif

#if QT_CONFIG(wayland_client_primary_selection)
    QScopedPointer<QWaylandPrimarySelectionDeviceV1> mPrimarySelectionDevice;
#endif

    QScopedPointer<Keyboard> mKeyboard;
    QScopedPointer<Pointer> mPointer;
    QScopedPointer<QWaylandPointerGestureSwipe> mPointerGestureSwipe;
    QScopedPointer<QWaylandPointerGesturePinch> mPointerGesturePinch;
    QScopedPointer<Touch> mTouch;

    QScopedPointer<QWaylandTextInputInterface> mTextInput;
    QScopedPointer<QWaylandTextInputMethod> mTextInputMethod;
#if QT_CONFIG(tabletevent)
    QScopedPointer<QWaylandTabletSeatV2> mTabletSeat;
#endif

    uint32_t mTime = 0;
    uint32_t mSerial = 0;

    void seat_capabilities(uint32_t caps) override;
    void seat_name(const QString &name) override;
    void maybeRegisterInputDevices();
    void handleTouchPoint(int id, QEventPoint::State state, const QPointF &surfacePosition = QPoint());

    QPointingDevice *mTouchDevice = nullptr;
    QPointingDevice *mTouchPadDevice = nullptr;

    friend class QWaylandTouchExtension;
    friend class QWaylandQtKeyExtension;
    friend class QWaylandPointerGestureSwipe;
    friend class QWaylandPointerGesturePinch;
    friend class QWaylandWindow;
    friend class QWaylandTabletToolV2;
};

inline uint32_t QWaylandInputDevice::serial() const
{
    return mSerial;
}


class Q_WAYLANDCLIENT_EXPORT QWaylandInputDevice::Keyboard : public QObject, public QtWayland::wl_keyboard
{
    Q_OBJECT

public:
    Keyboard(QWaylandInputDevice *p);
    ~Keyboard() override;

    QWaylandWindow *focusWindow() const;

    void keyboard_keymap(uint32_t format,
                         int32_t fd,
                         uint32_t size) override;
    void keyboard_enter(uint32_t time,
                        struct wl_surface *surface,
                        struct wl_array *keys) override;
    void keyboard_leave(uint32_t time,
                        struct wl_surface *surface) override;
    void keyboard_key(uint32_t serial, uint32_t time,
                      uint32_t key, uint32_t state) override;
    void keyboard_modifiers(uint32_t serial,
                            uint32_t mods_depressed,
                            uint32_t mods_latched,
                            uint32_t mods_locked,
                            uint32_t group) override;
    void keyboard_repeat_info(int32_t rate, int32_t delay) override;

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandSurface> mFocus;

    uint32_t mNativeModifiers = 0;

    struct repeatKey {
        int key = 0;
        uint32_t code = 0;
        uint32_t time = 0 ;
        QString text;
        uint32_t nativeVirtualKey = 0;
    } mRepeatKey;

    QTimer mRepeatTimer;
    int mRepeatRate = 25;
    int mRepeatDelay = 400;

    uint32_t mKeymapFormat = WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1;

    Qt::KeyboardModifiers modifiers() const;

    struct ::wl_keyboard *wl_keyboard() { return QtWayland::wl_keyboard::object(); }

#if QT_CONFIG(xkbcommon)
    virtual int keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers, xkb_state *state, xkb_keycode_t code) {
        return QXkbCommon::keysymToQtKey(keysym, modifiers, state, code);
    }
#endif

private Q_SLOTS:
    void handleFocusDestroyed();
    void handleFocusLost();

private:
#if QT_CONFIG(xkbcommon)
    bool createDefaultKeymap();
#endif
    void handleKey(ulong timestamp, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                   quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                   const QString &text, bool autorepeat = false, ushort count = 1);

#if QT_CONFIG(xkbcommon)
    QXkbCommon::ScopedXKBKeymap mXkbKeymap;
    QXkbCommon::ScopedXKBState mXkbState;
#endif
    friend class QWaylandInputDevice;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandInputDevice::Pointer : public QObject, public QtWayland::wl_pointer
{
    Q_OBJECT
public:
    explicit Pointer(QWaylandInputDevice *seat);
    ~Pointer() override;
    QWaylandWindow *focusWindow() const;
#if QT_CONFIG(cursor)
    int idealCursorScale() const;
    void updateCursorTheme();
    void updateCursor();
    void cursorTimerCallback();
    void cursorFrameCallback();
    CursorSurface<QWaylandInputDevice::Pointer> *getOrCreateCursorSurface();
#endif
    QWaylandInputDevice *seat() const { return mParent; }

    struct ::wl_pointer *wl_pointer() { return QtWayland::wl_pointer::object(); }

protected:
    void pointer_enter(uint32_t serial, struct wl_surface *surface,
                       wl_fixed_t sx, wl_fixed_t sy) override;
    void pointer_leave(uint32_t time, struct wl_surface *surface) override;
    void pointer_motion(uint32_t time,
                        wl_fixed_t sx, wl_fixed_t sy) override;
    void pointer_button(uint32_t serial, uint32_t time,
                        uint32_t button, uint32_t state) override;
    void pointer_axis(uint32_t time,
                      uint32_t axis,
                      wl_fixed_t value) override;
    void pointer_axis_source(uint32_t source) override;
    void pointer_axis_stop(uint32_t time, uint32_t axis) override;
    void pointer_axis_discrete(uint32_t axis, int32_t value) override;
    void pointer_frame() override;
    void pointer_axis_value120(uint32_t axis, int32_t value120) override;
    void pointer_axis_relative_direction(uint32_t axis, uint32_t direction) override;

private Q_SLOTS:
    void handleFocusDestroyed() { invalidateFocus(); }

private:
    void invalidateFocus();

public:
    void releaseButtons();
    void leavePointers();

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandSurface> mFocus;
    uint32_t mEnterSerial = 0;
#if QT_CONFIG(cursor)
    struct {
        QScopedPointer<QWaylandCursorShape> shape;
        QWaylandCursorTheme *theme = nullptr;
        int themeBufferScale = 0;
        QScopedPointer<CursorSurface<QWaylandInputDevice::Pointer>> surface;
        QTimer frameTimer;
        bool gotFrameCallback = false;
        bool gotTimerCallback = false;
    } mCursor;
#endif
    QPointF mSurfacePos;
    QPointF mGlobalPos;
    Qt::MouseButtons mButtons = Qt::NoButton;
    Qt::MouseButton mLastButton = Qt::NoButton;

    struct FrameData {
        QWaylandPointerEvent *event = nullptr;

        QPointF delta;
        QPoint delta120;
        axis_source axisSource = axis_source_wheel;
        bool verticalAxisInverted = false;
        bool horizontalAxisInverted = false;

        void resetScrollData();
        bool hasPixelDelta() const;
        QPoint pixelDeltaAndError(QPointF *accumulatedError) const;
        QPoint pixelDelta() const { return hasPixelDelta() ? delta.toPoint() : QPoint(); }
        QPoint angleDelta() const;
        Qt::MouseEventSource wheelEventSource() const;
    } mFrameData;

    bool mScrollBeginSent = false;
    QPointF mScrollDeltaRemainder;

    void setFrameEvent(QWaylandPointerEvent *event);
    void flushScrollEvent();
    void flushFrameEvent();
private: //TODO: should other methods be private as well?
    bool isDefinitelyTerminated(axis_source source) const;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandInputDevice::Touch : public QtWayland::wl_touch
{
public:
    Touch(QWaylandInputDevice *p);
    ~Touch() override;

    void touch_down(uint32_t serial,
                    uint32_t time,
                    struct wl_surface *surface,
                    int32_t id,
                    wl_fixed_t x,
                    wl_fixed_t y) override;
    void touch_up(uint32_t serial,
                  uint32_t time,
                  int32_t id) override;
    void touch_motion(uint32_t time,
                      int32_t id,
                      wl_fixed_t x,
                      wl_fixed_t y) override;
    void touch_frame() override;
    void touch_cancel() override;

    bool allTouchPointsReleased();
    void releasePoints();

    struct ::wl_touch *wl_touch() { return QtWayland::wl_touch::object(); }

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandWindow> mFocus;
    QList<QWindowSystemInterface::TouchPoint> mPendingTouchPoints;
};

class QWaylandPointerEvent
{
    Q_GADGET
public:
    inline QWaylandPointerEvent(QEvent::Type type, Qt::ScrollPhase phase, QWaylandWindow *surface,
                                ulong timestamp, const QPointF &localPos, const QPointF &globalPos,
                                Qt::MouseButtons buttons, Qt::MouseButton button,
                                Qt::KeyboardModifiers modifiers)
        : type(type)
        , phase(phase)
        , timestamp(timestamp)
        , local(localPos)
        , global(globalPos)
        , buttons(buttons)
        , button(button)
        , modifiers(modifiers)
        , surface(surface)
    {}
    inline QWaylandPointerEvent(QEvent::Type type, Qt::ScrollPhase phase, QWaylandWindow *surface,
                                ulong timestamp, const QPointF &local, const QPointF &global,
                                const QPoint &pixelDelta, const QPoint &angleDelta,
                                Qt::MouseEventSource source,
                                Qt::KeyboardModifiers modifiers, bool inverted)
        : type(type)
        , phase(phase)
        , timestamp(timestamp)
        , local(local)
        , global(global)
        , modifiers(modifiers)
        , pixelDelta(pixelDelta)
        , angleDelta(angleDelta)
        , source(source)
        , surface(surface)
        , inverted(inverted)
    {}

    QEvent::Type type = QEvent::None;
    Qt::ScrollPhase phase = Qt::NoScrollPhase;
    ulong timestamp = 0;
    QPointF local;
    QPointF global;
    Qt::MouseButtons buttons;
    Qt::MouseButton button = Qt::NoButton; // Button that caused the event (QMouseEvent::button)
    Qt::KeyboardModifiers modifiers;
    QPoint pixelDelta;
    QPoint angleDelta;
    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;
    QPointer<QWaylandWindow> surface;
    bool inverted = false;
};

#ifndef QT_NO_GESTURES
class QWaylandPointerGestureSwipeEvent
{
    Q_GADGET
public:
    inline QWaylandPointerGestureSwipeEvent(QWaylandWindow *surface, Qt::GestureState state,
                                            ulong timestamp, const QPointF &local,
                                            const QPointF &global, uint fingers, const QPointF& delta)
        : surface(surface)
        , state(state)
        , timestamp(timestamp)
        , local(local)
        , global(global)
        , fingers(fingers)
        , delta(delta)
    {}

    QPointer<QWaylandWindow> surface;
    Qt::GestureState state = Qt::GestureState::NoGesture;
    ulong timestamp = 0;
    QPointF local;
    QPointF global;
    uint fingers = 0;
    QPointF delta;
};

class QWaylandPointerGesturePinchEvent
{
    Q_GADGET
public:
    inline QWaylandPointerGesturePinchEvent(QWaylandWindow *surface, Qt::GestureState state,
                                            ulong timestamp, const QPointF &local,
                                            const QPointF &global, uint fingers, const QPointF& delta,
                                            qreal scale_delta, qreal rotation_delta)
        : surface(surface)
        , state(state)
        , timestamp(timestamp)
        , local(local)
        , global(global)
        , fingers(fingers)
        , delta(delta)
        , scale_delta(scale_delta)
        , rotation_delta(rotation_delta)
    {}

    QPointer<QWaylandWindow> surface;
    Qt::GestureState state = Qt::GestureState::NoGesture;
    ulong timestamp = 0;
    QPointF local;
    QPointF global;
    uint fingers = 0;
    QPointF delta;
    qreal scale_delta = 0;
    qreal rotation_delta = 0;
};
#endif // #ifndef QT_NO_GESTURES

}

QT_END_NAMESPACE

#endif
