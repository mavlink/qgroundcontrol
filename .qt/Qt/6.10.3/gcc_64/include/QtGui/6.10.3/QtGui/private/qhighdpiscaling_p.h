// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHIGHDPISCALING_P_H
#define QHIGHDPISCALING_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmargins.h>
#include <QtCore/qmath.h>
#include <QtCore/qrect.h>
#include <QtGui/qregion.h>
#include <QtGui/qscreen.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcHighDpi);

class QScreen;
class QPlatformScreen;
typedef std::pair<qreal, qreal> QDpi;

#ifndef QT_NO_HIGHDPISCALING
class Q_GUI_EXPORT QHighDpiScaling {
    Q_GADGET
public:
    enum class DpiAdjustmentPolicy {
        Unset,
        Enabled,
        Disabled,
        UpOnly
    };
    Q_ENUM(DpiAdjustmentPolicy)

    QHighDpiScaling() = delete;
    ~QHighDpiScaling() = delete;
    QHighDpiScaling(const QHighDpiScaling &) = delete;
    QHighDpiScaling &operator=(const QHighDpiScaling &) = delete;
    QHighDpiScaling(QHighDpiScaling &&) = delete;
    QHighDpiScaling &operator=(QHighDpiScaling &&) = delete;

    static void initHighDpiScaling();
    static void updateHighDpiScaling();
    static void setGlobalFactor(qreal factor);
    static void setScreenFactor(QScreen *screen, qreal factor);

    static bool isActive() { return m_active; }

    struct Point {
        enum Kind {
            Invalid,
            DeviceIndependent,
            Native
        };
        Kind kind;
        QPoint point;
    };

    struct ScaleAndOrigin
    {
        qreal factor;
        QPoint origin;
    };

    static ScaleAndOrigin scaleAndOrigin(const QPlatformScreen *platformScreen, Point position = Point{ Point::Invalid, QPoint() });
    static ScaleAndOrigin scaleAndOrigin(const QScreen *screen, Point position = Point{ Point::Invalid, QPoint() });
    static ScaleAndOrigin scaleAndOrigin(const QWindow *platformScreen, Point position = Point{ Point::Invalid, QPoint() });

    template<typename C>
    static qreal factor(C *context) {
        return scaleAndOrigin(context).factor;
    }

    static QPoint mapPositionFromNative(const QPoint &pos, const QPlatformScreen *platformScreen);
    static QPoint mapPositionToNative(const QPoint &pos, const QPlatformScreen *platformScreen);
    static QDpi logicalDpi(const QScreen *screen);
    static qreal roundScaleFactor(qreal rawFactor);

private:
    struct ScreenFactor {
        ScreenFactor(QString name, qreal factor)
            :name(name), factor(factor) { }
        QString name;
        qreal factor;
    };

    static qreal rawScaleFactor(const QPlatformScreen *screen);
    static QDpi effectiveLogicalDpi(const QPlatformScreen *screen, qreal rawFactor, qreal roundedFactor);
    static qreal screenSubfactor(const QPlatformScreen *screen);
    static QScreen *screenForPosition(Point position, QScreen *guess);
    static QList<QHighDpiScaling::ScreenFactor> parseScreenScaleFactorsSpec(QStringView screenScaleFactors);

    static qreal m_factor;
    static bool m_active;
    static bool m_usePlatformPluginDpi;
    static bool m_platformPluginDpiScalingActive;
    static bool m_globalScalingActive;
    static bool m_screenFactorSet;
    static bool m_usePhysicalDpi;
    static QVector<ScreenFactor> m_screenFactors;
    static DpiAdjustmentPolicy m_dpiAdjustmentPolicy;
    static QHash<QString, qreal> m_namedScreenScaleFactors;

#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const ScreenFactor &);
#endif
};

