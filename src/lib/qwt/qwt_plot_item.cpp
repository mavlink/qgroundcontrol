/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_text.h"
#include "qwt_plot.h"
#include "qwt_legend.h"
#include "qwt_legend_item.h"
#include "qwt_plot_item.h"

class QwtPlotItem::PrivateData
{
public:
    PrivateData():
        plot(NULL),
        isVisible(true),
        attributes(0),
#if QT_VERSION >= 0x040000
        renderHints(0),
#endif
        z(0.0),
        xAxis(QwtPlot::xBottom),
        yAxis(QwtPlot::yLeft) {
    }

    mutable QwtPlot *plot;

    bool isVisible;
    int attributes;
#if QT_VERSION >= 0x040000
    int renderHints;
#endif
    double z;

    int xAxis;
    int yAxis;

    QwtText title;
};

//! Constructor
QwtPlotItem::QwtPlotItem(const QwtText &title)
{
    d_data = new PrivateData;
    d_data->title = title;
}

//! Destroy the QwtPlotItem
QwtPlotItem::~QwtPlotItem()
{
    attach(NULL);
    delete d_data;
}

/*!
  \brief Attach the item to a plot.

  This method will attach a QwtPlotItem to the QwtPlot argument. It will first
  detach the QwtPlotItem from any plot from a previous call to attach (if
  necessary). If a NULL argument is passed, it will detach from any QwtPlot it
  was attached to.

  \sa QwtPlotItem::detach()
*/
void QwtPlotItem::attach(QwtPlot *plot)
{
    if ( plot == d_data->plot )
        return;

    // remove the item from the previous plot

    if ( d_data->plot ) {
        if ( d_data->plot->legend() ) {
            QWidget *legendItem = d_data->plot->legend()->find(this);
            if ( legendItem )
                delete legendItem;
        }

        d_data->plot->attachItem(this, false);

        if ( d_data->plot->autoReplot() )
            d_data->plot->update();
    }

    d_data->plot = plot;

    if ( d_data->plot ) {
        // insert the item into the current plot

        d_data->plot->attachItem(this, true);
        itemChanged();
    }
}

/*!
   Return rtti for the specific class represented. QwtPlotItem is simply
   a virtual interface class, and base classes will implement this method
   with specific rtti values so a user can differentiate them.

   The rtti value is useful for environments, where the
   runtime type information is disabled and it is not possible
   to do a dynamic_cast<...>.

   \return rtti value
   \sa RttiValues
*/
int QwtPlotItem::rtti() const
{
    return Rtti_PlotItem;
}

//! Return attached plot
QwtPlot *QwtPlotItem::plot() const
{
    return d_data->plot;
}

/*!
   Plot items are painted in increasing z-order.

   \return setZ(), QwtPlotDict::itemList()
*/
double QwtPlotItem::z() const
{
    return d_data->z;
}

/*!
   \brief Set the z value

   Plot items are painted in increasing z-order.

   \param z Z-value
   \sa z(), QwtPlotDict::itemList()
*/
void QwtPlotItem::setZ(double z)
{
    if ( d_data->z != z ) {
        d_data->z = z;
        if ( d_data->plot ) {
            // update the z order
            d_data->plot->attachItem(this, false);
            d_data->plot->attachItem(this, true);
        }
        itemChanged();
    }
}

/*!
   Set a new title

   \param title Title
   \sa title()
*/
void QwtPlotItem::setTitle(const QString &title)
{
    setTitle(QwtText(title));
}

/*!
   Set a new title

   \param title Title
   \sa title()
*/
void QwtPlotItem::setTitle(const QwtText &title)
{
    if ( d_data->title != title ) {
        d_data->title = title;
        itemChanged();
    }
}

/*!
   \return Title of the item
   \sa setTitle()
*/
const QwtText &QwtPlotItem::title() const
{
    return d_data->title;
}

/*!
   Toggle an item attribute

   \param attribute Attribute type
   \param on true/false

   \sa testItemAttribute(), ItemAttribute
*/
void QwtPlotItem::setItemAttribute(ItemAttribute attribute, bool on)
{
    if ( bool(d_data->attributes & attribute) != on ) {
        if ( on )
            d_data->attributes |= attribute;
        else
            d_data->attributes &= ~attribute;

        itemChanged();
    }
}

