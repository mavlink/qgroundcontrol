// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGFILTER_P_H
#define QSVGFILTER_P_H

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

#include "qsvgnode_p.h"
#include "qtsvgglobal_p.h"
#include "qsvgstructure_p.h"
#include "qgenericmatrix.h"

#include "QtCore/qlist.h"
#include "QtCore/qhash.h"
#include "QtGui/qvector4d.h"

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgFeFilterPrimitive : public QSvgStructureNode
{
public:
    QSvgFeFilterPrimitive(QSvgNode *parent, const QString &input,
                          const QString &result, const QSvgRectF &rect);
    void drawCommand(QPainter *, QSvgExtraStates &) override {};
    bool shouldDrawNode(QPainter *, QSvgExtraStates &) const override;
    QRectF internalFastBounds(QPainter *, QSvgExtraStates &) const override { return QRectF(); }
    QRectF internalBounds(QPainter *, QSvgExtraStates &) const override { return QRectF(); }
    QRectF localSubRegion(const QRectF &itemBounds, const QRectF &filterBounds,
                          QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const;
    QRectF globalSubRegion(QPainter *p,
                           const QRectF &itemBounds, const QRectF &filterBounds,
                           QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const;
    void clipToTransformedBounds(QImage *buffer, QPainter *p, const QRectF &localRect) const;
    virtual QImage apply(const QMap<QString, QImage> &sources,
                         QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                         QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const = 0;
    virtual bool requiresSourceAlpha() const;
    QString input() const {
        return m_input;
    }
    QString result() const {
        return m_result;
    }

    static const QSvgFeFilterPrimitive *castToFilterPrimitive(const QSvgNode *node);

protected:
    QString m_input;
    QString m_result;
    QSvgRectF m_rect;


};

class Q_SVG_EXPORT QSvgFeColorMatrix : public QSvgFeFilterPrimitive
{
public:
    enum class ColorShiftType : quint8 {
        Matrix,
        Saturate,
        HueRotate,
        LuminanceToAlpha
    };

    typedef QGenericMatrix<5, 5, qreal> Matrix;
    typedef QGenericMatrix<5, 1, qreal> Vector;

    QSvgFeColorMatrix(QSvgNode *parent, const QString &input, const QString &result,
                      const QSvgRectF &rect, ColorShiftType type, const Matrix &matrix);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
private:
    Matrix m_matrix;
};

class Q_SVG_EXPORT QSvgFeGaussianBlur : public QSvgFeFilterPrimitive
{
public:
    enum class EdgeMode : quint8 {
        Duplicate,
        Wrap,
        None
    };

    QSvgFeGaussianBlur(QSvgNode *parent, const QString &input, const QString &result,
                       const QSvgRectF &rect, qreal stdDeviationX, qreal stdDeviationY,
                       EdgeMode edgemode);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
private:
    qreal m_stdDeviationX;
    qreal m_stdDeviationY;
    EdgeMode m_edgemode; // TODO: Unused. Start using it when there's a reference implementation.
};

class Q_SVG_EXPORT QSvgFeOffset : public QSvgFeFilterPrimitive
{
public:
    QSvgFeOffset(QSvgNode *parent, const QString &input, const QString &result,
                 const QSvgRectF &rect, qreal dx, qreal dy);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
private:
    qreal m_dx;
    qreal m_dy;
};

class Q_SVG_EXPORT QSvgFeMerge : public QSvgFeFilterPrimitive
{
public:
    QSvgFeMerge(QSvgNode *parent, const QString &input,
                const QString &result, const QSvgRectF &rect);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
    bool requiresSourceAlpha() const override;
};

class Q_SVG_EXPORT QSvgFeMergeNode : public QSvgFeFilterPrimitive
{
public:
    QSvgFeMergeNode(QSvgNode *parent, const QString &input,
                    const QString &result, const QSvgRectF &rect);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
};

class Q_SVG_EXPORT QSvgFeComposite : public QSvgFeFilterPrimitive
{
public:
    enum class Operator : quint8 {
        Over,
        In,
        Out,
        Atop,
        Xor,
        Lighter,
        Arithmetic
    };
    QSvgFeComposite(QSvgNode *parent, const QString &input, const QString &result,
                    const QSvgRectF &rect, const QString &input2, Operator op, const QVector4D &k);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
    bool requiresSourceAlpha() const override;
private:
    QString m_input2;
    Operator m_operator;
    QVector4D m_k;
};

class Q_SVG_EXPORT QSvgFeFlood : public QSvgFeFilterPrimitive
{
public:
    QSvgFeFlood(QSvgNode *parent, const QString &input, const QString &result,
                const QSvgRectF &rect, const QColor &color);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
private:
    QColor m_color;
};

class Q_SVG_EXPORT QSvgFeBlend : public QSvgFeFilterPrimitive
{
public:
    enum class Mode : quint8 {
        Normal,
        Multiply,
        Screen,
        Darken,
        Lighten
    };
    QSvgFeBlend(QSvgNode *parent, const QString &input, const QString &result,
                const QSvgRectF &rect, const QString &input2, Mode mode);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
    bool requiresSourceAlpha() const override;
private:
    QString m_input2;
    Mode m_mode;

};

class Q_SVG_EXPORT QSvgFeUnsupported : public QSvgFeFilterPrimitive
{
public:
    QSvgFeUnsupported(QSvgNode *parent, const QString &input,
                      const QString &result, const QSvgRectF &rect);
    Type type() const override;
    QImage apply(const QMap<QString, QImage> &sources,
                 QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const override;
};

QT_END_NAMESPACE

#endif // QSVGFILTER_P_H