namespace QHighDpi {

inline qreal scale(qreal value, qreal scaleFactor, QPointF /* origin */ = QPointF(0, 0))
{
    return value * scaleFactor;
}

inline QSize scale(const QSize &value, qreal scaleFactor, QPointF /* origin */ = QPointF(0, 0))
{
    return value * scaleFactor;
}

inline QSizeF scale(const QSizeF &value, qreal scaleFactor, QPointF /* origin */ = QPointF(0, 0))
{
    return value * scaleFactor;
}

inline QVector2D scale(const QVector2D &value, qreal scaleFactor, QPointF /* origin */ = QPointF(0, 0))
{
    return value * float(scaleFactor);
}

inline QPointF scale(const QPointF &pos, qreal scaleFactor, QPointF origin = QPointF(0, 0))
{
     return (pos - origin) * scaleFactor + origin;
}

inline QPoint scale(const QPoint &pos, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
     return (pos - origin) * scaleFactor + origin;
}

inline QRect scale(const QRect &rect, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
    return QRect(scale(rect.topLeft(), scaleFactor, origin), scale(rect.size(), scaleFactor));
}

inline QRectF scale(const QRectF &rect, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
    return QRectF(scale(rect.topLeft(), scaleFactor, origin), scale(rect.size(), scaleFactor));
}

inline QMargins scale(const QMargins &margins, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
    Q_UNUSED(origin);
    return QMargins(qRound(qreal(margins.left()) * scaleFactor), qRound(qreal(margins.top()) * scaleFactor),
                    qRound(qreal(margins.right()) * scaleFactor), qRound(qreal(margins.bottom()) * scaleFactor));
}

template<typename T>
QList<T> scale(const QList<T> &list, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
    if (qFuzzyCompare(scaleFactor, qreal(1)))
        return list;

    QList<T> scaled;
    scaled.reserve(list.size());
    for (const T &item : list)
        scaled.append(scale(item, scaleFactor, origin));
    return scaled;
}

inline QRegion scale(const QRegion &region, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
    if (qFuzzyCompare(scaleFactor, qreal(1)))
        return region;

    QRegion scaled = region.translated(-origin);
    scaled = QTransform::fromScale(scaleFactor, scaleFactor).map(scaled);
    return scaled.translated(origin);
}

template <typename T>
inline QHighDpiScaling::Point position(T, QHighDpiScaling::Point::Kind) {
    return QHighDpiScaling::Point{ QHighDpiScaling::Point::Invalid, QPoint() };
}
inline QHighDpiScaling::Point position(QPoint point, QHighDpiScaling::Point::Kind kind) {
    return QHighDpiScaling::Point{ kind, point };
}
inline QHighDpiScaling::Point position(QPointF point, QHighDpiScaling::Point::Kind kind) {
    return QHighDpiScaling::Point{ kind, point.toPoint() };
}
inline QHighDpiScaling::Point position(QRect rect, QHighDpiScaling::Point::Kind kind) {
    return QHighDpiScaling::Point{ kind, rect.topLeft() };
}
inline QHighDpiScaling::Point position(QRectF rect, QHighDpiScaling::Point::Kind kind) {
    return QHighDpiScaling::Point{ kind, rect.topLeft().toPoint() };
}

template <typename T, typename C>
T fromNativePixels(const T &value, const C *context)
{
    QHighDpiScaling::ScaleAndOrigin so = QHighDpiScaling::scaleAndOrigin(context);
    return scale(value, qreal(1) / so.factor, so.origin);
}

template <typename T, typename C>
T toNativePixels(const T &value, const C *context)
{
    QHighDpiScaling::ScaleAndOrigin so = QHighDpiScaling::scaleAndOrigin(context);
    return scale(value, so.factor, so.origin);
}

template <typename T, typename C>
T fromNativeLocalPosition(const T &value, const C *context)
{
    return scale(value, qreal(1) / QHighDpiScaling::factor(context));
}

template <typename T, typename C>
T toNativeLocalPosition(const T &value, const C *context)
{
    return scale(value, QHighDpiScaling::factor(context));
}

template <typename T, typename C>
T fromNativeGlobalPosition(const T &value, const C *context)
{
    QHighDpiScaling::ScaleAndOrigin so =
        QHighDpiScaling::scaleAndOrigin(context, position(value, QHighDpiScaling::Point::Native));
    return scale(value, qreal(1) / so.factor, so.origin);
}

template <typename T, typename C>
T toNativeGlobalPosition(const T &value, const C *context)
{
    QHighDpiScaling::ScaleAndOrigin so =
        QHighDpiScaling::scaleAndOrigin(context, position(value, QHighDpiScaling::Point::DeviceIndependent));
    return scale(value, so.factor, so.origin);
}

template <typename T, typename C>
T fromNativeWindowGeometry(const T &value, const C *context)
{
    QHighDpiScaling::ScaleAndOrigin so = QHighDpiScaling::scaleAndOrigin(context);
    QPoint effectiveOrigin = (context && context->isTopLevel()) ? so.origin : QPoint(0,0);
    return scale(value, qreal(1) / so.factor, effectiveOrigin);
}

template <typename T, typename C>
T toNativeWindowGeometry(const T &value, const C *context)
{
    QHighDpiScaling::ScaleAndOrigin so = QHighDpiScaling::scaleAndOrigin(context);
    QPoint effectiveOrigin = (context && context->isTopLevel()) ? so.origin : QPoint(0,0);
    return scale(value, so.factor, effectiveOrigin);
}

template <typename T>
inline T fromNative(const T &value, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
    return scale(value, qreal(1) / scaleFactor, origin);
}

template <typename T>
inline T toNative(const T &value, qreal scaleFactor, QPoint origin = QPoint(0, 0))
{
    return scale(value, scaleFactor, origin);
}

inline QRect fromNative(const QRect &rect, const QScreen *screen, const QPoint &screenOrigin)
{
    return scale(rect, qreal(1) / QHighDpiScaling::factor(screen), screenOrigin);
}

inline QRect fromNativeScreenGeometry(const QRect &nativeScreenGeometry, const QScreen *screen)
{
    return QRect(nativeScreenGeometry.topLeft(),
                 scale(nativeScreenGeometry.size(), qreal(1) / QHighDpiScaling::factor(screen)));
}

inline QRegion fromNativeLocalRegion(const QRegion &pixelRegion, const QWindow *window)
{
    return scale(pixelRegion, qreal(1) / QHighDpiScaling::factor(window));
}

// When mapping expose events to Qt rects: round top/left towards the origin and
// bottom/right away from the origin, making sure that we cover the whole window.
inline QRegion fromNativeLocalExposedRegion(const QRegion &pixelRegion, const QWindow *window)
{
    if (!QHighDpiScaling::isActive())
        return pixelRegion;

    const qreal scaleFactor = QHighDpiScaling::factor(window);
    QRegion pointRegion;
    for (const QRectF rect: pixelRegion)
        pointRegion += QRectF(rect.topLeft() / scaleFactor, rect.size() / scaleFactor).toAlignedRect();

    return pointRegion;
}

inline QRegion toNativeLocalRegion(const QRegion &pointRegion, const QWindow *window)
{
    return scale(pointRegion, QHighDpiScaling::factor(window));
}

} // namespace QHighDpi
#else // QT_NO_HIGHDPISCALING
class Q_GUI_EXPORT QHighDpiScaling {
public:
    static inline void initHighDpiScaling() {}
    static inline void updateHighDpiScaling() {}
    static inline void setGlobalFactor(qreal) {}
    static inline void setScreenFactor(QScreen *, qreal) {}

