// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPE_P_H
#define QQUICKSHAPE_P_H

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

#include <QtQuickShapes/private/qquickshapesglobal_p.h>
#include <QtQuick/qquickitem.h>

#include <private/qtquickglobal_p.h>
#include <private/qquickpath_p_p.h>
#include <private/qquickrectangle_p.h>

QT_BEGIN_NAMESPACE

class QQuickShapePathPrivate;
class QQuickShapePrivate;

void Q_QUICKSHAPES_EXPORT QQuickShapes_initializeModule();

class Q_QUICKSHAPES_EXPORT QQuickShapesModule
{
public:
    static void defineModule();
};

class Q_QUICKSHAPES_EXPORT QQuickShapeGradient : public QQuickGradient
{
    Q_OBJECT
    Q_PROPERTY(SpreadMode spread READ spread WRITE setSpread NOTIFY spreadChanged)
    Q_CLASSINFO("DefaultProperty", "stops")

    QML_NAMED_ELEMENT(ShapeGradient)
    QML_ADDED_IN_VERSION(1, 0)
    QML_UNCREATABLE("ShapeGradient is an abstract base class.");

public:
    enum SpreadMode {
        PadSpread,
        ReflectSpread,
        RepeatSpread
    };
    Q_ENUM(SpreadMode)

    QQuickShapeGradient(QObject *parent = nullptr);

    SpreadMode spread() const;
    void setSpread(SpreadMode mode);

Q_SIGNALS:
    void spreadChanged();

private:
    SpreadMode m_spread;
};

class Q_QUICKSHAPES_EXPORT QQuickShapeLinearGradient : public QQuickShapeGradient
{
    Q_OBJECT
    Q_PROPERTY(qreal x1 READ x1 WRITE setX1 NOTIFY x1Changed)
    Q_PROPERTY(qreal y1 READ y1 WRITE setY1 NOTIFY y1Changed)
    Q_PROPERTY(qreal x2 READ x2 WRITE setX2 NOTIFY x2Changed)
    Q_PROPERTY(qreal y2 READ y2 WRITE setY2 NOTIFY y2Changed)
    Q_CLASSINFO("DefaultProperty", "stops")
    QML_NAMED_ELEMENT(LinearGradient)
    QML_ADDED_IN_VERSION(1, 0)

public:
    QQuickShapeLinearGradient(QObject *parent = nullptr);

    qreal x1() const;
    void setX1(qreal v);
    qreal y1() const;
    void setY1(qreal v);
    qreal x2() const;
    void setX2(qreal v);
    qreal y2() const;
    void setY2(qreal v);

Q_SIGNALS:
    void x1Changed();
    void y1Changed();
    void x2Changed();
    void y2Changed();

private:
    QPointF m_start;
    QPointF m_end;
};

class Q_QUICKSHAPES_EXPORT QQuickShapeRadialGradient : public QQuickShapeGradient
{
    Q_OBJECT
    Q_PROPERTY(qreal centerX READ centerX WRITE setCenterX NOTIFY centerXChanged)
    Q_PROPERTY(qreal centerY READ centerY WRITE setCenterY NOTIFY centerYChanged)
    Q_PROPERTY(qreal centerRadius READ centerRadius WRITE setCenterRadius NOTIFY centerRadiusChanged)
    Q_PROPERTY(qreal focalX READ focalX WRITE setFocalX NOTIFY focalXChanged)
    Q_PROPERTY(qreal focalY READ focalY WRITE setFocalY NOTIFY focalYChanged)
    Q_PROPERTY(qreal focalRadius READ focalRadius WRITE setFocalRadius NOTIFY focalRadiusChanged)
    Q_CLASSINFO("DefaultProperty", "stops")
    QML_NAMED_ELEMENT(RadialGradient)
    QML_ADDED_IN_VERSION(1, 0)

public:
    QQuickShapeRadialGradient(QObject *parent = nullptr);

    qreal centerX() const;
    void setCenterX(qreal v);

    qreal centerY() const;
    void setCenterY(qreal v);

    qreal centerRadius() const;
    void setCenterRadius(qreal v);

    qreal focalX() const;
    void setFocalX(qreal v);

