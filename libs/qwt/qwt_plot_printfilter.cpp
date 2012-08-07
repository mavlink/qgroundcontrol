/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qmap.h>
#include "qwt_plot.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_marker.h"
#include "qwt_symbol.h"
#include "qwt_legend.h"
#include "qwt_legend_item.h"
#include "qwt_scale_widget.h"
#include "qwt_text_label.h"
#include "qwt_plot_printfilter.h"

#if QT_VERSION < 0x040000
typedef QColorGroup Palette;
#else
typedef QPalette Palette;
#endif

class QwtPlotPrintFilter::PrivateData
{
public:
    PrivateData():
        options(QwtPlotPrintFilter::PrintAll),
        cache(NULL) {
    }

    ~PrivateData() {
        delete cache;
    }

    class Cache
    {
    public:
        QColor titleColor;
        QFont titleFont;

        QwtText scaleTitle[QwtPlot::axisCnt];
        QColor scaleColor[QwtPlot::axisCnt];
        QFont scaleFont[QwtPlot::axisCnt];
        QColor scaleTitleColor[QwtPlot::axisCnt];
        QFont scaleTitleFont[QwtPlot::axisCnt];

        QMap<QWidget *, QFont> legendFonts;

        QColor widgetBackground;
        QColor canvasBackground;
        QColor gridColors[2];

        QMap<const QwtPlotItem *, QColor> curveColors;
        QMap<const QwtPlotItem *, QColor> curveSymbolBrushColors;
        QMap<const QwtPlotItem *, QColor> curveSymbolPenColors;

        QMap<const QwtPlotItem *, QFont> markerFonts;
        QMap<const QwtPlotItem *, QColor> markerLabelColors;
        QMap<const QwtPlotItem *, QColor> markerLineColors;
        QMap<const QwtPlotItem *, QColor> markerSymbolBrushColors;
        QMap<const QwtPlotItem *, QColor> markerSymbolPenColors;
    };

    int options;
    mutable Cache *cache;
};


/*!
  Sets filter options to PrintAll
*/

QwtPlotPrintFilter::QwtPlotPrintFilter()
{
    d_data = new PrivateData;
}

//! Destructor
QwtPlotPrintFilter::~QwtPlotPrintFilter()
{
    delete d_data;
}

/*!
  \brief Set plot print options
  \param options Or'd QwtPlotPrintFilter::Options values

  \sa options()
*/
void QwtPlotPrintFilter::setOptions(int options)
{
    d_data->options = options;
}

/*!
  \brief Get plot print options
  \sa setOptions()
*/
int QwtPlotPrintFilter::options() const
{
    return d_data->options;
}

/*!
  \brief Modifies a color for printing
  \param c Color to be modified
  \param item Type of item where the color belongs
  \return Modified color.

  In case of !(QwtPlotPrintFilter::options() & PrintBackground)
  MajorGrid is modified to Qt::darkGray, MinorGrid to Qt::gray.
  All other colors are returned unmodified.
*/

QColor QwtPlotPrintFilter::color(const QColor &c, Item item) const
{
    if ( !(options() & PrintBackground)) {
        switch(item) {
        case MajorGrid:
            return Qt::darkGray;
        case MinorGrid:
            return Qt::gray;
        default:
            ;
        }
    }
    return c;
}

/*!
  \brief Modifies a font for printing
  \param f Font to be modified
  \param item Type of item where the font belongs

  All fonts are returned unmodified
*/

QFont QwtPlotPrintFilter::font(const QFont &f, Item) const
{
    return f;
}

