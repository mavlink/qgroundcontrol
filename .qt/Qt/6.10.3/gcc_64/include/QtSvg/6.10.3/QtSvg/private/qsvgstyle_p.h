// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGSTYLE_P_H
#define QSVGSTYLE_P_H

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

#include "QtCore/qstack.h"
#include "QtGui/qpainter.h"
#include "QtGui/qpen.h"
#include "QtGui/qbrush.h"
#include "QtGui/qtransform.h"
#include "QtGui/qcolor.h"
#include "QtGui/qfont.h"
#include <qdebug.h>
#include "qtsvgglobal_p.h"

QT_BEGIN_NAMESPACE

class QPainter;
class QSvgNode;
class QSvgFont;
class QSvgTinyDocument;
class QSvgPattern;

template <class T> class QSvgRefCounter
{
public:
    QSvgRefCounter() { t = nullptr; }
    QSvgRefCounter(T *_t)
    {
        t = _t;
        if (t)
            t->ref();
    }
    QSvgRefCounter(const QSvgRefCounter &other)
    {
        t = other.t;
        if (t)
            t->ref();
    }
    QSvgRefCounter &operator =(T *_t)
    {
        if(_t)
            _t->ref();
        if (t)
            t->deref();
        t = _t;
        return *this;
    }
    QSvgRefCounter &operator =(const QSvgRefCounter &other)
    {
        if(other.t)
            other.t->ref();
        if (t)
            t->deref();
        t = other.t;
        return *this;
    }
    ~QSvgRefCounter()
    {
        if (t)
            t->deref();
    }

    inline T *operator->() const { return t; }
    inline operator T*() const { return t; }

    inline bool isDefault() const { return !t || t->isDefault(); }

private:
    T *t;
};

class Q_SVG_EXPORT QSvgRefCounted
{
public:
    QSvgRefCounted() { _ref = 0; }
    virtual ~QSvgRefCounted() {}
    void ref() {
        ++_ref;
//        qDebug() << this << ": adding ref, now " << _ref;
    }
    void deref() {
//        qDebug() << this << ": removing ref, now " << _ref;
        if(!--_ref) {
//            qDebug("     deleting");
            delete this;
        }
    }
private:
    int _ref;
};

struct Q_SVG_EXPORT QSvgExtraStates
{
    QSvgExtraStates();

    qreal fillOpacity;
    qreal strokeOpacity;
    QSvgFont *svgFont;
    Qt::Alignment textAnchor;
    int fontWeight;
    Qt::FillRule fillRule;
    qreal strokeDashOffset;
    int nestedUseLevel = 0;
    int nestedUseCount = 0;
    bool vectorEffect; // true if pen is cosmetic
    qint8 imageRendering; // QSvgQualityStyle::ImageRendering
    bool inUse = false; // true if currently in QSvgUseNode
};

class Q_SVG_EXPORT QSvgStyleProperty : public QSvgRefCounted
{
public:
    enum Type
    {
        QUALITY,
        FILL,
        VIEWPORT_FILL,
        FONT,
        STROKE,
        SOLID_COLOR,
        GRADIENT,
        PATTERN,
        TRANSFORM,
        ANIMATE_TRANSFORM,
        ANIMATE_COLOR,
        OPACITY,
        COMP_OP
    };
public:
    virtual ~QSvgStyleProperty();
    virtual void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) = 0;
    virtual void revert(QPainter *p, QSvgExtraStates &states) =0;
    virtual Type type() const=0;
    bool isDefault() const { return false; } // [not virtual since called from templated class]
};

class Q_SVG_EXPORT QSvgPaintStyleProperty : public QSvgStyleProperty
{
public:
    virtual QBrush brush(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) = 0;
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
};

class Q_SVG_EXPORT QSvgQualityStyle : public QSvgStyleProperty
{
public:
    enum ImageRendering: qint8 {
        ImageRenderingAuto = 0,
        ImageRenderingOptimizeSpeed = 1,
        ImageRenderingOptimizeQuality = 2,
    };

