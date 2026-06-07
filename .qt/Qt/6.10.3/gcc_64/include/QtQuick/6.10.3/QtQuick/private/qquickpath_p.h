// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPATH_H
#define QQUICKPATH_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_path);

#include <qqml.h>

#include <private/qqmlnullablevalue_p.h>
#include <private/qlazilyallocated_p.h>
#include <private/qbezier_p.h>
#include <private/qtquickglobal_p.h>

#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtGui/QPainterPath>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE

class QQuickCurve;
class QQuickCurvePrivate;

struct QQuickPathData
{
    int index;
    QPointF endPoint;
    QList<QQuickCurve*> curves;
};

class Q_QUICK_EXPORT QQuickPathElement : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathElement(QObject *parent=nullptr) : QObject(parent) {}
Q_SIGNALS:
    void changed();
};

class Q_QUICK_EXPORT QQuickPathAttribute : public QQuickPathElement
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    QML_NAMED_ELEMENT(PathAttribute)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathAttribute(QObject *parent=nullptr) : QQuickPathElement(parent) {}


    QString name() const;
    void setName(const QString &name);

    qreal value() const;
    void setValue(qreal value);

Q_SIGNALS:
    void nameChanged();
    void valueChanged();

private:
    QString _name;
    qreal _value = 0;
};

class Q_QUICK_EXPORT QQuickCurve : public QQuickPathElement
{
    Q_OBJECT

    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal relativeX READ relativeX WRITE setRelativeX NOTIFY relativeXChanged)
    Q_PROPERTY(qreal relativeY READ relativeY WRITE setRelativeY NOTIFY relativeYChanged)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickCurve(QObject *parent=nullptr) : QQuickPathElement(parent) {}

    qreal x() const;
    void setX(qreal x);
    bool hasX();

    qreal y() const;
    void setY(qreal y);
    bool hasY();

    qreal relativeX() const;
    void setRelativeX(qreal x);
    bool hasRelativeX();

    qreal relativeY() const;
    void setRelativeY(qreal y);
    bool hasRelativeY();

    virtual void addToPath(QPainterPath &, const QQuickPathData &) {}

Q_SIGNALS:
    void xChanged();
    void yChanged();
    void relativeXChanged();
    void relativeYChanged();

private:
#ifndef QT_NO_DEBUG_STREAM
    friend Q_QUICK_EXPORT QDebug operator<<(QDebug debug, const QQuickCurve *curve);
#endif

    QQmlNullableValue<qreal> _x;
    QQmlNullableValue<qreal> _y;
    QQmlNullableValue<qreal> _relativeX;
    QQmlNullableValue<qreal> _relativeY;
};

class Q_QUICK_EXPORT QQuickPathLine : public QQuickCurve
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PathLine)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathLine(QObject *parent=nullptr) : QQuickCurve(parent) {}

    void addToPath(QPainterPath &path, const QQuickPathData &) override;
};

class Q_QUICK_EXPORT QQuickPathMove : public QQuickCurve
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PathMove)
    QML_ADDED_IN_VERSION(2, 9)
public:
    QQuickPathMove(QObject *parent=nullptr) : QQuickCurve(parent) {}

    void addToPath(QPainterPath &path, const QQuickPathData &) override;
};

class Q_QUICK_EXPORT QQuickPathQuad : public QQuickCurve
{
    Q_OBJECT

    Q_PROPERTY(qreal controlX READ controlX WRITE setControlX NOTIFY controlXChanged)
    Q_PROPERTY(qreal controlY READ controlY WRITE setControlY NOTIFY controlYChanged)
    Q_PROPERTY(qreal relativeControlX READ relativeControlX WRITE setRelativeControlX NOTIFY relativeControlXChanged)
    Q_PROPERTY(qreal relativeControlY READ relativeControlY WRITE setRelativeControlY NOTIFY relativeControlYChanged)

    QML_NAMED_ELEMENT(PathQuad)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathQuad(QObject *parent=nullptr) : QQuickCurve(parent) {}

