/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpainter.h>
#if QT_VERSION < 0x040000
#include <qguardedptr.h>
#include <qfocusdata.h>
#else
#include <qpointer.h>
#include <qpaintengine.h>
#endif
#include <qapplication.h>
#include <qevent.h>
#include "qwt_plot.h"
#include "qwt_plot_dict.h"
#include "qwt_plot_layout.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_engine.h"
#include "qwt_text_label.h"
#include "qwt_legend.h"
#include "qwt_dyngrid_layout.h"
#include "qwt_plot_canvas.h"
#include "qwt_paint_buffer.h"

class QwtPlot::PrivateData
{
public:
#if QT_VERSION < 0x040000
    QGuardedPtr<QwtTextLabel> lblTitle;
    QGuardedPtr<QwtPlotCanvas> canvas;
    QGuardedPtr<QwtLegend> legend;
#else
    QPointer<QwtTextLabel> lblTitle;
    QPointer<QwtPlotCanvas> canvas;
    QPointer<QwtLegend> legend;
#endif
    QwtPlotLayout *layout;

    bool autoReplot;
};

/*!
  \brief Constructor
  \param parent Parent widget
 */
QwtPlot::QwtPlot(QWidget *parent):
    QFrame(parent)
{
    initPlot(QwtText());
}

/*!
  \brief Constructor
  \param title Title text
  \param parent Parent widget
 */
QwtPlot::QwtPlot(const QwtText &title, QWidget *parent):
    QFrame(parent)
{
    initPlot(title);
}

#if QT_VERSION < 0x040000
/*!
  \brief Constructor
  \param parent Parent widget
  \param name Object name
 */
QwtPlot::QwtPlot(QWidget *parent, const char *name):
    QFrame(parent, name)
{
    initPlot(QwtText());
}
#endif


//! Destructor
QwtPlot::~QwtPlot()
{
    detachItems(QwtPlotItem::Rtti_PlotItem, autoDelete());

    delete d_data->layout;
    deleteAxesData();
    delete d_data;
}

/*!
  \brief Initializes a QwtPlot instance
  \param title Title text
 */
void QwtPlot::initPlot(const QwtText &title)
{
    d_data = new PrivateData;

#if QT_VERSION < 0x040000
    setWFlags(Qt::WNoAutoErase);
#endif

    d_data->layout = new QwtPlotLayout;

    d_data->autoReplot = false;

    d_data->lblTitle = new QwtTextLabel(title, this);
    d_data->lblTitle->setFont(QFont(fontInfo().family(), 14, QFont::Bold));

    QwtText text(title);
    int flags = Qt::AlignCenter;
#if QT_VERSION < 0x040000
    flags |= Qt::WordBreak | Qt::ExpandTabs;
#else
    flags |= Qt::TextWordWrap;
#endif
    text.setRenderFlags(flags);
    d_data->lblTitle->setText(text);

    d_data->legend = NULL;

    initAxesData();

    d_data->canvas = new QwtPlotCanvas(this);
    d_data->canvas->setFrameStyle(QFrame::Panel|QFrame::Sunken);
    d_data->canvas->setLineWidth(2);
    d_data->canvas->setMidLineWidth(0);

    updateTabOrder();

    setSizePolicy(QSizePolicy::MinimumExpanding,
                  QSizePolicy::MinimumExpanding);
}

/*!
  \brief Adds handling of layout requests
*/
bool QwtPlot::event(QEvent *e)
{
    bool ok = QFrame::event(e);
    switch(e->type()) {
#if QT_VERSION < 0x040000
    case QEvent::LayoutHint:
#else
    case QEvent::LayoutRequest:
#endif
        updateLayout();
        break;
#if QT_VERSION >= 0x040000
    case QEvent::PolishRequest:
        polish();
        break;
#endif
    default:
        ;
    }
    return ok;
}

//! Replots the plot if QwtPlot::autoReplot() is \c true.
void QwtPlot::autoRefresh()
{
    if (d_data->autoReplot)
        replot();
}

/*!
  \brief Set or reset the autoReplot option

  If the autoReplot option is set, the plot will be
  updated implicitly by manipulating member functions.
  Since this may be time-consuming, it is recommended
  to leave this option switched off and call replot()
  explicitly if necessary.

  The autoReplot option is set to false by default, which
  means that the user has to call replot() in order to make
  changes visible.
  \param tf \c true or \c false. Defaults to \c true.
  \sa replot()
*/
void QwtPlot::setAutoReplot(bool tf)
{
    d_data->autoReplot = tf;
}