/*!
  Change color and fonts of a plot
  \sa apply
*/
void QwtPlotPrintFilter::apply(QwtPlot *plot) const
{
    const bool doAutoReplot = plot->autoReplot();
    plot->setAutoReplot(false);

    delete d_data->cache;
    d_data->cache = new PrivateData::Cache;

    PrivateData::Cache &cache = *d_data->cache;

    if ( plot->titleLabel() ) {
        QPalette palette = plot->titleLabel()->palette();
        cache.titleColor = palette.color(
                               QPalette::Active, Palette::Text);
        palette.setColor(QPalette::Active, Palette::Text,
                         color(cache.titleColor, Title));
        plot->titleLabel()->setPalette(palette);

        cache.titleFont = plot->titleLabel()->font();
        plot->titleLabel()->setFont(font(cache.titleFont, Title));
    }
    if ( plot->legend() ) {
#if QT_VERSION < 0x040000
        QValueList<QWidget *> list = plot->legend()->legendItems();
        for ( QValueListIterator<QWidget *> it = list.begin();
                it != list.end(); ++it )
#else
        QList<QWidget *> list = plot->legend()->legendItems();
        for ( QList<QWidget*>::iterator it = list.begin();
                it != list.end(); ++it )
#endif
        {
            QWidget *w = *it;

            cache.legendFonts.insert(w, w->font());
            w->setFont(font(w->font(), Legend));

            if ( w->inherits("QwtLegendItem") ) {
                QwtLegendItem *label = (QwtLegendItem *)w;

                QwtSymbol symbol = label->symbol();
                QPen pen = symbol.pen();
                QBrush brush = symbol.brush();

                pen.setColor(color(pen.color(), CurveSymbol));
                brush.setColor(color(brush.color(), CurveSymbol));

                symbol.setPen(pen);
                symbol.setBrush(brush);
                label->setSymbol(symbol);

                pen = label->curvePen();
                pen.setColor(color(pen.color(), Curve));
                label->setCurvePen(pen);
            }
        }
    }
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        QwtScaleWidget *scaleWidget = plot->axisWidget(axis);
        if ( scaleWidget ) {
            cache.scaleColor[axis] = scaleWidget->palette().color(
                                         QPalette::Active, Palette::Foreground);
            QPalette palette = scaleWidget->palette();
            palette.setColor(QPalette::Active, Palette::Foreground,
                             color(cache.scaleColor[axis], AxisScale));
            scaleWidget->setPalette(palette);

            cache.scaleFont[axis] = scaleWidget->font();
            scaleWidget->setFont(font(cache.scaleFont[axis], AxisScale));

            cache.scaleTitle[axis] = scaleWidget->title();

            QwtText scaleTitle = scaleWidget->title();
            if ( scaleTitle.testPaintAttribute(QwtText::PaintUsingTextColor) ) {
                cache.scaleTitleColor[axis] = scaleTitle.color();
                scaleTitle.setColor(
                    color(cache.scaleTitleColor[axis], AxisTitle));
            }

            if ( scaleTitle.testPaintAttribute(QwtText::PaintUsingTextFont) ) {
                cache.scaleTitleFont[axis] = scaleTitle.font();
                scaleTitle.setFont(
                    font(cache.scaleTitleFont[axis], AxisTitle));
            }

            scaleWidget->setTitle(scaleTitle);

            int startDist, endDist;
            scaleWidget->getBorderDistHint(startDist, endDist);
            scaleWidget->setBorderDist(startDist, endDist);
        }
    }


    QPalette p = plot->palette();
    cache.widgetBackground = plot->palette().color(
                                 QPalette::Active, Palette::Background);
    p.setColor(QPalette::Active, Palette::Background,
               color(cache.widgetBackground, WidgetBackground));
    plot->setPalette(p);

    cache.canvasBackground = plot->canvasBackground();
    plot->setCanvasBackground(color(cache.canvasBackground, CanvasBackground));

    const QwtPlotItemList& itmList = plot->itemList();
    for ( QwtPlotItemIterator it = itmList.begin();
            it != itmList.end(); ++it ) {
        apply(*it);
    }

    plot->setAutoReplot(doAutoReplot);
}