    qreal controlX() const;
    void setControlX(qreal x);

    qreal controlY() const;
    void setControlY(qreal y);

    qreal relativeControlX() const;
    void setRelativeControlX(qreal x);
    bool hasRelativeControlX();

    qreal relativeControlY() const;
    void setRelativeControlY(qreal y);
    bool hasRelativeControlY();

    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void controlXChanged();
    void controlYChanged();
    void relativeControlXChanged();
    void relativeControlYChanged();

private:
    qreal _controlX = 0;
    qreal _controlY = 0;
    QQmlNullableValue<qreal> _relativeControlX;
    QQmlNullableValue<qreal> _relativeControlY;
};

class Q_QUICK_EXPORT QQuickPathCubic : public QQuickCurve
{
    Q_OBJECT

    Q_PROPERTY(qreal control1X READ control1X WRITE setControl1X NOTIFY control1XChanged)
    Q_PROPERTY(qreal control1Y READ control1Y WRITE setControl1Y NOTIFY control1YChanged)
    Q_PROPERTY(qreal control2X READ control2X WRITE setControl2X NOTIFY control2XChanged)
    Q_PROPERTY(qreal control2Y READ control2Y WRITE setControl2Y NOTIFY control2YChanged)
    Q_PROPERTY(qreal relativeControl1X READ relativeControl1X WRITE setRelativeControl1X NOTIFY relativeControl1XChanged)
    Q_PROPERTY(qreal relativeControl1Y READ relativeControl1Y WRITE setRelativeControl1Y NOTIFY relativeControl1YChanged)
    Q_PROPERTY(qreal relativeControl2X READ relativeControl2X WRITE setRelativeControl2X NOTIFY relativeControl2XChanged)
    Q_PROPERTY(qreal relativeControl2Y READ relativeControl2Y WRITE setRelativeControl2Y NOTIFY relativeControl2YChanged)
    QML_NAMED_ELEMENT(PathCubic)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathCubic(QObject *parent=nullptr) : QQuickCurve(parent) {}

    qreal control1X() const;
    void setControl1X(qreal x);

    qreal control1Y() const;
    void setControl1Y(qreal y);

    qreal control2X() const;
    void setControl2X(qreal x);

    qreal control2Y() const;
    void setControl2Y(qreal y);

    qreal relativeControl1X() const;
    void setRelativeControl1X(qreal x);
    bool hasRelativeControl1X();

    qreal relativeControl1Y() const;
    void setRelativeControl1Y(qreal y);
    bool hasRelativeControl1Y();

    qreal relativeControl2X() const;
    void setRelativeControl2X(qreal x);
    bool hasRelativeControl2X();

    qreal relativeControl2Y() const;
    void setRelativeControl2Y(qreal y);
    bool hasRelativeControl2Y();

    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void control1XChanged();
    void control1YChanged();
    void control2XChanged();
    void control2YChanged();
    void relativeControl1XChanged();
    void relativeControl1YChanged();
    void relativeControl2XChanged();
    void relativeControl2YChanged();

private:
    qreal _control1X = 0;
    qreal _control1Y = 0;
    qreal _control2X = 0;
    qreal _control2Y = 0;
    QQmlNullableValue<qreal> _relativeControl1X;
    QQmlNullableValue<qreal> _relativeControl1Y;
    QQmlNullableValue<qreal> _relativeControl2X;
    QQmlNullableValue<qreal> _relativeControl2Y;
};

class Q_QUICK_EXPORT QQuickPathCatmullRomCurve : public QQuickCurve
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PathCurve)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathCatmullRomCurve(QObject *parent=nullptr) : QQuickCurve(parent) {}

    void addToPath(QPainterPath &path, const QQuickPathData &) override;
};