//! \return true if the autoReplot option is set.
bool QwtPlot::autoReplot() const
{
    return d_data->autoReplot;
}

/*!
  Change the plot's title
  \param title New title
*/
void QwtPlot::setTitle(const QString &title)
{
    if ( title != d_data->lblTitle->text().text() ) {
        d_data->lblTitle->setText(title);
        updateLayout();
    }
}

/*!
  Change the plot's title
  \param title New title
*/
void QwtPlot::setTitle(const QwtText &title)
{
    if ( title != d_data->lblTitle->text() ) {
        d_data->lblTitle->setText(title);
        updateLayout();
    }
}

//! \return the plot's title
QwtText QwtPlot::title() const
{
    return d_data->lblTitle->text();
}

//! \return the plot's title
QwtPlotLayout *QwtPlot::plotLayout()
{
    return d_data->layout;
}

//! \return the plot's titel label.
const QwtPlotLayout *QwtPlot::plotLayout() const
{
    return d_data->layout;
}

//! \return the plot's titel label.
QwtTextLabel *QwtPlot::titleLabel()
{
    return d_data->lblTitle;
}

/*!
  \return the plot's titel label.
*/
const QwtTextLabel *QwtPlot::titleLabel() const
{
    return d_data->lblTitle;
}

/*!
  \return the plot's legend
  \sa insertLegend()
*/
QwtLegend *QwtPlot::legend()
{
    return d_data->legend;
}

/*!
  \return the plot's legend
  \sa insertLegend()
*/
const QwtLegend *QwtPlot::legend() const
{
    return d_data->legend;
}


/*!
  \return the plot's canvas
*/
QwtPlotCanvas *QwtPlot::canvas()
{
    return d_data->canvas;
}

/*!
  \return the plot's canvas
*/
const QwtPlotCanvas *QwtPlot::canvas() const
{
    return d_data->canvas;
}

//! Polish
void QwtPlot::polish()
{
    replot();

#if QT_VERSION < 0x040000
    QFrame::polish();
#endif
}

/*!
  Return sizeHint
  \sa minimumSizeHint()
*/

QSize QwtPlot::sizeHint() const
{
    int dw = 0;
    int dh = 0;
    for ( int axisId = 0; axisId < axisCnt; axisId++ ) {
        if ( axisEnabled(axisId) ) {
            const int niceDist = 40;
            const QwtScaleWidget *scaleWidget = axisWidget(axisId);
            const QwtScaleDiv &scaleDiv = scaleWidget->scaleDraw()->scaleDiv();
            const int majCnt = scaleDiv.ticks(QwtScaleDiv::MajorTick).count();

            if ( axisId == yLeft || axisId == yRight ) {
                int hDiff = (majCnt - 1) * niceDist
                            - scaleWidget->minimumSizeHint().height();
                if ( hDiff > dh )
                    dh = hDiff;
            } else {
                int wDiff = (majCnt - 1) * niceDist
                            - scaleWidget->minimumSizeHint().width();
                if ( wDiff > dw )
                    dw = wDiff;
            }
        }
    }
    return minimumSizeHint() + QSize(dw, dh);
}

/*!
  \brief Return a minimum size hint
*/
QSize QwtPlot::minimumSizeHint() const
{
    QSize hint = d_data->layout->minimumSizeHint(this);
    hint += QSize(2 * frameWidth(), 2 * frameWidth());

    return hint;
}

//! Resize and update internal layout
void QwtPlot::resizeEvent(QResizeEvent *e)
{
    QFrame::resizeEvent(e);
    updateLayout();
}

/*!
  \brief Redraw the plot

  If the autoReplot option is not set (which is the default)
  or if any curves are attached to raw data, the plot has to
  be refreshed explicitly in order to make changes visible.

  \sa setAutoReplot()
  \warning Calls canvas()->repaint, take care of infinite recursions
*/
void QwtPlot::replot()
{
    bool doAutoReplot = autoReplot();
    setAutoReplot(false);

    updateAxes();

    /*
      Maybe the layout needs to be updated, because of changed
      axes labels. We need to process them here before painting
      to avoid that scales and canvas get out of sync.
     */
#if QT_VERSION >= 0x040000
    QApplication::sendPostedEvents(this, QEvent::LayoutRequest);
#else
    QApplication::sendPostedEvents(this, QEvent::LayoutHint);
#endif

    QwtPlotCanvas &canvas = *d_data->canvas;

    canvas.invalidatePaintCache();

    /*
      In case of cached or packed painting the canvas
      is repainted completely and doesn't need to be erased.
     */
    const bool erase =
        !canvas.testPaintAttribute(QwtPlotCanvas::PaintPacked)
        && !canvas.testPaintAttribute(QwtPlotCanvas::PaintCached);

#if QT_VERSION >= 0x040000
    const bool noBackgroundMode = canvas.testAttribute(Qt::WA_NoBackground);
    if ( !erase && !noBackgroundMode )
        canvas.setAttribute(Qt::WA_NoBackground, true);

    canvas.repaint(canvas.contentsRect());

    if ( !erase && !noBackgroundMode )
        canvas.setAttribute(Qt::WA_NoBackground, false);
#else
    canvas.repaint(canvas.contentsRect(), erase);
#endif

    setAutoReplot(doAutoReplot);
}