    qreal focalY() const;
    void setFocalY(qreal v);

    qreal focalRadius() const;
    void setFocalRadius(qreal v);

Q_SIGNALS:
    void centerXChanged();
    void centerYChanged();
    void focalXChanged();
    void focalYChanged();
    void centerRadiusChanged();
    void focalRadiusChanged();

private:
    QPointF m_centerPoint;
    QPointF m_focalPoint;
    qreal m_centerRadius = 0;
    qreal m_focalRadius = 0;
};

class Q_QUICKSHAPES_EXPORT QQuickShapeConicalGradient : public QQuickShapeGradient
{
    Q_OBJECT
    Q_PROPERTY(qreal centerX READ centerX WRITE setCenterX NOTIFY centerXChanged)
    Q_PROPERTY(qreal centerY READ centerY WRITE setCenterY NOTIFY centerYChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_CLASSINFO("DefaultProperty", "stops")
    QML_NAMED_ELEMENT(ConicalGradient)
    QML_ADDED_IN_VERSION(1, 0)

public:
    QQuickShapeConicalGradient(QObject *parent = nullptr);

    qreal centerX() const;
    void setCenterX(qreal v);

    qreal centerY() const;
    void setCenterY(qreal v);

    qreal angle() const;
    void setAngle(qreal v);

Q_SIGNALS:
    void centerXChanged();
    void centerYChanged();
    void angleChanged();

private:
    QPointF m_centerPoint;
    qreal m_angle = 0;
};

class Q_QUICKSHAPES_EXPORT QQuickShapeTrim : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal start READ start WRITE setStart NOTIFY startChanged FINAL)
    Q_PROPERTY(qreal end READ end WRITE setEnd NOTIFY endChanged FINAL)
    Q_PROPERTY(qreal offset READ offset WRITE setOffset NOTIFY offsetChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 10)
public:
    QQuickShapeTrim(QObject *parent = nullptr);

    qreal start() const;
    void setStart(qreal t);

    qreal end() const;
    void setEnd(qreal t);

    qreal offset() const;
    void setOffset(qreal t);

Q_SIGNALS:
    void startChanged();
    void endChanged();
    void offsetChanged();

private:
    qreal m_start = 0;
    qreal m_end = 1;
    qreal m_offset = 0;
};

class Q_QUICKSHAPES_EXPORT QQuickShapePath : public QQuickPath
{
    Q_OBJECT

    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(FillRule fillRule READ fillRule WRITE setFillRule NOTIFY fillRuleChanged)
    Q_PROPERTY(JoinStyle joinStyle READ joinStyle WRITE setJoinStyle NOTIFY joinStyleChanged)
    Q_PROPERTY(int miterLimit READ miterLimit WRITE setMiterLimit NOTIFY miterLimitChanged)
    Q_PROPERTY(CapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY capStyleChanged)
    Q_PROPERTY(StrokeStyle strokeStyle READ strokeStyle WRITE setStrokeStyle NOTIFY strokeStyleChanged)
    Q_PROPERTY(qreal dashOffset READ dashOffset WRITE setDashOffset NOTIFY dashOffsetChanged)
    Q_PROPERTY(QVector<qreal> dashPattern READ dashPattern WRITE setDashPattern NOTIFY dashPatternChanged)
    Q_PROPERTY(QQuickShapeGradient *fillGradient READ fillGradient WRITE setFillGradient RESET resetFillGradient)
    Q_PROPERTY(QSizeF scale READ scale WRITE setScale NOTIFY scaleChanged REVISION(1, 14))
    Q_PROPERTY(PathHints pathHints READ pathHints WRITE setPathHints NOTIFY pathHintsChanged REVISION(6, 7) FINAL)
    Q_PROPERTY(QMatrix4x4 fillTransform READ fillTransform WRITE setFillTransform NOTIFY fillTransformChanged REVISION(6, 8) FINAL)
    Q_PROPERTY(QQuickItem *fillItem READ fillItem WRITE setFillItem NOTIFY fillItemChanged REVISION(6, 8) FINAL)
    Q_PROPERTY(QQuickShapeTrim *trim READ trim CONSTANT REVISION(6, 10) FINAL)
    QML_NAMED_ELEMENT(ShapePath)
    QML_ADDED_IN_VERSION(1, 0)

public:
    enum FillRule {
        OddEvenFill = Qt::OddEvenFill,
        WindingFill = Qt::WindingFill
    };
    Q_ENUM(FillRule)