class Q_QUICK_EXPORT QQuickPathArc : public QQuickCurve
{
    Q_OBJECT
    Q_PROPERTY(qreal radiusX READ radiusX WRITE setRadiusX NOTIFY radiusXChanged)
    Q_PROPERTY(qreal radiusY READ radiusY WRITE setRadiusY NOTIFY radiusYChanged)
    Q_PROPERTY(bool useLargeArc READ useLargeArc WRITE setUseLargeArc NOTIFY useLargeArcChanged)
    Q_PROPERTY(ArcDirection direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(qreal xAxisRotation READ xAxisRotation WRITE setXAxisRotation NOTIFY xAxisRotationChanged REVISION(2, 9))
    QML_NAMED_ELEMENT(PathArc)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickPathArc(QObject *parent=nullptr)
        : QQuickCurve(parent) {}

    enum ArcDirection { Clockwise, Counterclockwise };
    Q_ENUM(ArcDirection)

    qreal radiusX() const;
    void setRadiusX(qreal);

    qreal radiusY() const;
    void setRadiusY(qreal);

    bool useLargeArc() const;
    void setUseLargeArc(bool);

    ArcDirection direction() const;
    void setDirection(ArcDirection direction);

    qreal xAxisRotation() const;
    void setXAxisRotation(qreal rotation);

    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void radiusXChanged();
    void radiusYChanged();
    void useLargeArcChanged();
    void directionChanged();
    Q_REVISION(2, 9) void xAxisRotationChanged();

private:
    qreal _radiusX = 0;
    qreal _radiusY = 0;
    bool _useLargeArc = false;
    ArcDirection _direction = Clockwise;
    qreal _xAxisRotation = 0;
};

class Q_QUICK_EXPORT QQuickPathAngleArc : public QQuickCurve
{
    Q_OBJECT
    Q_PROPERTY(qreal centerX READ centerX WRITE setCenterX NOTIFY centerXChanged)
    Q_PROPERTY(qreal centerY READ centerY WRITE setCenterY NOTIFY centerYChanged)
    Q_PROPERTY(qreal radiusX READ radiusX WRITE setRadiusX NOTIFY radiusXChanged)
    Q_PROPERTY(qreal radiusY READ radiusY WRITE setRadiusY NOTIFY radiusYChanged)
    Q_PROPERTY(qreal startAngle READ startAngle WRITE setStartAngle NOTIFY startAngleChanged)
    Q_PROPERTY(qreal sweepAngle READ sweepAngle WRITE setSweepAngle NOTIFY sweepAngleChanged)
    Q_PROPERTY(bool moveToStart READ moveToStart WRITE setMoveToStart NOTIFY moveToStartChanged)

    QML_NAMED_ELEMENT(PathAngleArc)
    QML_ADDED_IN_VERSION(2, 11)

public:
    QQuickPathAngleArc(QObject *parent=nullptr)
        : QQuickCurve(parent) {}

    qreal centerX() const;
    void setCenterX(qreal);

    qreal centerY() const;
    void setCenterY(qreal);

    qreal radiusX() const;
    void setRadiusX(qreal);

    qreal radiusY() const;
    void setRadiusY(qreal);

    qreal startAngle() const;
    void setStartAngle(qreal);

    qreal sweepAngle() const;
    void setSweepAngle(qreal);

    bool moveToStart() const;
    void setMoveToStart(bool);

    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void centerXChanged();
    void centerYChanged();
    void radiusXChanged();
    void radiusYChanged();
    void startAngleChanged();
    void sweepAngleChanged();
    void moveToStartChanged();

private:
    qreal _centerX = 0;
    qreal _centerY = 0;
    qreal _radiusX = 0;
    qreal _radiusY = 0;
    qreal _startAngle = 0;
    qreal _sweepAngle = 0;
    bool _moveToStart = true;
};

class Q_QUICK_EXPORT QQuickPathSvg : public QQuickCurve
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    QML_NAMED_ELEMENT(PathSvg)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathSvg(QObject *parent=nullptr) : QQuickCurve(parent) {}

    QString path() const;
    void setPath(const QString &path);

    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void pathChanged();

private:
    QString _path;
};