void QwtPlotPrintFilter::apply(QwtPlotItem *item) const
{
    PrivateData::Cache &cache = *d_data->cache;

    switch(item->rtti()) {
    case QwtPlotItem::Rtti_PlotGrid: {
        QwtPlotGrid *grid = (QwtPlotGrid *)item;

        QPen pen = grid->majPen();
        cache.gridColors[0] = pen.color();
        pen.setColor(color(pen.color(), MajorGrid));
        grid->setMajPen(pen);

        pen = grid->minPen();
        cache.gridColors[1] = pen.color();
        pen.setColor(color(pen.color(), MinorGrid));
        grid->setMinPen(pen);

        break;
    }
    case QwtPlotItem::Rtti_PlotCurve: {
        QwtPlotCurve *c = (QwtPlotCurve *)item;

        QwtSymbol symbol = c->symbol();

        QPen pen = symbol.pen();
        cache.curveSymbolPenColors.insert(c, pen.color());
        pen.setColor(color(pen.color(), CurveSymbol));
        symbol.setPen(pen);

        QBrush brush = symbol.brush();
        cache.curveSymbolBrushColors.insert(c, brush.color());
        brush.setColor(color(brush.color(), CurveSymbol));
        symbol.setBrush(brush);

        c->setSymbol(symbol);

        pen = c->pen();
        cache.curveColors.insert(c, pen.color());
        pen.setColor(color(pen.color(), Curve));
        c->setPen(pen);

        break;
    }
    case QwtPlotItem::Rtti_PlotMarker: {
        QwtPlotMarker *m = (QwtPlotMarker *)item;

        QwtText label = m->label();
        cache.markerFonts.insert(m, label.font());
        label.setFont(font(label.font(), Marker));
        cache.markerLabelColors.insert(m, label.color());
        label.setColor(color(label.color(), Marker));
        m->setLabel(label);

        QPen pen = m->linePen();
        cache.markerLineColors.insert(m, pen.color());
        pen.setColor(color(pen.color(), Marker));
        m->setLinePen(pen);

        QwtSymbol symbol = m->symbol();

        pen = symbol.pen();
        cache.markerSymbolPenColors.insert(m, pen.color());
        pen.setColor(color(pen.color(), MarkerSymbol));
        symbol.setPen(pen);

        QBrush brush = symbol.brush();
        cache.markerSymbolBrushColors.insert(m, brush.color());
        brush.setColor(color(brush.color(), MarkerSymbol));
        symbol.setBrush(brush);

        m->setSymbol(symbol);

        break;
    }
    default:
        break;
    }
}

/*!
   Reset color and fonts of a plot
   \sa apply
*/
void QwtPlotPrintFilter::reset(QwtPlot *plot) const
{
    if ( d_data->cache == 0 )
        return;

    const bool doAutoReplot = plot->autoReplot();
    plot->setAutoReplot(false);

    const PrivateData::Cache &cache = *d_data->cache;

    if ( plot->titleLabel() ) {
        QwtTextLabel* title = plot->titleLabel();
        if ( title->text().testPaintAttribute(QwtText::PaintUsingTextFont) ) {
            QwtText text = title->text();
            text.setColor(cache.titleColor);
            title->setText(text);
        } else {
            QPalette palette = title->palette();
            palette.setColor(
                QPalette::Active, Palette::Text, cache.titleColor);
            title->setPalette(palette);
        }

        if ( title->text().testPaintAttribute(QwtText::PaintUsingTextFont) ) {
            QwtText text = title->text();
            text.setFont(cache.titleFont);
            title->setText(text);
        } else {
            title->setFont(cache.titleFont);
        }
    }

    if ( plot->legend() ) {
#if QT_VERSION < 0x040000
        QValueList<QWidget *> list = plot->legend()->legendItems();
        for ( QValueListIterator<QWidget *> it = list.begin();
                it != list.end(); ++it )
#else
        QList<QWidget *> list = plot->legend()->legendItems();
        for ( QList<QWidget*>::iterator it = list.begin();
                it != list.end(); ++it )
#endif
        {
            QWidget *w = *it;

            if ( cache.legendFonts.contains(w) )
                w->setFont(cache.legendFonts[w]);

            if ( w->inherits("QwtLegendItem") ) {
                QwtLegendItem *label = (QwtLegendItem *)w;
                const QwtPlotItem *plotItem =
                    (const QwtPlotItem*)plot->legend()->find(label);

                QwtSymbol symbol = label->symbol();
                if ( cache.curveSymbolPenColors.contains(plotItem) ) {
                    QPen pen = symbol.pen();
                    pen.setColor(cache.curveSymbolPenColors[plotItem]);
                    symbol.setPen(pen);
                }

                if ( cache.curveSymbolBrushColors.contains(plotItem) ) {
                    QBrush brush = symbol.brush();
                    brush.setColor(cache.curveSymbolBrushColors[plotItem]);
                    symbol.setBrush(brush);
                }
                label->setSymbol(symbol);

                if ( cache.curveColors.contains(plotItem) ) {
                    QPen pen = label->curvePen();
                    pen.setColor(cache.curveColors[plotItem]);
                    label->setCurvePen(pen);
                }
            }
        }
    }
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        QwtScaleWidget *scaleWidget = plot->axisWidget(axis);
        if ( scaleWidget ) {
            QPalette palette = scaleWidget->palette();
            palette.setColor(QPalette::Active, Palette::Foreground,
                             cache.scaleColor[axis]);
            scaleWidget->setPalette(palette);

            scaleWidget->setFont(cache.scaleFont[axis]);
            scaleWidget->setTitle(cache.scaleTitle[axis]);

            int startDist, endDist;
            scaleWidget->getBorderDistHint(startDist, endDist);
            scaleWidget->setBorderDist(startDist, endDist);
        }
    }

    QPalette p = plot->palette();
    p.setColor(QPalette::Active, Palette::Background, cache.widgetBackground);
    plot->setPalette(p);

    plot->setCanvasBackground(cache.canvasBackground);

    const QwtPlotItemList& itmList = plot->itemList();
    for ( QwtPlotItemIterator it = itmList.begin();
            it != itmList.end(); ++it ) {
        reset(*it);
    }

    delete d_data->cache;
    d_data->cache = 0;

    plot->setAutoReplot(doAutoReplot);
}