/*!
  \brief Adjust plot content to its current size.
  \sa resizeEvent()
*/
void QwtPlot::updateLayout()
{
    d_data->layout->activate(this, contentsRect());

    //
    // resize and show the visible widgets
    //
    if (!d_data->lblTitle->text().isEmpty()) {
        d_data->lblTitle->setGeometry(d_data->layout->titleRect());
        if (!d_data->lblTitle->isVisible())
            d_data->lblTitle->show();
    } else
        d_data->lblTitle->hide();

    for (int axisId = 0; axisId < axisCnt; axisId++ ) {
        if (axisEnabled(axisId) ) {
            axisWidget(axisId)->setGeometry(d_data->layout->scaleRect(axisId));

            if ( axisId == xBottom || axisId == xTop ) {
                QRegion r(d_data->layout->scaleRect(axisId));
                if ( axisEnabled(yLeft) )
                    r = r.subtract(QRegion(d_data->layout->scaleRect(yLeft)));
                if ( axisEnabled(yRight) )
                    r = r.subtract(QRegion(d_data->layout->scaleRect(yRight)));
                r.translate(-d_data->layout->scaleRect(axisId).x(),
                            -d_data->layout->scaleRect(axisId).y());

                axisWidget(axisId)->setMask(r);
            }
            if (!axisWidget(axisId)->isVisible())
                axisWidget(axisId)->show();
        } else
            axisWidget(axisId)->hide();
    }

    if ( d_data->legend &&
            d_data->layout->legendPosition() != ExternalLegend ) {
        if (d_data->legend->itemCount() > 0) {
            d_data->legend->setGeometry(d_data->layout->legendRect());
            d_data->legend->show();
        } else
            d_data->legend->hide();
    }

    d_data->canvas->setGeometry(d_data->layout->canvasRect());
}

/*!
   Update the focus tab order

   The order is changed so that the canvas will be in front of the
   first legend item, or behind the last legend item - depending
   on the position of the legend.
*/

void QwtPlot::updateTabOrder()
{
#if QT_VERSION >= 0x040000
    using namespace Qt; // QWidget::NoFocus/Qt::NoFocus
#else
    if ( d_data->canvas->focusPolicy() == NoFocus )
        return;
#endif
    if ( d_data->legend.isNull()
            || d_data->layout->legendPosition() == ExternalLegend
            || d_data->legend->legendItems().count() == 0 ) {
        return;
    }

    // Depending on the position of the legend the
    // tab order will be changed that the canvas is
    // next to the last legend item, or before
    // the first one.

    const bool canvasFirst =
        d_data->layout->legendPosition() == QwtPlot::BottomLegend ||
        d_data->layout->legendPosition() == QwtPlot::RightLegend;

    QWidget *previous = NULL;

    QWidget *w;
#if QT_VERSION >= 0x040000
    w = d_data->canvas;
    while ( ( w = w->nextInFocusChain() ) != d_data->canvas )
#else
    if ( focusData() == NULL )
        return;

    while ( focusData()->next() != d_data->canvas );
    while ( (w = focusData()->next()) != d_data->canvas )
#endif
    {
        bool isLegendItem = false;
        if ( w->focusPolicy() != NoFocus
                && w->parent() && w->parent() == d_data->legend->contentsWidget() ) {
            isLegendItem = true;
        }

        if ( canvasFirst ) {
            if ( isLegendItem )
                break;

            previous = w;
        } else {
            if ( isLegendItem )
                previous = w;
            else {
                if ( previous )
                    break;
            }
        }
    }

    if ( previous && previous != d_data->canvas)
        setTabOrder(previous, d_data->canvas);
}