class Q_QUICK_EXPORT QQuickPathRectangle : public QQuickCurve
{
    Q_OBJECT

    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(qreal strokeAdjustment READ strokeAdjustment WRITE setStrokeAdjustment NOTIFY strokeAdjustmentChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(qreal topLeftRadius READ topLeftRadius WRITE setTopLeftRadius RESET resetTopLeftRadius NOTIFY topLeftRadiusChanged FINAL)
    Q_PROPERTY(qreal topRightRadius READ topRightRadius WRITE setTopRightRadius NOTIFY topRightRadiusChanged RESET resetTopRightRadius FINAL)
    Q_PROPERTY(qreal bottomLeftRadius READ bottomLeftRadius WRITE setBottomLeftRadius NOTIFY bottomLeftRadiusChanged RESET resetBottomLeftRadius FINAL)
    Q_PROPERTY(qreal bottomRightRadius READ bottomRightRadius WRITE setBottomRightRadius NOTIFY bottomRightRadiusChanged RESET resetBottomRightRadius FINAL)
    Q_PROPERTY(bool bevel READ hasBevel WRITE setBevel NOTIFY bevelChanged FINAL REVISION(6, 10))
    Q_PROPERTY(bool topLeftBevel READ hasTopLeftBevel WRITE setTopLeftBevel NOTIFY topLeftBevelChanged RESET resetTopLeftBevel FINAL REVISION(6, 10))
    Q_PROPERTY(bool topRightBevel READ hasTopRightBevel WRITE setTopRightBevel NOTIFY topRightBevelChanged RESET resetTopRightBevel FINAL REVISION(6, 10))
    Q_PROPERTY(bool bottomLeftBevel READ hasBottomLeftBevel WRITE setBottomLeftBevel NOTIFY bottomLeftBevelChanged RESET resetBottomLeftBevel FINAL REVISION(6, 10))
    Q_PROPERTY(bool bottomRightBevel READ hasBottomRightBevel WRITE setBottomRightBevel NOTIFY bottomRightBevelChanged RESET resetBottomRightBevel FINAL REVISION(6, 10))

    QML_NAMED_ELEMENT(PathRectangle)
    QML_ADDED_IN_VERSION(6, 8)
public:
    QQuickPathRectangle(QObject *parent = nullptr) : QQuickCurve(parent) {}

    qreal width() const;
    void setWidth(qreal width);

    qreal height() const;
    void setHeight(qreal height);

    qreal strokeAdjustment() const;
    void setStrokeAdjustment(qreal newStrokeAdjustment);

    qreal radius() const;
    void setRadius(qreal newRadius);

    qreal topLeftRadius() const { return cornerRadius(Qt::TopLeftCorner); }
    void setTopLeftRadius(qreal radius) { setCornerRadius(Qt::TopLeftCorner, radius); }
    void resetTopLeftRadius() { resetCornerRadius(Qt::TopLeftCorner); }

    qreal topRightRadius() const { return cornerRadius(Qt::TopRightCorner); }
    void setTopRightRadius(qreal radius) { setCornerRadius(Qt::TopRightCorner, radius); }
    void resetTopRightRadius() { resetCornerRadius(Qt::TopRightCorner); }

    qreal bottomLeftRadius() const { return cornerRadius(Qt::BottomLeftCorner); }
    void setBottomLeftRadius(qreal radius) { setCornerRadius(Qt::BottomLeftCorner, radius); }
    void resetBottomLeftRadius() { resetCornerRadius(Qt::BottomLeftCorner); }

    qreal bottomRightRadius() const { return cornerRadius(Qt::BottomRightCorner); }
    void setBottomRightRadius(qreal radius) { setCornerRadius(Qt::BottomRightCorner, radius); }
    void resetBottomRightRadius() { resetCornerRadius(Qt::BottomRightCorner); }

    qreal cornerRadius(Qt::Corner corner) const;
    void setCornerRadius(Qt::Corner corner, qreal newCornerRadius);
    void resetCornerRadius(Qt::Corner corner);

    bool hasBevel() const;
    void setBevel(bool bevel);

    bool hasTopLeftBevel() const { return cornerBevel(Qt::TopLeftCorner); }
    void setTopLeftBevel(bool bevel) { setCornerBevel(Qt::TopLeftCorner, bevel); }
    void resetTopLeftBevel() { resetCornerBevel(Qt::TopLeftCorner); }

    bool hasTopRightBevel() const { return cornerBevel(Qt::TopRightCorner); }
    void setTopRightBevel(bool bevel) { setCornerBevel(Qt::TopRightCorner, bevel); }
    void resetTopRightBevel() { resetCornerBevel(Qt::TopRightCorner); }

    bool hasBottomLeftBevel() const { return cornerBevel(Qt::BottomLeftCorner); }
    void setBottomLeftBevel(bool bevel) { setCornerBevel(Qt::BottomLeftCorner, bevel); }
    void resetBottomLeftBevel() { resetCornerBevel(Qt::BottomLeftCorner); }

    bool hasBottomRightBevel() const { return cornerBevel(Qt::BottomRightCorner); }
    void setBottomRightBevel(bool bevel) { setCornerBevel(Qt::BottomRightCorner, bevel); }
    void resetBottomRightBevel() { resetCornerBevel(Qt::BottomRightCorner); }

    bool cornerBevel(Qt::Corner corner) const;
    void setCornerBevel(Qt::Corner corner, bool newCornerBevel);
    void resetCornerBevel(Qt::Corner corner);

    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void widthChanged();
    void heightChanged();
    void strokeAdjustmentChanged();
    void radiusChanged();
    void topLeftRadiusChanged();
    void topRightRadiusChanged();
    void bottomLeftRadiusChanged();
    void bottomRightRadiusChanged();
    Q_REVISION(6, 10) void bevelChanged();
    Q_REVISION(6, 10) void topLeftBevelChanged();
    Q_REVISION(6, 10) void topRightBevelChanged();
    Q_REVISION(6, 10) void bottomLeftBevelChanged();
    Q_REVISION(6, 10) void bottomRightBevelChanged();

private:
    void emitCornerRadiusChanged(Qt::Corner corner);
    void emitCornerBevelChanged(Qt::Corner corner);

    qreal _width = 0;
    qreal _height = 0;
    qreal _strokeAdjustment = 0;
    struct ExtraData
    {
        ExtraData() {
            std::fill_n(cornerRadii, 4, 0);
            cornerProperties = 0;
        }
        qreal radius = 0;
        qreal cornerRadii[4];
        unsigned cornerProperties :9;
        inline bool isRadiusSet(Qt::Corner corner) {
            return cornerProperties & (1 << corner);
        };
        inline bool isBevelSet(Qt::Corner corner) {
            return cornerProperties & (1 << (corner + 4));
        };
    };
    QLazilyAllocated<ExtraData> _extra;
};

class Q_QUICK_EXPORT QQuickPathPercent : public QQuickPathElement
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    QML_NAMED_ELEMENT(PathPercent)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPathPercent(QObject *parent=nullptr) : QQuickPathElement(parent) {}