    QSvgQualityStyle(int color);
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setImageRendering(ImageRendering);
private:
    // color-render ing v 	v 	'auto' | 'optimizeSpeed' |
    //                                  'optimizeQuality' | 'inherit'
    //int m_colorRendering;

    // shape-rendering v 	v 	'auto' | 'optimizeSpeed' | 'crispEdges' |
    //                                  'geometricPrecision' | 'inherit'
    //QSvgShapeRendering m_shapeRendering;


    // text-rendering    v 	v 	'auto' | 'optimizeSpeed' | 'optimizeLegibility'
    //                                | 'geometricPrecision' | 'inherit'
    //QSvgTextRendering m_textRendering;


    // vector-effect         v 	x 	'default' | 'non-scaling-stroke' | 'inherit'
    //QSvgVectorEffect m_vectorEffect;

    // image-rendering v 	v 	'auto' | 'optimizeSpeed' | 'optimizeQuality' |
    //                                      'inherit'
    qint32 m_imageRendering: 4;
    qint32 m_oldImageRendering: 4;
    quint32 m_imageRenderingSet: 1;
};



class Q_SVG_EXPORT QSvgOpacityStyle : public QSvgStyleProperty
{
public:
    QSvgOpacityStyle(qreal opacity);
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    qreal opacity() const { return m_opacity; }
    bool isDefault() const { return qFuzzyCompare(m_opacity, qreal(1.0)); }

private:
    qreal m_opacity;
    qreal m_oldOpacity;
};

class Q_SVG_EXPORT QSvgFillStyle : public QSvgStyleProperty
{
public:
    QSvgFillStyle();
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setFillRule(Qt::FillRule f);
    void setFillOpacity(qreal opacity);
    void setFillStyle(QSvgPaintStyleProperty* style);
    void setBrush(QBrush brush);

    const QBrush & qbrush() const
    {
        return m_fill;
    }

    qreal fillOpacity() const
    {
        return m_fillOpacity;
    }

    Qt::FillRule fillRule() const
    {
        return m_fillRule;
    }

    QSvgPaintStyleProperty* style() const
    {
        return m_style;
    }

    void setPaintStyleId(const QString &Id)
    {
        m_paintStyleId = Id;
    }

    QString paintStyleId() const
    {
        return m_paintStyleId;
    }

    void setPaintStyleResolved(bool resolved)
    {
        m_paintStyleResolved = resolved;
    }

    bool isPaintStyleResolved() const
    {
        return m_paintStyleResolved;
    }

private:
    // fill            v 	v 	'inherit' | <Paint.datatype>
    // fill-opacity    v 	v 	'inherit' | <OpacityValue.datatype>
    QBrush m_fill;
    QBrush m_oldFill;
    QSvgPaintStyleProperty *m_style;

    Qt::FillRule m_fillRule;
    Qt::FillRule m_oldFillRule;
    qreal m_fillOpacity;
    qreal m_oldFillOpacity;

    QString m_paintStyleId;
    uint m_paintStyleResolved : 1;

    uint m_fillRuleSet : 1;
    uint m_fillOpacitySet : 1;
    uint m_fillSet : 1;
};

class Q_SVG_EXPORT QSvgViewportFillStyle : public QSvgStyleProperty
{
public:
    QSvgViewportFillStyle(const QBrush &brush);
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    const QBrush & qbrush() const
    {
        return m_viewportFill;
    }
private:
    // viewport-fill         v 	x 	'inherit' | <Paint.datatype>
    // viewport-fill-opacity 	v 	x 	'inherit' | <OpacityValue.datatype>
    QBrush m_viewportFill;

    QBrush m_oldFill;
};

class Q_SVG_EXPORT QSvgFontStyle : public QSvgStyleProperty
{
public:
    static const int LIGHTER = -1;
    static const int BOLDER = 1;

    QSvgFontStyle(QSvgFont *font, QSvgTinyDocument *doc);
    QSvgFontStyle();
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setSize(qreal size)
    {
        // Store the _pixel_ size in the font. Since QFont::setPixelSize() only takes an int, call
        // QFont::SetPointSize() instead. Set proper font size just before rendering.
        m_qfont.setPointSizeF(size);
        m_sizeSet = 1;
    }