    enum JoinStyle {
        MiterJoin = Qt::MiterJoin,
        BevelJoin = Qt::BevelJoin,
        RoundJoin = Qt::RoundJoin
    };
    Q_ENUM(JoinStyle)

    enum CapStyle {
        FlatCap = Qt::FlatCap,
        SquareCap = Qt::SquareCap,
        RoundCap = Qt::RoundCap
    };
    Q_ENUM(CapStyle)

    enum StrokeStyle {
        SolidLine = Qt::SolidLine,
        DashLine = Qt::DashLine
    };
    Q_ENUM(StrokeStyle)

    enum PathHint {
        PathLinear = 0x1,
        PathQuadratic = 0x2,
        PathConvex = 0x4,
        PathFillOnRight = 0x8,
        PathSolid = 0x10,
        PathNonIntersecting = 0x20,
        PathNonOverlappingControlPointTriangles = 0x40
    };
    Q_DECLARE_FLAGS(PathHints, PathHint)
    Q_FLAG(PathHints)

    QQuickShapePath(QObject *parent = nullptr);
    ~QQuickShapePath();

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    qreal strokeWidth() const;
    void setStrokeWidth(qreal w);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    FillRule fillRule() const;
    void setFillRule(FillRule fillRule);

    JoinStyle joinStyle() const;
    void setJoinStyle(JoinStyle style);

    int miterLimit() const;
    void setMiterLimit(int limit);

    CapStyle capStyle() const;
    void setCapStyle(CapStyle style);

    StrokeStyle strokeStyle() const;
    void setStrokeStyle(StrokeStyle style);

    qreal dashOffset() const;
    void setDashOffset(qreal offset);

    QVector<qreal> dashPattern() const;
    void setDashPattern(const QVector<qreal> &array);

    QQuickShapeGradient *fillGradient() const;
    void setFillGradient(QQuickShapeGradient *gradient);
    void resetFillGradient();

    PathHints pathHints() const;
    void setPathHints(PathHints newPathHints);

    QMatrix4x4 fillTransform() const;
    void setFillTransform(const QMatrix4x4 &matrix);

    QQuickItem *fillItem() const;
    void setFillItem(QQuickItem *newFillItem);

    QQuickShapeTrim *trim();
    bool hasTrim() const;

Q_SIGNALS:
    void shapePathChanged();
    void strokeColorChanged();
    void strokeWidthChanged();
    void fillColorChanged();
    void fillRuleChanged();
    void joinStyleChanged();
    void miterLimitChanged();
    void capStyleChanged();
    void strokeStyleChanged();
    void dashOffsetChanged();
    void dashPatternChanged();

    Q_REVISION(6, 7) void pathHintsChanged();
    Q_REVISION(6, 8) void fillTransformChanged();
    Q_REVISION(6, 8) void fillItemChanged();

private:
    Q_DISABLE_COPY(QQuickShapePath)
    Q_DECLARE_PRIVATE(QQuickShapePath)
    Q_PRIVATE_SLOT(d_func(), void _q_fillGradientChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_fillItemDestroyed())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickShapePath::PathHints)