void QwtPlotPrintFilter::reset(QwtPlotItem *item) const
{
    if ( d_data->cache == 0 )
        return;

    const PrivateData::Cache &cache = *d_data->cache;

    switch(item->rtti()) {
    case QwtPlotItem::Rtti_PlotGrid: {
        QwtPlotGrid *grid = (QwtPlotGrid *)item;

        QPen pen = grid->majPen();
        pen.setColor(cache.gridColors[0]);
        grid->setMajPen(pen);

        pen = grid->minPen();
        pen.setColor(cache.gridColors[1]);
        grid->setMinPen(pen);

        break;
    }
    case QwtPlotItem::Rtti_PlotCurve: {
        QwtPlotCurve *c = (QwtPlotCurve *)item;

        QwtSymbol symbol = c->symbol();

        if ( cache.curveSymbolPenColors.contains(c) ) {
            symbol.setPen(cache.curveSymbolPenColors[c]);
        }

        if ( cache.curveSymbolBrushColors.contains(c) ) {
            QBrush brush = symbol.brush();
            brush.setColor(cache.curveSymbolBrushColors[c]);
            symbol.setBrush(brush);
        }
        c->setSymbol(symbol);

        if ( cache.curveColors.contains(c) ) {
            QPen pen = c->pen();
            pen.setColor(cache.curveColors[c]);
            c->setPen(pen);
        }

        break;
    }
    case QwtPlotItem::Rtti_PlotMarker: {
        QwtPlotMarker *m = (QwtPlotMarker *)item;

        if ( cache.markerFonts.contains(m) ) {
            QwtText label = m->label();
            label.setFont(cache.markerFonts[m]);
            m->setLabel(label);
        }

        if ( cache.markerLabelColors.contains(m) ) {
            QwtText label = m->label();
            label.setColor(cache.markerLabelColors[m]);
            m->setLabel(label);
        }

        if ( cache.markerLineColors.contains(m) ) {
            QPen pen = m->linePen();
            pen.setColor(cache.markerLineColors[m]);
            m->setLinePen(pen);
        }

        QwtSymbol symbol = m->symbol();

        if ( cache.markerSymbolPenColors.contains(m) ) {
            QPen pen = symbol.pen();
            pen.setColor(cache.markerSymbolPenColors[m]);
            symbol.setPen(pen);
        }

        if ( cache.markerSymbolBrushColors.contains(m) ) {
            QBrush brush = symbol.brush();
            brush.setColor(cache.markerSymbolBrushColors[m]);
            symbol.setBrush(brush);
        }

        m->setSymbol(symbol);

        break;
    }
    default:
        break;
    }
}