    void setTextAnchor(Qt::Alignment anchor)
    {
        m_textAnchor = anchor;
        m_textAnchorSet = 1;
    }

    void setFamily(const QString &family)
    {
        m_qfont.setFamilies({family});
        m_familySet = 1;
    }

    void setStyle(QFont::Style fontStyle) {
        m_qfont.setStyle(fontStyle);
        m_styleSet = 1;
    }

    void setVariant(QFont::Capitalization fontVariant)
    {
        m_qfont.setCapitalization(fontVariant);
        m_variantSet = 1;
    }

    void setWeight(int weight)
    {
        m_weight = weight;
        m_weightSet = 1;
    }

    QSvgFont * svgFont() const
    {
        return m_svgFont;
    }

    const QFont &qfont() const
    {
        return m_qfont;
    }

    QSvgTinyDocument *doc() const {return m_doc;}

private:
    QSvgFont *m_svgFont;
    QSvgTinyDocument *m_doc;
    QFont m_qfont;

    int m_weight;
    Qt::Alignment m_textAnchor;

    QSvgFont *m_oldSvgFont;
    QFont m_oldQFont;
    Qt::Alignment m_oldTextAnchor;
    int m_oldWeight;

    uint m_familySet : 1;
    uint m_sizeSet : 1;
    uint m_styleSet : 1;
    uint m_variantSet : 1;
    uint m_weightSet : 1;
    uint m_textAnchorSet : 1;
};

class Q_SVG_EXPORT QSvgStrokeStyle : public QSvgStyleProperty
{
public:
    QSvgStrokeStyle();
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setStroke(QBrush brush)
    {
        m_stroke.setBrush(brush);
        m_style = nullptr;
        m_strokeSet = 1;
    }

    void setStyle(QSvgPaintStyleProperty *style)
    {
        m_style = style;
        m_strokeSet = 1;
    }

    void setDashArray(const QList<qreal> &dashes);

    void setDashArrayNone()
    {
        m_stroke.setStyle(Qt::SolidLine);
        m_strokeDashArraySet = 1;
    }

    void setDashOffset(qreal offset)
    {
        m_strokeDashOffset = offset;
        m_strokeDashOffsetSet = 1;
    }

    void setLineCap(Qt::PenCapStyle cap)
    {
        m_stroke.setCapStyle(cap);
        m_strokeLineCapSet = 1;
    }

    void setLineJoin(Qt::PenJoinStyle join)
    {
        m_stroke.setJoinStyle(join);
        m_strokeLineJoinSet = 1;
    }

    void setMiterLimit(qreal limit)
    {
        m_stroke.setMiterLimit(limit);
        m_strokeMiterLimitSet = 1;
    }

    void setOpacity(qreal opacity)
    {
        m_strokeOpacity = opacity;
        m_strokeOpacitySet = 1;
    }

    void setWidth(qreal width)
    {
        m_stroke.setWidthF(width);
        m_strokeWidthSet = 1;
        Q_ASSERT(!m_strokeDashArraySet); // set width before dash array.
    }

    qreal width()
    {
        return m_stroke.widthF();
    }

    void setVectorEffect(bool nonScalingStroke)
    {
        m_vectorEffect = nonScalingStroke;
        m_vectorEffectSet = 1;
    }

    QSvgPaintStyleProperty* style() const
    {
        return m_style;
    }

    void setPaintStyleId(const QString &Id)
    {
        m_paintStyleId = Id;
    }

    QString paintStyleId() const
    {
        return m_paintStyleId;
    }

    void setPaintStyleResolved(bool resolved)
    {
        m_paintStyleResolved = resolved;
    }

    bool isPaintStyleResolved() const
    {
        return m_paintStyleResolved;
    }

