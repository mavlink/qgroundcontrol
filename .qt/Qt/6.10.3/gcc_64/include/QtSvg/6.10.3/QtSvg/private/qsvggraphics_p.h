// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGGRAPHICS_P_H
#define QSVGGRAPHICS_P_H

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

#include "QtGui/qpainterpath.h"
#include "QtGui/qimage.h"
#include "QtGui/qtextlayout.h"
#include "QtGui/qtextoption.h"
#include "QtCore/qloggingcategory.h"
#include "QtCore/qstack.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSvgDraw);

class QTextCharFormat;

class Q_SVG_EXPORT QSvgDummyNode : public QSvgNode
{
public:
    void drawCommand(QPainter *, QSvgExtraStates &) override;
    Type type() const override;
};

class Q_SVG_EXPORT QSvgEllipse : public QSvgNode
{
public:
    QSvgEllipse(QSvgNode *parent, const QRectF &rect);
    bool separateFillStroke() const override;
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF internalFastBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF rect() const { return m_bounds; }
private:
    QRectF m_bounds;
};

class Q_SVG_EXPORT QSvgCircle : public QSvgEllipse
{
public:
    QSvgCircle(QSvgNode *parent, const QRectF &rect) : QSvgEllipse(parent, rect) { }
    Type type() const override;
};

class Q_SVG_EXPORT QSvgImage : public QSvgNode
{
public:
    QSvgImage(QSvgNode *parent,
              const QImage &image,
              const QString &filename,
              const QRectF &bounds);
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;

    QRectF rect() const { return m_bounds; }
    const QImage &image() const { return m_image; }
    QString filename() const { return m_filename; }
private:
    QString m_filename;
    QImage m_image;
    QRectF m_bounds;
};

class Q_SVG_EXPORT QSvgLine : public QSvgNode
{
public:
    QSvgLine(QSvgNode *parent, const QLineF &line);
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF internalFastBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const override;
    bool requiresGroupRendering() const override;
    QLineF line() const { return m_line; }
private:
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states, BoundsMode mode) const;
    QLineF m_line;
};

class Q_SVG_EXPORT QSvgPath : public QSvgNode
{
public:
    QSvgPath(QSvgNode *parent, const QPainterPath &qpath);
    bool separateFillStroke() const override;
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF internalFastBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const override;
    bool requiresGroupRendering() const override;
    const QPainterPath &path() const { return m_path; }
private:
    QPainterPath m_path;
};

class Q_SVG_EXPORT QSvgPolygon : public QSvgNode
{
public:
    QSvgPolygon(QSvgNode *parent, const QPolygonF &poly);
    bool separateFillStroke() const override;
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF internalFastBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const override;
    bool requiresGroupRendering() const override;
    const QPolygonF &polygon() const { return m_poly; }
private:
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states, BoundsMode mode) const;
    QPolygonF m_poly;
};

class Q_SVG_EXPORT QSvgPolyline : public QSvgNode
{
public:
    QSvgPolyline(QSvgNode *parent, const QPolygonF &poly);
    bool separateFillStroke() const override;
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF internalFastBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const override;
    bool requiresGroupRendering() const override;
    const QPolygonF &polygon() const { return m_poly; }
private:
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states, BoundsMode mode) const;
    QPolygonF m_poly;
};

class Q_SVG_EXPORT QSvgRect : public QSvgNode
{
public:
    QSvgRect(QSvgNode *paren, const QRectF &rect, qreal rx=0, qreal ry=0);
    Type type() const override;
    bool separateFillStroke() const override;
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    QRectF internalFastBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF rect() const { return m_rect; }
    QPointF radius() const { return { m_rx, m_ry }; }
private:
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states, BoundsMode mode) const;
    QRectF m_rect;
    qreal m_rx, m_ry;
};

class  QSvgTspan;

class Q_SVG_EXPORT QSvgText : public QSvgNode
{
public:
    enum WhitespaceMode
    {
        Default,
        Preserve
    };

    QSvgText(QSvgNode *parent, const QPointF &coord);
    ~QSvgText();
    void setTextArea(const QSizeF &size);

    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    bool shouldDrawNode(QPainter *p, QSvgExtraStates &states) const override;
    Type type() const override;
    bool separateFillStroke() const override;

    void addTspan(QSvgTspan *tspan) {m_tspans.append(tspan);}
    const QList<QSvgTspan *> tspans() const { return m_tspans; }
    void addText(QStringView text);
    void addLineBreak() {m_tspans.append(LINEBREAK);}
    void setWhitespaceMode(WhitespaceMode mode) {m_mode = mode;}

    QRectF internalFastBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;

    QPointF position() const { return m_coord; }
    QSizeF size() const { return m_size; }
    WhitespaceMode whitespaceMode() const { return m_mode; }

private:
    void draw_helper(QPainter *p, QSvgExtraStates &states, QRectF *boundingRect = nullptr) const;

    static QSvgTspan * const LINEBREAK;

    QPointF m_coord;

    // 'm_tspans' is also used to store characters outside tspans and line breaks.
    // If a 'm_tspan' item is null, it indicates a line break.
    QList<QSvgTspan *> m_tspans;

    Type m_type;
    QSizeF m_size;
    WhitespaceMode m_mode;
};

class Q_SVG_EXPORT QSvgTspan : public QSvgNode
{
public:
    // tspans are also used to store normal text, so the 'isProperTspan' is used to separate text from tspan.
    QSvgTspan(QSvgNode *parent, bool isProperTspan = true)
        : QSvgNode(parent), m_mode(QSvgText::Default), m_isTspan(isProperTspan)
    {
    }
    ~QSvgTspan() { };
    Type type() const override { return Tspan; }
    void drawCommand(QPainter *, QSvgExtraStates &) override { Q_ASSERT(!"Tspans should be drawn through QSvgText::draw()."); }
    void addText(QStringView text) {m_text += text;}
    const QString &text() const {return m_text;}
    bool isTspan() const {return m_isTspan;}
    void setWhitespaceMode(QSvgText::WhitespaceMode mode) {m_mode = mode;}
    QSvgText::WhitespaceMode whitespaceMode() const {return m_mode;}
private:
    QString m_text;
    QSvgText::WhitespaceMode m_mode;
    bool m_isTspan;
};

class QSvgUse : public QSvgNode
{
public:
    QSvgUse(const QPointF &start, QSvgNode *parent, QSvgNode *link);
    QSvgUse(const QPointF &start, QSvgNode *parent, const QString &linkId)
        : QSvgUse(start, parent, nullptr)
    { m_linkId = linkId; }
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF internalBounds(QPainter *p, QSvgExtraStates &states) const override;
    QRectF decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const override;
    bool isResolved() const { return m_link != nullptr; }
    const QString &linkId() const { return m_linkId; }
    void setLink(QSvgNode *link) { m_link = link; }
    QSvgNode *link() const { return m_link; }
    QPointF start() const { return m_start; }
    bool isRecursing() const { return m_recursing; }

private:
    QSvgNode *m_link;
    QPointF   m_start;
    QString   m_linkId;
    mutable bool m_recursing;
};

class QSvgVideo : public QSvgNode
{
public:
    void drawCommand(QPainter *, QSvgExtraStates &) override {};
    Type type() const override;
};

QT_END_NAMESPACE

#endif // QSVGGRAPHICS_P_H
