// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUADPATH_P_H
#define QQUADPATH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qrect.h>
#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qpainterpath.h>
#include <QtQuick/qtquickexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuadPath
{
public:
    // This is a copy of the flags in QQuickShapePath ### TODO: use a common definition
    enum PathHint : quint8 {
        PathLinear = 0x1,
        PathQuadratic = 0x2,
        PathConvex = 0x4,
        PathFillOnRight = 0x8,
        PathSolid = 0x10,
        PathNonIntersecting = 0x20,
        PathNonOverlappingControlPointTriangles = 0x40
    };
    Q_DECLARE_FLAGS(PathHints, PathHint)

    class Q_QUICK_EXPORT Element
    {
    public:
        Element ()
            : m_isSubpathStart(false), m_isSubpathEnd(false), m_isLine(false)
        {
        }

        Element (QVector2D s, QVector2D c, QVector2D e)
            : sp(s), cp(c), ep(e), m_isSubpathStart(false), m_isSubpathEnd(false), m_isLine(false)
        {
        }

        bool isSubpathStart() const
        {
            return m_isSubpathStart;
        }

        bool isSubpathEnd() const
        {
            return m_isSubpathEnd;
        }

        bool isLine() const
        {
            return m_isLine;
        }

        bool isConvex() const
        {
            return m_curvatureFlags & Convex;
        }

        QVector2D startPoint() const
        {
            return sp;
        }

        QVector2D controlPoint() const
        {
            return cp;
        }

        QVector2D endPoint() const
        {
            return ep;
        }

        QVector2D midPoint() const
        {
            return isLine() ? 0.5f * (sp + ep) : (0.25f * sp) + (0.5f * cp) + (0.25 * ep);
        }

        /* For a curve, returns the control point. For a line, returns an arbitrary point on the
         * inside side of the line (assuming the curvature has been set for the path). The point
         * doesn't need to actually be inside the shape: it just makes for easier calculations
         * later when it is at the same side as the fill. */
        QVector2D referencePoint() const
        {
            if (isLine()) {
                QVector2D normal(sp.y() - ep.y(), ep.x() - sp.x());
                return m_curvatureFlags & Element::FillOnRight ? sp + normal : sp - normal;
            } else {
                return cp;
            }
        }

        Element segmentFromTo(float t0, float t1) const;

        Element reversed() const;

        int childCount() const { return m_numChildren; }

        int indexOfChild(int childNumber) const
        {
            Q_ASSERT(childNumber >= 0 && childNumber < childCount());
            return -(m_firstChildIndex + 1 + childNumber);
        }

        QVector2D pointAtFraction(float t) const;

        QVector2D tangentAtFraction(float t) const
        {
            return isLine() ? (ep - sp) : ((1 - t) * 2 * (cp - sp)) + (t * 2 * (ep - cp));
        }

        QVector2D normalAtFraction(float t) const
        {
            const QVector2D tan = tangentAtFraction(t);
            return QVector2D(-tan.y(), tan.x());
        }

        float extent() const;

        void setAsConvex(bool isConvex)
        {
            if (isConvex)
                m_curvatureFlags = Element::CurvatureFlags(m_curvatureFlags | Element::Convex);
            else
                m_curvatureFlags = Element::CurvatureFlags(m_curvatureFlags & ~Element::Convex);
        }

        void setFillOnRight(bool isFillOnRight)
        {
            if (isFillOnRight)
                m_curvatureFlags = Element::CurvatureFlags(m_curvatureFlags | Element::FillOnRight);
            else
                m_curvatureFlags = Element::CurvatureFlags(m_curvatureFlags & ~Element::FillOnRight);
        }

        bool isFillOnRight() const { return m_curvatureFlags & FillOnRight; }

        bool isControlPointOnLeft() const
        {
            return isPointOnLeft(cp, sp, ep);
        }

        enum CurvatureFlags : quint8 {
            CurvatureUndetermined = 0,
            FillOnRight = 1,
            Convex = 2
        };

        enum FillSide : quint8 {
            FillSideUndetermined = 0,
            FillSideRight = 1,
            FillSideLeft = 2,
            FillSideBoth = 3
        };

    private:
        int intersectionsAtY(float y, float *fractions, bool swapXY = false) const;

        QVector2D sp;
        QVector2D cp;
        QVector2D ep;
        int m_firstChildIndex = 0;
        quint8 m_numChildren = 0;
        CurvatureFlags m_curvatureFlags = CurvatureUndetermined;
        quint8 m_isSubpathStart : 1;
        quint8 m_isSubpathEnd : 1;
        quint8 m_isLine : 1;
        friend class QQuadPath;
        friend Q_QUICK_EXPORT QDebug operator<<(QDebug, const QQuadPath::Element &);
    };

    void moveTo(const QVector2D &to)
    {
        m_subPathToStart = true;
        m_currentPoint = to;
    }

    void lineTo(const QVector2D &to)
    {
        addElement({}, to, true);
    }

    void quadTo(const QVector2D &control, const QVector2D &to)
    {
        addElement(control, to);
    }

    Element &elementAt(int i)
    {
        return i < 0 ? m_childElements[-(i + 1)] : m_elements[i];
    }

    const Element &elementAt(int i) const
    {
        return i < 0 ? m_childElements[-(i + 1)] : m_elements[i];
    }

    int indexOfChildAt(int i, int childNumber) const
    {
        return elementAt(i).indexOfChild(childNumber);
    }

    QRectF controlPointRect() const;

    Qt::FillRule fillRule() const { return m_windingFill ? Qt::WindingFill : Qt::OddEvenFill; }
    void setFillRule(Qt::FillRule rule) { m_windingFill = (rule == Qt::WindingFill); }

    void reserve(int size) { m_elements.reserve(size); }
    int elementCount() const { return m_elements.size(); }
    bool isEmpty() const { return m_elements.size() == 0; }
    int elementCountRecursive() const;
    int lineCount() const
    {
        return std::count_if(m_elements.cbegin(), m_elements.cend(),
                             [](const Element &e) { return e.isLine(); });
    }

    static QQuadPath fromPainterPath(const QPainterPath &path, PathHints hints = {});
    QPainterPath toPainterPath() const;
    QString asSvgString() const;

    QQuadPath subPathsClosed(bool *didClose = nullptr) const;
    void addCurvatureData();
    QQuadPath flattened() const;
    QQuadPath dashed(qreal lineWidth, const QList<qreal> &dashPattern, qreal dashOffset = 0) const;
    void splitElementAt(int index);
    bool contains(const QVector2D &point) const;
    bool contains(const QVector2D &point, int fromIndex, int toIndex) const;
    Element::FillSide fillSideOf(int elementIdx, float elementT) const;

    template<typename Func>
    void iterateChildrenOf(Element &e, Func &&lambda)
    {
        const int lastChildIndex = e.m_firstChildIndex + e.childCount() - 1;
        for (int i = e.m_firstChildIndex; i <= lastChildIndex; i++) {
            Element &c = m_childElements[i];
            if (c.childCount() > 0)
                iterateChildrenOf(c, lambda);
            else
                lambda(c, -(i + 1));
        }
    }

    template<typename Func>
    void iterateChildrenOf(const Element &e, Func &&lambda) const
    {
        const int lastChildIndex = e.m_firstChildIndex + e.childCount() - 1;
        for (int i = e.m_firstChildIndex; i <= lastChildIndex; i++) {
            const Element &c = m_childElements[i];
            if (c.childCount() > 0)
                iterateChildrenOf(c, lambda);
            else
                lambda(c, -(i + 1));
        }
    }

    template<typename Func>
    void iterateElements(Func &&lambda)
    {
        for (int i = 0; i < m_elements.size(); i++) {
            Element &e = m_elements[i];
            if (e.childCount() > 0)
                iterateChildrenOf(e, lambda);
            else
                lambda(e, i);
        }
    }

    template<typename Func>
    void iterateElements(Func &&lambda) const
    {
        for (int i = 0; i < m_elements.size(); i++) {
            const Element &e = m_elements[i];
            if (e.childCount() > 0)
                iterateChildrenOf(e, lambda);
            else
                lambda(e, i);
        }
    }

    static QVector2D closestPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointOnLeft(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointNearLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);

    bool testHint(PathHint hint) const
    {
        return m_hints.testFlag(hint);
    }

    void setHint(PathHint hint, bool on = true)
    {
        m_hints.setFlag(hint, on);
    }

    PathHints pathHints() const
    {
        return m_hints;
    }

    void setPathHints(PathHints newHints)
    {
        m_hints = newHints;
    }

private:
    void addElement(const QVector2D &control, const QVector2D &to, bool isLine = false);
    void addElement(const Element &e);
    Element::FillSide coordinateOrderOfElement(const Element &element) const;

    friend Q_QUICK_EXPORT QDebug operator<<(QDebug, const QQuadPath &);

    QList<Element> m_elements;
    QList<Element> m_childElements;
    QVector2D m_currentPoint;
    bool m_subPathToStart = true;
    bool m_windingFill = false;
    PathHints m_hints;

    friend class QSGCurveProcessor;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuadPath::PathHints);

Q_QUICK_EXPORT QDebug operator<<(QDebug, const QQuadPath::Element &);
Q_QUICK_EXPORT QDebug operator<<(QDebug, const QQuadPath &);

QT_END_NAMESPACE

#endif