    QPen stroke() const
    {
        return m_stroke;
    }

private:
    // stroke            v 	v 	'inherit' | <Paint.datatype>
    // stroke-dasharray  v 	v 	'inherit' | <StrokeDashArrayValue.datatype>
    // stroke-dashoffset v 	v 	'inherit' | <StrokeDashOffsetValue.datatype>
    // stroke-linecap    v 	v 	'butt' | 'round' | 'square' | 'inherit'
    // stroke-linejoin   v 	v 	'miter' | 'round' | 'bevel' | 'inherit'
    // stroke-miterlimit v 	v 	'inherit' | <StrokeMiterLimitValue.datatype>
    // stroke-opacity    v 	v 	'inherit' | <OpacityValue.datatype>
    // stroke-width      v 	v 	'inherit' | <StrokeWidthValue.datatype>
    QPen m_stroke;
    QPen m_oldStroke;
    qreal m_strokeOpacity;
    qreal m_oldStrokeOpacity;
    qreal m_strokeDashOffset;
    qreal m_oldStrokeDashOffset;

    QSvgPaintStyleProperty *m_style;
    QString m_paintStyleId;
    uint m_paintStyleResolved : 1;
    uint m_vectorEffect : 1;
    uint m_oldVectorEffect : 1;

    uint m_strokeSet : 1;
    uint m_strokeDashArraySet : 1;
    uint m_strokeDashOffsetSet : 1;
    uint m_strokeLineCapSet : 1;
    uint m_strokeLineJoinSet : 1;
    uint m_strokeMiterLimitSet : 1;
    uint m_strokeOpacitySet : 1;
    uint m_strokeWidthSet : 1;
    uint m_vectorEffectSet : 1;
};

class Q_SVG_EXPORT QSvgSolidColorStyle : public QSvgPaintStyleProperty
{
public:
    QSvgSolidColorStyle(const QColor &color);
    Type type() const override;

    const QColor & qcolor() const
    {
        return m_solidColor;
    }

    QBrush brush(QPainter *, const QSvgNode *, QSvgExtraStates &) override
    {
        return m_solidColor;
    }

private:
    // solid-color       v 	x 	'inherit' | <SVGColor.datatype>
    // solid-opacity     v 	x 	'inherit' | <OpacityValue.datatype>
    QColor m_solidColor;

    QBrush m_oldFill;
    QPen   m_oldStroke;
};

class Q_SVG_EXPORT QSvgGradientStyle : public QSvgPaintStyleProperty
{
public:
    QSvgGradientStyle(QGradient *grad);
    ~QSvgGradientStyle() { delete m_gradient; }
    Type type() const override;

    void setStopLink(const QString &link, QSvgTinyDocument *doc);
    QString stopLink() const { return m_link; }
    void resolveStops();
    void resolveStops_helper(QStringList *visited);

    void setTransform(const QTransform &transform);
    QTransform qtransform() const
    {
        return m_transform;
    }

    QGradient *qgradient() const
    {
        return m_gradient;
    }

    bool gradientStopsSet() const
    {
        return m_gradientStopsSet;
    }

    void setGradientStopsSet(bool set)
    {
        m_gradientStopsSet = set;
    }

    QBrush brush(QPainter *, const QSvgNode *, QSvgExtraStates &) override;
private:
    QGradient      *m_gradient;
    QTransform m_transform;

    QSvgTinyDocument *m_doc;
    QString           m_link;
    bool m_gradientStopsSet;
};

class Q_SVG_EXPORT QSvgPatternStyle : public QSvgPaintStyleProperty
{
public:
    QSvgPatternStyle(QSvgPattern *pattern);
    ~QSvgPatternStyle() = default;
    Type type() const override;

    QBrush brush(QPainter *, const QSvgNode *, QSvgExtraStates &) override;
    QSvgPattern *patternNode() { return m_pattern; }
private:
    QSvgPattern *m_pattern;
    QImage m_patternImage;
    QRectF m_parentBound;
};


class Q_SVG_EXPORT QSvgTransformStyle : public QSvgStyleProperty
{
public:
    QSvgTransformStyle(const QTransform &transform);
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    const QTransform & qtransform() const
    {
        return m_transform;
    }
    bool isDefault() const { return m_transform.isIdentity(); }
private:
    //7.6 The transform  attribute
    QTransform m_transform;
    QStack<QTransform> m_oldWorldTransform;
};