    qreal value() const;
    void setValue(qreal value);

Q_SIGNALS:
    void valueChanged();

private:
    qreal _value = 0;
};

class Q_QUICK_EXPORT QQuickPathPolyline : public QQuickCurve
{
    Q_OBJECT
    Q_PROPERTY(QPointF start READ start NOTIFY startChanged)
    Q_PROPERTY(QVariant path READ path WRITE setPath NOTIFY pathChanged)
    QML_NAMED_ELEMENT(PathPolyline)
    QML_ADDED_IN_VERSION(2, 14)
public:
    QQuickPathPolyline(QObject *parent=nullptr);

    QVariant path() const;
    void setPath(const QVariant &path);
    void setPath(const QVector<QPointF> &path);
    QPointF start() const;
    void addToPath(QPainterPath &path, const QQuickPathData &data) override;

Q_SIGNALS:
    void pathChanged();
    void startChanged();

private:
    QVector<QPointF> m_path;
};

class Q_QUICK_EXPORT QQuickPathMultiline : public QQuickCurve
{
    Q_OBJECT
    Q_PROPERTY(QPointF start READ start NOTIFY startChanged)
    Q_PROPERTY(QVariant paths READ paths WRITE setPaths NOTIFY pathsChanged)
    QML_NAMED_ELEMENT(PathMultiline)
    QML_ADDED_IN_VERSION(2, 14)
public:
    QQuickPathMultiline(QObject *parent=nullptr);