/*!
  Redraw the canvas.
  \param painter Painter used for drawing

  \warning drawCanvas calls drawItems what is also used
           for printing. Applications that like to add individual
           plot items better overload drawItems()
  \sa drawItems()
*/
void QwtPlot::drawCanvas(QPainter *painter)
{
    QwtScaleMap maps[axisCnt];
    for ( int axisId = 0; axisId < axisCnt; axisId++ )
        maps[axisId] = canvasMap(axisId);

    drawItems(painter,
              d_data->canvas->contentsRect(), maps, QwtPlotPrintFilter());
}

/*!
  Redraw the canvas items.
  \param painter Painter used for drawing
  \param rect Bounding rectangle where to paint
  \param map QwtPlot::axisCnt maps, mapping between plot and paint device coordinates
  \param pfilter Plot print filter
*/

void QwtPlot::drawItems(QPainter *painter, const QRect &rect,
                        const QwtScaleMap map[axisCnt],
                        const QwtPlotPrintFilter &pfilter) const
{
    const QwtPlotItemList& itmList = itemList();
    for ( QwtPlotItemIterator it = itmList.begin();
            it != itmList.end(); ++it ) {
        QwtPlotItem *item = *it;
        if ( item && item->isVisible() ) {
            if ( !(pfilter.options() & QwtPlotPrintFilter::PrintGrid)
                    && item->rtti() == QwtPlotItem::Rtti_PlotGrid ) {
                continue;
            }

            painter->save();

#if QT_VERSION >= 0x040000
            painter->setRenderHint(QPainter::Antialiasing,
                                   item->testRenderHint(QwtPlotItem::RenderAntialiased) );
#endif

            item->draw(painter,
                       map[item->xAxis()], map[item->yAxis()],
                       rect);

            painter->restore();
        }
    }
}

/*!
  \param axisId Axis
  \return Map for the axis on the canvas. With this map pixel coordinates can
          translated to plot coordinates and vice versa.
  \sa QwtScaleMap, transform(), invTransform()

*/
QwtScaleMap QwtPlot::canvasMap(int axisId) const
{
    QwtScaleMap map;
    if ( !d_data->canvas )
        return map;

    map.setTransformation(axisScaleEngine(axisId)->transformation());

    const QwtScaleDiv *sd = axisScaleDiv(axisId);
    map.setScaleInterval(sd->lBound(), sd->hBound());

    if ( axisEnabled(axisId) ) {
        const QwtScaleWidget *s = axisWidget(axisId);
        if ( axisId == yLeft || axisId == yRight ) {
            int y = s->y() + s->startBorderDist() - d_data->canvas->y();
            int h = s->height() - s->startBorderDist() - s->endBorderDist();
            map.setPaintInterval(y + h, y);
        } else {
            int x = s->x() + s->startBorderDist() - d_data->canvas->x();
            int w = s->width() - s->startBorderDist() - s->endBorderDist();
            map.setPaintInterval(x, x + w);
        }
    } else {
        const int margin = plotLayout()->canvasMargin(axisId);

        const QRect &canvasRect = d_data->canvas->contentsRect();
        if ( axisId == yLeft || axisId == yRight ) {
            map.setPaintInterval(canvasRect.bottom() - margin,
                                 canvasRect.top() + margin);
        } else {
            map.setPaintInterval(canvasRect.left() + margin,
                                 canvasRect.right() - margin);
        }
    }
    return map;
}

/*!
  Change the margin of the plot. The margin is the space
  around all components.

  \param margin new margin
  \sa QwtPlotLayout::setMargin(), margin(), plotLayout()
*/
void QwtPlot::setMargin(int margin)
{
    if ( margin < 0 )
        margin = 0;

    if ( margin != d_data->layout->margin() ) {
        d_data->layout->setMargin(margin);
        updateLayout();
    }
}

/*!
    \return margin
    \sa setMargin(), QwtPlotLayout::margin(), plotLayout()
*/
int QwtPlot::margin() const
{
    return d_data->layout->margin();
}

/*!
  \brief Change the background of the plotting area

  Sets c to QColorGroup::Background of all colorgroups of
  the palette of the canvas. Using canvas()->setPalette()
  is a more powerful way to set these colors.
  \param c new background color
*/
void QwtPlot::setCanvasBackground(const QColor &c)
{
    QPalette p = d_data->canvas->palette();

    for ( int i = 0; i < QPalette::NColorGroups; i++ ) {
#if QT_VERSION < 0x040000
        p.setColor((QPalette::ColorGroup)i, QColorGroup::Background, c);
#else
        p.setColor((QPalette::ColorGroup)i, QPalette::Background, c);
#endif
    }

    canvas()->setPalette(p);
}