class Q_SVG_EXPORT QSvgCompOpStyle : public QSvgStyleProperty
{
public:
    QSvgCompOpStyle(QPainter::CompositionMode mode);
    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    const QPainter::CompositionMode & compOp() const
    {
        return m_mode;
    }
private:
    //comp-op attribute
    QPainter::CompositionMode m_mode;

    QPainter::CompositionMode m_oldMode{QPainter::CompositionMode_SourceOver};
};

class Q_SVG_EXPORT QSvgStaticStyle
{
public:
    QSvgStaticStyle();
    ~QSvgStaticStyle();

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states);
    void revert(QPainter *p, QSvgExtraStates &states);
    QSvgRefCounter<QSvgQualityStyle>      quality;
    QSvgRefCounter<QSvgFillStyle>         fill;
    QSvgRefCounter<QSvgViewportFillStyle> viewportFill;
    QSvgRefCounter<QSvgFontStyle>         font;
    QSvgRefCounter<QSvgStrokeStyle>       stroke;
    QSvgRefCounter<QSvgSolidColorStyle>   solidColor;
    QSvgRefCounter<QSvgGradientStyle>     gradient;
    QSvgRefCounter<QSvgPatternStyle>      pattern;
    QSvgRefCounter<QSvgTransformStyle>    transform;
    QSvgRefCounter<QSvgOpacityStyle>      opacity;
    QSvgRefCounter<QSvgCompOpStyle>       compop;
};

class QSvgAbstractAnimatedProperty;
class Q_SVG_EXPORT QSvgAnimatedStyle
{
public:
    QSvgAnimatedStyle();
    ~QSvgAnimatedStyle();

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states);
    void revert(QPainter *p, QSvgExtraStates &states);

private:
    void savePaintingState(const QPainter *p, const QSvgNode *node, QSvgExtraStates &states);
    void applyPropertyAnimation(QPainter *p, QSvgAbstractAnimatedProperty *property, bool replace, QSvgExtraStates &states);

private:
    QBrush m_brush;
    QPen m_pen;
    QTransform m_worldTransform;
    QTransform m_transformToNode;
    qreal m_fillOpacity = 1.0;
    qreal m_strokeOpacity = 1.0;
    qreal m_opacity = 1.0;
};

/********************************************************/
// NOT implemented:

// color           v 	v 	'inherit' | <Color.datatype>
//QColor m_color;

// display         v 	x 	'inline' | 'block' | 'list-item'
//                                 | 'run-in' | 'compact' | 'marker' |
//                                 'table' | 'inline-table' |
//                                 'table-row-group' | 'table-header-group' |
//                                 'table-footer-group' | 'table-row' |
//                                 'table-column-group' | 'table-column' |
//                                 'table-cell' | 'table-caption' |
//                                 'none' | 'inherit'
//QSvgDisplayStyle m_display;

// display-align   v 	v 	'auto' | 'before' | 'center' | 'after' | 'inherit'
//QSvgDisplayAlign m_displayAlign;

// line-increment  v 	v 	'auto' | 'inherit' | <Number.datatype>
//int m_lineIncrement;

// text-anchor       v 	v 	'start' | 'middle' | 'end' | 'inherit'
//QSvgTextAnchor m_textAnchor;

// visibility 	v 	v 	'visible' | 'hidden' | 'inherit'
//QSvgVisibility m_visibility;

/******************************************************/
// the following do not make sense for us

// pointer-events  v 	v 	'visiblePainted' | 'visibleFill' | 'visibleStroke' |
//                              'visible' | 'painted' | 'fill' | 'stroke' | 'all' |
//                              'none' | 'inherit'
//QSvgPointEvents m_pointerEvents;

// audio-level     v  	x  	'inherit' | <Number.datatype>

QT_END_NAMESPACE

#endif // QSVGSTYLE_P_H