    struct ScaleAndOrigin
    {
        qreal factor;
        QPoint origin;
    };
    static ScaleAndOrigin scaleAndOrigin(const QPlatformScreen *platformScreen, QPoint *nativePosition = nullptr);
    static ScaleAndOrigin scaleAndOrigin(const QScreen *screen, QPoint *nativePosition = nullptr);
    static ScaleAndOrigin scaleAndOrigin(const QWindow *platformScreen, QPoint *nativePosition = nullptr);

    static inline bool isActive() { return false; }
    static inline qreal factor(const QWindow *) { return 1.0; }
    static inline qreal factor(const QScreen *) { return 1.0; }
    static inline qreal factor(const QPlatformScreen *) { return 1.0; }
    static inline QPoint origin(const QScreen *) { return QPoint(); }
    static inline QPoint origin(const QPlatformScreen *) { return QPoint(); }
    static inline QPoint mapPositionFromNative(const QPoint &pos, const QPlatformScreen *) { return pos; }
    static inline QPoint mapPositionToNative(const QPoint &pos, const QPlatformScreen *) { return pos; }
    static inline QPointF mapPositionToGlobal(const QPointF &pos, const QPoint &, const QWindow *) { return pos; }
    static inline QPointF mapPositionFromGlobal(const QPointF &pos, const QPoint &, const QWindow *) { return pos; }
    static inline QDpi logicalDpi(const QScreen *) { return QDpi(-1,-1); }
    static inline qreal roundScaleFactor(qreal) { return 1.0; }
};

namespace QHighDpi {
    template <typename T> inline
    T scale(const T &value, ...) { return value; }

    template <typename T> inline
    T toNative(const T &value, ...) { return value; }
    template <typename T> inline
    T fromNative(const T &value, ...) { return value; }

    template <typename T> inline
    T fromNativeLocalPosition(const T &value, ...) { return value; }
    template <typename T> inline
    T toNativeLocalPosition(const T &value, ...) { return value; }
    template <typename T, typename C> inline
    T fromNativeGlobalPosition(const T &value, const C *) { return value; }
    template <typename T, typename C> inline
    T toNativeGlobalPosition(const T &value, const C *) { return value; }
    template <typename T, typename C> inline
    T fromNativeWindowGeometry(const T &value, const C *) { return value; }
    template <typename T, typename C> inline
    T toNativeWindowGeometry(const T &value, const C *) { return value; }

    template <typename T> inline
    T fromNativeLocalRegion(const T &value, ...) { return value; }
    template <typename T> inline
    T fromNativeLocalExposedRegion(const T &value, ...) { return value; }
    template <typename T> inline
    T toNativeLocalRegion(const T &value, ...) { return value; }

    template <typename T> inline
    T fromNativeScreenGeometry(const T &value, ...) { return value; }

    template <typename T, typename U> inline
    T toNativePixels(const T &value, const U*) {return value;}
    template <typename T, typename U> inline
    T fromNativePixels(const T &value, const U*) {return value;}
}
#endif // QT_NO_HIGHDPISCALING
QT_END_NAMESPACE

#endif // QHIGHDPISCALING_P_H