/*!
   Test an item attribute

   \param ItemAttribute Attribute type
   \return true/false
   \sa setItemAttribute(), ItemAttribute
*/
bool QwtPlotItem::testItemAttribute(ItemAttribute attribute) const
{
    return d_data->attributes & attribute;
}

#if QT_VERSION >= 0x040000

/*!
   Toggle an render hint

   \param hint Render hint
   \param on true/false

   \sa testRenderHint(), RenderHint
*/
void QwtPlotItem::setRenderHint(RenderHint hint, bool on)
{
    if ( ((d_data->renderHints & hint) != 0) != on ) {
        if ( on )
            d_data->renderHints |= hint;
        else
            d_data->renderHints &= ~hint;

        itemChanged();
    }
}

/*!
   Test a render hint

   \param hint Render hint
   \return true/false
   \sa setRenderHint(), RenderHint
*/
bool QwtPlotItem::testRenderHint(RenderHint hint) const
{
    return (d_data->renderHints & hint);
}

#endif

//! Show the item
void QwtPlotItem::show()
{
    setVisible(true);
}

//! Hide the item
void QwtPlotItem::hide()
{
    setVisible(false);
}

/*!
    Show/Hide the item

    \param on Show if true, otherwise hide
    \sa isVisible(), show(), hide()
*/
void QwtPlotItem::setVisible(bool on)
{
    if ( on != d_data->isVisible ) {
        d_data->isVisible = on;
        itemChanged();
    }
}

/*!
    \return true if visible
    \sa setVisible(), show(), hide()
*/
bool QwtPlotItem::isVisible() const
{
    return d_data->isVisible;
}

/*!
   Update the legend and call QwtPlot::autoRefresh for the
   parent plot.

   \sa updateLegend()
*/
void QwtPlotItem::itemChanged()
{
    if ( d_data->plot ) {
        if ( d_data->plot->legend() )
            updateLegend(d_data->plot->legend());

        d_data->plot->autoRefresh();
    }
}

/*!
   Set X and Y axis

   The item will painted according to the coordinates its Axes.

   \param xAxis X Axis
   \param yAxis Y Axis

   \sa setXAxis(), setYAxis(), xAxis(), yAxis()
*/
void QwtPlotItem::setAxis(int xAxis, int yAxis)
{
    if (xAxis == QwtPlot::xBottom || xAxis == QwtPlot::xTop )
        d_data->xAxis = xAxis;

    if (yAxis == QwtPlot::yLeft || yAxis == QwtPlot::yRight )
        d_data->yAxis = yAxis;

    itemChanged();
}

/*!
   Set the X axis

   The item will painted according to the coordinates its Axes.

   \param axis X Axis
   \sa setAxis(), setYAxis(), xAxis()
*/
void QwtPlotItem::setXAxis(int axis)
{
    if (axis == QwtPlot::xBottom || axis == QwtPlot::xTop ) {
        d_data->xAxis = axis;
        itemChanged();
    }
}

/*!
   Set the Y axis

   The item will painted according to the coordinates its Axes.

   \param axis Y Axis
   \sa setAxis(), setXAxis(), yAxis()
*/
void QwtPlotItem::setYAxis(int axis)
{
    if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight ) {
        d_data->yAxis = axis;
        itemChanged();
    }
}

//! Return xAxis
int QwtPlotItem::xAxis() const
{
    return d_data->xAxis;
}

//! Return yAxis
int QwtPlotItem::yAxis() const
{
    return d_data->yAxis;
}

/*!
   \return An invalid bounding rect: QwtDoubleRect(1.0, 1.0, -2.0, -2.0)
*/
QwtDoubleRect QwtPlotItem::boundingRect() const
{
    return QwtDoubleRect(1.0, 1.0, -2.0, -2.0); // invalid
}

/*!
   \brief Allocate the widget that represents the item on the legend

   The default implementation is made for QwtPlotCurve and returns a
   QwtLegendItem(), but an item could be represented by any type of widget,
   by overloading legendItem() and updateLegend().

   \return QwtLegendItem()
   \sa updateLegend() QwtLegend()
*/
QWidget *QwtPlotItem::legendItem() const
{
    return new QwtLegendItem;
}