    QVariant paths() const;
    void setPaths(const QVariant &paths);
    void setPaths(const QVector<QVector<QPointF>> &paths);
    QPointF start() const;
    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void pathsChanged();
    void startChanged();

private:
    QPointF absolute(const QPointF &relative) const;

    QVector<QVector<QPointF>> m_paths;
};

struct QQuickCachedBezier
{
    QQuickCachedBezier() {}
    QBezier bezier;
    int element;
    qreal bezLength;
    qreal currLength;
    qreal p;
    bool isValid = false;
};

class QQuickPathPrivate;
class Q_QUICK_EXPORT QQuickPath : public QObject, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QQuickPathElement> pathElements READ pathElements)
    Q_PROPERTY(qreal startX READ startX WRITE setStartX NOTIFY startXChanged)
    Q_PROPERTY(qreal startY READ startY WRITE setStartY NOTIFY startYChanged)
    Q_PROPERTY(bool closed READ isClosed NOTIFY changed)
    Q_PROPERTY(bool simplify READ simplify WRITE setSimplify NOTIFY simplifyChanged REVISION(6, 6) FINAL)
    Q_PROPERTY(QSizeF scale READ scale WRITE setScale NOTIFY scaleChanged REVISION(2, 14))
    Q_PROPERTY(bool asynchronous READ isAsynchronous WRITE setAsynchronous NOTIFY asynchronousChanged REVISION(6, 9))
    Q_CLASSINFO("DefaultProperty", "pathElements")
    QML_NAMED_ELEMENT(Path)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickPath(QObject *parent=nullptr);
    ~QQuickPath() override;

    QQmlListProperty<QQuickPathElement> pathElements();

    qreal startX() const;
    void setStartX(qreal x);
    bool hasStartX() const;

    qreal startY() const;
    void setStartY(qreal y);
    bool hasStartY() const;

    bool isClosed() const;

    QPainterPath path() const;
    void setPath(const QPainterPath &path);

    QStringList attributes() const;
    qreal attributeAt(const QString &, qreal) const;
    Q_REVISION(2, 14) Q_INVOKABLE QPointF pointAtPercent(qreal t) const;
    QPointF sequentialPointAt(qreal p, qreal *angle = nullptr) const;
    void invalidateSequentialHistory() const;

    QSizeF scale() const;
    void setScale(const QSizeF &scale);

    bool simplify() const;
    void setSimplify(bool s);

    bool isAsynchronous() const;
    void setAsynchronous(bool a);

Q_SIGNALS:
    void changed();
    void startXChanged();
    void startYChanged();
    Q_REVISION(6, 6) void simplifyChanged();
    Q_REVISION(2, 14) void scaleChanged();
    Q_REVISION(6, 9) void asynchronousChanged();

protected:
    QQuickPath(QQuickPathPrivate &dd, QObject *parent = nullptr);
    void componentComplete() override;
    void classBegin() override;
    void disconnectPathElements();
    void connectPathElements();
    void gatherAttributes();

    // pathElements property
    static QQuickPathElement *pathElements_at(QQmlListProperty<QQuickPathElement> *, qsizetype);
    static void pathElements_append(QQmlListProperty<QQuickPathElement> *, QQuickPathElement *);
    static qsizetype pathElements_count(QQmlListProperty<QQuickPathElement> *);
    static void pathElements_clear(QQmlListProperty<QQuickPathElement> *);
    static void pathElements_replace(
            QQmlListProperty<QQuickPathElement> *, qsizetype, QQuickPathElement *);
    static void pathElements_removeLast(QQmlListProperty<QQuickPathElement> *);

private Q_SLOTS:
    void processPath();