/*!
  Nothing else than: canvas()->palette().color(
        QPalette::Normal, QColorGroup::Background);

  \return the background color of the plotting area.
*/
const QColor & QwtPlot::canvasBackground() const
{
#if QT_VERSION < 0x040000
    return canvas()->palette().color(
               QPalette::Normal, QColorGroup::Background);
#else
    return canvas()->palette().color(
               QPalette::Normal, QPalette::Background);
#endif
}

/*!
  \brief Change the border width of the plotting area
  Nothing else than canvas()->setLineWidth(w),
  left for compatibility only.
  \param w new border width
*/
void QwtPlot::setCanvasLineWidth(int w)
{
    canvas()->setLineWidth(w);
    updateLayout();
}

/*!
  Nothing else than: canvas()->lineWidth(),
  left for compatibility only.
  \return the border width of the plotting area
*/
int QwtPlot::canvasLineWidth() const
{
    return canvas()->lineWidth();
}

/*!
  \return \c true if the specified axis exists, otherwise \c false
  \param axisId axis index
 */
bool QwtPlot::axisValid(int axisId)
{
    return ((axisId >= QwtPlot::yLeft) && (axisId < QwtPlot::axisCnt));
}

/*!
  Called internally when the legend has been clicked on.
  Emits a legendClicked() signal.
*/
void QwtPlot::legendItemClicked()
{
    if ( d_data->legend && sender()->isWidgetType() ) {
        QwtPlotItem *plotItem =
            (QwtPlotItem*)d_data->legend->find((QWidget *)sender());
        if ( plotItem )
            emit legendClicked(plotItem);
    }
}

/*!
  Called internally when the legend has been checked
  Emits a legendClicked() signal.
*/
void QwtPlot::legendItemChecked(bool on)
{
    if ( d_data->legend && sender()->isWidgetType() ) {
        QwtPlotItem *plotItem =
            (QwtPlotItem*)d_data->legend->find((QWidget *)sender());
        if ( plotItem )
            emit legendChecked(plotItem, on);
    }
}

//! Remove all curves and markers
void QwtPlot::clear()
{
    detachItems(QwtPlotItem::Rtti_PlotCurve);
    detachItems(QwtPlotItem::Rtti_PlotMarker);
}

/*!
  \brief Insert a legend

  If the position legend is \c QwtPlot::LeftLegend or \c QwtPlot::RightLegend
  the legend will be organized in one column from top to down.
  Otherwise the legend items will be placed in a table
  with a best fit number of columns from left to right.

  If pos != QwtPlot::ExternalLegend the plot widget will become
  parent of the legend. It will be deleted when the plot is deleted,
  or another legend is set with insertLegend().

  \param legend Legend
  \param pos The legend's position. For top/left position the number
             of colums will be limited to 1, otherwise it will be set to
             unlimited.

  \param ratio Ratio between legend and the bounding rect
               of title, canvas and axes. The legend will be shrinked
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.

  \sa legend(), QwtPlotLayout::legendPosition(),
      QwtPlotLayout::setLegendPosition()
*/
void QwtPlot::insertLegend(QwtLegend *legend,
                           QwtPlot::LegendPosition pos, double ratio)
{
    d_data->layout->setLegendPosition(pos, ratio);

    if ( legend != d_data->legend ) {
        if ( d_data->legend && d_data->legend->parent() == this )
            delete d_data->legend;

        d_data->legend = legend;

        if ( d_data->legend ) {
            if ( pos != ExternalLegend ) {
                if ( d_data->legend->parent() != this ) {
#if QT_VERSION < 0x040000
                    d_data->legend->reparent(this, QPoint(0, 0));
#else
                    d_data->legend->setParent(this);
#endif
                }
            }

            const QwtPlotItemList& itmList = itemList();
            for ( QwtPlotItemIterator it = itmList.begin();
                    it != itmList.end(); ++it ) {
                (*it)->updateLegend(d_data->legend);
            }

            QLayout *l = d_data->legend->contentsWidget()->layout();
            if ( l && l->inherits("QwtDynGridLayout") ) {
                QwtDynGridLayout *tl = (QwtDynGridLayout *)l;
                switch(d_data->layout->legendPosition()) {
                case LeftLegend:
                case RightLegend:
                    tl->setMaxCols(1); // 1 column: align vertical
                    break;
                case TopLegend:
                case BottomLegend:
                    tl->setMaxCols(0); // unlimited
                    break;
                case ExternalLegend:
                    break;
                }
            }
        }
        updateTabOrder();
    }

    updateLayout();
}
