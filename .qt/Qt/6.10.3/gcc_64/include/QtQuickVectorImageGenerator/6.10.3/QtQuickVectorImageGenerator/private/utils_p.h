// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef UTILS_P_H
#define UTILS_P_H

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

#include <private/qquicktranslate_p.h>
#include <private/qquickitem_p.h>
#include <private/qsvgnode_p.h>

#include <private/qquadpath_p.h>
#include <private/qsvgvisitor_p.h>

QT_BEGIN_NAMESPACE

namespace QQuickVectorImageGenerator::Utils
{

class ViewBoxItem : public QQuickItem
{
public:
    ViewBoxItem(const QRectF viewBox, QQuickItem *parent = nullptr) : QQuickItem(parent), m_viewBox(viewBox) { setXForm(); }

protected:
    void geometryChange(const QRectF &/*newGeometry*/, const QRectF &/*oldGeometry*/) override
    {
        setXForm();
    }

private:
    void setXForm()
    {
        auto xformProp = transform();
        xformProp.clear(&xformProp);
        bool translate = !qFuzzyIsNull(m_viewBox.x()) || !qFuzzyIsNull(m_viewBox.y());
        if (translate) {
            auto *tr = new QQuickTranslate(this);
            tr->setX(-m_viewBox.x());
            tr->setY(-m_viewBox.y());
            xformProp.append(&xformProp, tr);
        }
        if (!m_viewBox.isEmpty() && width() && height()) {
            auto *scale = new QQuickScale(this);
            qreal sx = width() / m_viewBox.width();
            qreal sy = height() / m_viewBox.height();

            scale->setXScale(sx);
            scale->setYScale(sy);
            xformProp.append(&xformProp, scale);
        }
    }
    QRectF m_viewBox;
};

inline QPainterPath polygonToPath(const QPolygonF &poly, bool closed)
{
    QPainterPath path;
    if (poly.isEmpty())
        return path;
    bool first = true;
    for (const auto &p : poly) {
        if (first)
            path.moveTo(p);
        else
            path.lineTo(p);
        first = false;
    }
    if (closed)
        path.closeSubpath();
    return path;
}

inline QString pathHintString(const QQuadPath &qp)
{
    QString res;
    QTextStream str(&res);
    auto flags = qp.pathHints();
    if (!flags)
        return res;
    str << "pathHints:";
    bool first = true;

#define CHECK_PATH_HINT(flagName)              \
    if (flags.testFlag(QQuadPath::flagName)) { \
            if (!first)                            \
            str << " |";                       \
            first = false;                         \
            str << " ShapePath." #flagName;        \
    }

    CHECK_PATH_HINT(PathLinear)
    CHECK_PATH_HINT(PathQuadratic)
    CHECK_PATH_HINT(PathConvex)
    CHECK_PATH_HINT(PathFillOnRight)
    CHECK_PATH_HINT(PathSolid)
    CHECK_PATH_HINT(PathNonIntersecting)
    CHECK_PATH_HINT(PathNonOverlappingControlPointTriangles)

    return res;
}

// Find the square that gives the same gradient in QGradient::LogicalMode as
// objModeRect does in QGradient::ObjectMode

// When the object's bounding box is not square, the stripes that are conceptually
// perpendicular to the gradient vector within object bounding box space shall render
// non-perpendicular relative to the gradient vector in user space due to application
// of the non-uniform scaling transformation from bounding box space to user space.
inline QRectF mapToQtLogicalMode(const QRectF &objModeRect, const QRectF &boundingRect)
{

    QRect pixelRect(objModeRect.x() * boundingRect.width() + boundingRect.left(),
                    objModeRect.y() * boundingRect.height() + boundingRect.top(),
                    objModeRect.width() * boundingRect.width(),
                    objModeRect.height() * boundingRect.height());

    if (pixelRect.isEmpty()) // pure horizontal/vertical gradient
        return pixelRect;

    double w = boundingRect.width();
    double h = boundingRect.height();
    double objModeSlope = objModeRect.height() / objModeRect.width();
    double a = objModeSlope * w / h;

    // do calculation with origin == pixelRect.topLeft
    double x2 = pixelRect.width();
    double y2 = pixelRect.height();
    double x = (x2 + a * y2) / (1 + a * a);
    double y = y2 - (x - x2)/a;

    return QRectF(pixelRect.topLeft(), QSizeF(x,y));
}

inline QString toSvgString(const QPainterPath &path)
{
    QString svgPathString;
    QTextStream strm(&svgPathString);

    for (int i = 0; i < path.elementCount(); ++i) {
        QPainterPath::Element element = path.elementAt(i);
        if (element.isMoveTo()) {
            strm << "M " << element.x << " " << element.y << " ";
        } else if (element.isLineTo()) {
            strm << "L " << element.x << " " << element.y << " ";
        } else if (element.isCurveTo()) {
            QPointF c1(element.x, element.y);
            ++i;
            element = path.elementAt(i);

            QPointF c2(element.x, element.y);
            ++i;
            element = path.elementAt(i);
            QPointF ep(element.x, element.y);

            strm <<  "C "
                 <<  c1.x() << " "
                 <<  c1.y() << " "
                 <<  c2.x() << " "
                 <<  c2.y() << " "
                 <<  ep.x() << " "
                 <<  ep.y() << " ";
        }
    }

    return svgPathString;
}

inline QString toSvgString(const QQuadPath &path)
{
    QString svgPathString;
    QTextStream strm(&svgPathString);
    path.iterateElements([&](const QQuadPath::Element &e, int) {
        if (e.isSubpathStart())
            strm << "M " << e.startPoint().x() << " " << e.startPoint().y() << " ";
        if (e.isLine())
            strm << "L " << e.endPoint().x() << " " << e.endPoint().y() << " ";
        else
            strm << "Q " << e.controlPoint().x() << " " << e.controlPoint().y() << " "
                 << e.endPoint().x() << " " << e.endPoint().y() << " ";
    });

    return svgPathString;
}

inline QString strokeCapStyleString(Qt::PenCapStyle strokeCapStyle)
{
    QString capStyle;
    switch (strokeCapStyle) {
    case Qt::FlatCap:
        capStyle = QStringLiteral("ShapePath.FlatCap");
        break;
    case Qt::SquareCap:
        capStyle = QStringLiteral("ShapePath.SquareCap");
        break;
    case Qt::RoundCap:
        capStyle = QStringLiteral("ShapePath.RoundCap");
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    return capStyle;
}

inline QString strokeJoinStyleString(Qt::PenJoinStyle strokeJoinStyle)
{
    QString joinStyle;
    switch (strokeJoinStyle) {
    case Qt::MiterJoin:
        joinStyle = QStringLiteral("ShapePath.MiterJoin");
        break;
    case Qt::BevelJoin:
        joinStyle = QStringLiteral("ShapePath.BevelJoin");
        break;
    case Qt::RoundJoin:
        joinStyle = QStringLiteral("ShapePath.RoundJoin");
        break;
    default:
        //TODO: Add support for SvgMiter case
        Q_UNREACHABLE();
        break;
    }

    return joinStyle;
}

template<typename T>
inline QString listString(QList<T> list)
{
    if (list.isEmpty())
        return QStringLiteral("[]");

    QString listString;
    QTextStream stream(&listString);
    stream << "[";

    if (list.length() > 1) {
        for (int i = 0; i < list.length() - 1; i++) {
            T v = list[i];
            stream << v << ", ";
        }
    }

    stream << list.last() << "]";
    return listString;
}

}

QT_END_NAMESPACE

#endif // UTILS_P_H