private:
    struct AttributePoint {
        AttributePoint() {}
        AttributePoint(const AttributePoint &other)
            : percent(other.percent), scale(other.scale), origpercent(other.origpercent), values(other.values) {}
        AttributePoint &operator=(const AttributePoint &other) {
            percent = other.percent; scale = other.scale; origpercent = other.origpercent; values = other.values; return *this;
        }
        qreal percent = 0;      //massaged percent along the painter path
        qreal scale = 1;
        qreal origpercent = 0;  //'real' percent along the painter path
        QHash<QString, qreal> values;
    };

    void doProcessPath();
    void interpolate(int idx, const QString &name, qreal value);
    void endpoint(const QString &name);
    void createPointCache() const;

    static void interpolate(QList<AttributePoint> &points, int idx, const QString &name, qreal value);
    static void endpoint(QList<AttributePoint> &attributePoints, const QString &name);
    static QPointF forwardsPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QQuickCachedBezier &prevBez, qreal p, qreal *angle = nullptr);
    static QPointF backwardsPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QQuickCachedBezier &prevBez, qreal p, qreal *angle = nullptr);

private:
    Q_DISABLE_COPY(QQuickPath)
    Q_DECLARE_PRIVATE(QQuickPath)
    friend class QQuickPathAnimationUpdater;

public:
    QPainterPath createPath(const QPointF &startPoint, const QPointF &endPoint, const QStringList &attributes, qreal &pathLength, QList<AttributePoint> &attributePoints, bool *closed = nullptr);
    QPainterPath createShapePath(const QPointF &startPoint, const QPointF &endPoint, qreal &pathLength, bool *closed = nullptr);
    static QPointF sequentialPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QQuickCachedBezier &prevBez, qreal p, qreal *angle = nullptr);
};

class Q_QUICK_EXPORT QQuickPathText : public QQuickPathElement
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal width READ width NOTIFY changed)
    Q_PROPERTY(qreal height READ height NOTIFY changed)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    QML_NAMED_ELEMENT(PathText)
    QML_ADDED_IN_VERSION(2, 15)
public:
    QQuickPathText(QObject *parent=nullptr) : QQuickPathElement(parent)
    {
        connect(this, &QQuickPathText::xChanged, this, &QQuickPathElement::changed);
        connect(this, &QQuickPathText::yChanged, this, &QQuickPathElement::changed);
        connect(this, &QQuickPathText::textChanged, this, &QQuickPathElement::changed);
        connect(this, &QQuickPathText::fontChanged, this, &QQuickPathElement::changed);

        connect(this, &QQuickPathElement::changed, this, &QQuickPathText::invalidate);
    }

    void addToPath(QPainterPath &path);

    qreal x() const { return _x; }
    qreal y() const { return _y; }
    QString text() const { return _text; }
    QFont font() const { return _font; }

    void setX(qreal x)
    {
        if (qFuzzyCompare(_x, x))
            return;

        _x = x;
        Q_EMIT xChanged();
    }

    void setY(qreal y)
    {
        if (qFuzzyCompare(_y, y))
            return;

        _y = y;
        Q_EMIT yChanged();
    }

    void setText(const QString &text)
    {
        if (text == _text)
            return;

        _text = text;
        Q_EMIT textChanged();
    }

    void setFont(const QFont &font)
    {
        if (font == _font)
            return;

        _font = font;
        Q_EMIT fontChanged();
    }

    qreal width() const
    {
        updatePath();
        return _path.boundingRect().width();
    }

    qreal height() const
    {
        updatePath();
        return _path.boundingRect().height();
    }

Q_SIGNALS:
    void xChanged();
    void yChanged();
    void textChanged();
    void fontChanged();

private Q_SLOTS:
    void invalidate()
    {
        _path.clear();
    }

private:
    void updatePath() const;

    QString _text;
    qreal _x = qreal(0.0);
    qreal _y = qreal(0.0);
    QFont _font;

    mutable QPainterPath _path;
};

QT_END_NAMESPACE

#endif // QQUICKPATH_H