/*!
   \brief Update the widget that represents the item on the legend

   updateLegend() is called from itemChanged() to adopt the widget
   representing the item on the legend to its new configuration.

   The default implementation is made for QwtPlotCurve and updates a
   QwtLegendItem(), but an item could be represented by any type of widget,
   by overloading legendItem() and updateLegend().

   \sa legendItem(), itemChanged(), QwtLegend()
*/
void QwtPlotItem::updateLegend(QwtLegend *legend) const
{
    if ( !legend )
        return;

    QWidget *lgdItem = legend->find(this);
    if ( testItemAttribute(QwtPlotItem::Legend) ) {
        if ( lgdItem == NULL ) {
            lgdItem = legendItem();
            if ( lgdItem ) {
                if ( lgdItem->inherits("QwtLegendItem") ) {
                    QwtLegendItem *label = (QwtLegendItem *)lgdItem;
                    label->setItemMode(legend->itemMode());

                    if ( d_data->plot ) {
                        QObject::connect(label, SIGNAL(clicked()),
                                         d_data->plot, SLOT(legendItemClicked()));
                        QObject::connect(label, SIGNAL(checked(bool)),
                                         d_data->plot, SLOT(legendItemChecked(bool)));
                    }
                }
                legend->insert(this, lgdItem);
            }
        }
        if ( lgdItem && lgdItem->inherits("QwtLegendItem") ) {
            QwtLegendItem* label = (QwtLegendItem*)lgdItem;
            if ( label )
                label->setText(d_data->title);
        }
    } else {
        delete lgdItem;
    }
}

/*!
   \brief Update the item to changes of the axes scale division

   Update the item, when the axes of plot have changed.
   The default implementation does nothing, but items that depend
   on the scale division (like QwtPlotGrid()) have to reimplement
   updateScaleDiv()

   \param xScaleDiv Scale division of the x-axis
   \param yScaleDiv Scale division of the y-axis

   \sa QwtPlot::updateAxes()
*/
void QwtPlotItem::updateScaleDiv(const QwtScaleDiv &,
                                 const QwtScaleDiv &)
{
}

/*!
   \brief Calculate the bounding scale rect of 2 maps

   \param xMap X map
   \param yMap X map

   \return Bounding rect of the scale maps
*/
QwtDoubleRect QwtPlotItem::scaleRect(const QwtScaleMap &xMap,
                                     const QwtScaleMap &yMap) const
{
    return QwtDoubleRect(xMap.s1(), yMap.s1(),
                         xMap.sDist(), yMap.sDist() );
}

/*!
   \brief Calculate the bounding paint rect of 2 maps

   \param xMap X map
   \param yMap X map

   \return Bounding rect of the scale maps
*/
QRect QwtPlotItem::paintRect(const QwtScaleMap &xMap,
                             const QwtScaleMap &yMap) const
{
    const QRect rect( qRound(xMap.p1()), qRound(yMap.p1()),
                      qRound(xMap.pDist()), qRound(yMap.pDist()) );

    return rect;
}

/*!
   Transform a rectangle

   \param xMap X map
   \param yMap Y map
   \param rect Rectangle in scale coordinates
   \return Rectangle in paint coordinates

   \sa invTransform()
*/
QRect QwtPlotItem::transform(const QwtScaleMap &xMap,
                             const QwtScaleMap &yMap, const QwtDoubleRect& rect) const
{
    int x1 = qRound(xMap.transform(rect.left()));
    int x2 = qRound(xMap.transform(rect.right()));
    int y1 = qRound(yMap.transform(rect.top()));
    int y2 = qRound(yMap.transform(rect.bottom()));

    if ( x2 < x1 )
        qSwap(x1, x2);
    if ( y2 < y1 )
        qSwap(y1, y2);

    return QRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

/*!
   Transform a rectangle from paint to scale coordinates

   \param xMap X map
   \param yMap Y map
   \param rect Rectangle in paint coordinates
   \return Rectangle in scale coordinates
   \sa transform()
*/
QwtDoubleRect QwtPlotItem::invTransform(const QwtScaleMap &xMap,
                                        const QwtScaleMap &yMap, const QRect& rect) const
{
    const double x1 = xMap.invTransform(rect.left());
    const double x2 = xMap.invTransform(rect.right());
    const double y1 = yMap.invTransform(rect.top());
    const double y2 = yMap.invTransform(rect.bottom());

    const QwtDoubleRect r(x1, y1, x2 - x1, y2 - y1);

    return r.normalized();
}