class Q_QUICKSHAPES_EXPORT QQuickShape : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(RendererType rendererType READ rendererType NOTIFY rendererChanged)
    Q_PROPERTY(bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged)
    Q_PROPERTY(bool vendorExtensionsEnabled READ vendorExtensionsEnabled WRITE setVendorExtensionsEnabled NOTIFY vendorExtensionsEnabledChanged)
    Q_PROPERTY(RendererType preferredRendererType READ preferredRendererType
               WRITE setPreferredRendererType NOTIFY preferredRendererTypeChanged REVISION(6, 6) FINAL)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(ContainsMode containsMode READ containsMode WRITE setContainsMode NOTIFY containsModeChanged REVISION(1, 11))
    Q_PROPERTY(QRectF boundingRect READ boundingRect NOTIFY boundingRectChanged REVISION(6, 6) FINAL)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged REVISION(6, 7) FINAL)
    Q_PROPERTY(HAlignment horizontalAlignment READ horizontalAlignment WRITE setHorizontalAlignment NOTIFY horizontalAlignmentChanged REVISION(6, 7) FINAL)
    Q_PROPERTY(VAlignment verticalAlignment READ verticalAlignment WRITE setVerticalAlignment NOTIFY verticalAlignmentChanged REVISION(6, 7) FINAL)

    Q_PROPERTY(QQmlListProperty<QObject> data READ data)
    Q_CLASSINFO("DefaultProperty", "data")
    QML_NAMED_ELEMENT(Shape)
    QML_ADDED_IN_VERSION(1, 0)

public:
    enum RendererType {
        UnknownRenderer,
        GeometryRenderer,
        NvprRenderer,
        SoftwareRenderer,
        CurveRenderer
    };
    Q_ENUM(RendererType)

    enum Status {
        Null,
        Ready,
        Processing
    };
    Q_ENUM(Status)

    enum ContainsMode {
        BoundingRectContains,
        FillContains
    };
    Q_ENUM(ContainsMode)

    enum FillMode {
        NoResize,
        PreserveAspectFit,
        PreserveAspectCrop,
        Stretch
    };
    Q_ENUM(FillMode)

    enum HAlignment { AlignLeft = Qt::AlignLeft,
                      AlignRight = Qt::AlignRight,
                      AlignHCenter = Qt::AlignHCenter };
    Q_ENUM(HAlignment)
    enum VAlignment { AlignTop = Qt::AlignTop,
                      AlignBottom = Qt::AlignBottom,
                      AlignVCenter = Qt::AlignVCenter };
    Q_ENUM(VAlignment)

    QQuickShape(QQuickItem *parent = nullptr);
    ~QQuickShape();

    RendererType rendererType() const;

    bool asynchronous() const;
    void setAsynchronous(bool async);

    Q_REVISION(6, 6) RendererType preferredRendererType() const;
    Q_REVISION(6, 6) void setPreferredRendererType(RendererType preferredType);

    Q_REVISION(6, 6) QRectF boundingRect() const override;

    bool vendorExtensionsEnabled() const;
    void setVendorExtensionsEnabled(bool enable);

    Status status() const;

    ContainsMode containsMode() const;
    void setContainsMode(ContainsMode containsMode);

    bool contains(const QPointF &point) const override;

    QQmlListProperty<QObject> data();

    Q_REVISION(6, 7) FillMode fillMode() const;
    Q_REVISION(6, 7) void setFillMode(FillMode newFillMode);

    Q_REVISION(6, 7) HAlignment horizontalAlignment() const;
    Q_REVISION(6, 7) void setHorizontalAlignment(HAlignment newHorizontalAlignment);

    Q_REVISION(6, 7) VAlignment verticalAlignment() const;
    Q_REVISION(6, 7) void setVerticalAlignment(VAlignment newVerticalAlignment);

protected:
    QQuickShape(QQuickShapePrivate &dd, QQuickItem *parent);

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override;
    void updatePolish() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void componentComplete() override;
    void classBegin() override;

Q_SIGNALS:
    void rendererChanged();
    void asynchronousChanged();
    void vendorExtensionsEnabledChanged();
    void statusChanged();
    Q_REVISION(6, 6) void preferredRendererTypeChanged();
    Q_REVISION(6, 6) void boundingRectChanged();
    Q_REVISION(1, 11) void containsModeChanged();

    Q_REVISION(6, 7) void fillModeChanged();
    Q_REVISION(6, 7) void horizontalAlignmentChanged();
    Q_REVISION(6, 7) void verticalAlignmentChanged();

private:
    Q_DISABLE_COPY(QQuickShape)
    Q_DECLARE_PRIVATE(QQuickShape)
    Q_PRIVATE_SLOT(d_func(), void _q_shapePathChanged())
};

QT_END_NAMESPACE

#endif // QQUICKSHAPE_P_H
