/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_H
#define QWT_PLOT_H

#include <qframe.h>
#include "qwt_global.h"
#include "qwt_array.h"
#include "qwt_text.h"
#include "qwt_plot_dict.h"
#include "qwt_scale_map.h"
#include "qwt_plot_printfilter.h"

class QwtPlotLayout;
class QwtLegend;
class QwtScaleWidget;
class QwtScaleEngine;
class QwtScaleDiv;
class QwtScaleDraw;
class QwtTextLabel;
class QwtPlotCanvas;
class QwtPlotPrintFilter;

/*!
  \brief A 2-D plotting widget

  QwtPlot is a widget for plotting two-dimensional graphs.
  An unlimited number of plot items can be displayed on
  its canvas. Plot items might be curves (QwtPlotCurve), markers
  (QwtPlotMarker), the grid (QwtPlotGrid), or anything else derived
  from QwtPlotItem.
  A plot can have up to four axes, with each plot item attached to an x- and
  a y axis. The scales at the axes can be explicitely set (QwtScaleDiv), or
  are calculated from the plot items, using algorithms (QwtScaleEngine) which
  can be configured separately for each axis.

  \image html plot.png

  \par Example
  The following example shows (schematically) the most simple
  way to use QwtPlot. By default, only the left and bottom axes are
  visible and their scales are computed automatically.
  \verbatim
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

QwtPlot *myPlot;
double x[100], y1[100], y2[100];        // x and y values

myPlot = new QwtPlot("Two Curves", parent);

// add curves
QwtPlotCurve *curve1 = new QwtPlotCurve("Curve 1");
QwtPlotCurve *curve2 = new QwtPlotCurve("Curve 2");

getSomeValues(x, y1, y2);

// copy the data into the curves
curve1->setData(x, y1, 100);
curve2->setData(x, y2, 100);

curve1->attach(myPlot);
curve2->attach(myPlot);

// finally, refresh the plot
myPlot->replot();
\endverbatim
*/

class QWT_EXPORT QwtPlot: public QFrame, public QwtPlotDict
{
    friend class QwtPlotCanvas;

    Q_OBJECT
    Q_PROPERTY( QString propertiesDocument
                READ grabProperties WRITE applyProperties )

public:
    //! Axis index
    enum Axis {
        yLeft,
        yRight,
        xBottom,
        xTop,

        axisCnt
    };

    /*!
        \brief Position of the legend, relative to the canvas.

        ExternalLegend means that only the content of the legend
        will be handled by QwtPlot, but not its geometry.
        This might be interesting if an application wants to
        have a legend in an external window.
     */
    enum LegendPosition {
        LeftLegend,
        RightLegend,
        BottomLegend,
        TopLegend,

        ExternalLegend
    };

    explicit QwtPlot(QWidget * = NULL);
    explicit QwtPlot(const QwtText &title, QWidget *p = NULL);
#if QT_VERSION < 0x040000
    explicit QwtPlot(QWidget *, const char* name);
#endif

    virtual ~QwtPlot();

    void applyProperties(const QString &);
    QString grabProperties() const;

    void setAutoReplot(bool tf = true);
    bool autoReplot() const;

    void print(QPaintDevice &p,
               const QwtPlotPrintFilter & = QwtPlotPrintFilter()) const;
    virtual void print(QPainter *, const QRect &rect,
                       const QwtPlotPrintFilter & = QwtPlotPrintFilter()) const;

    // Layout

    QwtPlotLayout *plotLayout();
    const QwtPlotLayout *plotLayout() const;

    void setMargin(int margin);
    int margin() const;

    // Title

    void setTitle(const QString &);
    void setTitle(const QwtText &t);
    QwtText title() const;

    QwtTextLabel *titleLabel();
    const QwtTextLabel *titleLabel() const;

    // Canvas

    QwtPlotCanvas *canvas();
    const QwtPlotCanvas *canvas() const;

    void setCanvasBackground (const QColor &c);
    const QColor& canvasBackground() const;

    void setCanvasLineWidth(int w);
    int canvasLineWidth() const;

    virtual QwtScaleMap canvasMap(int axisId) const;

    double invTransform(int axisId, int pos) const;
    int transform(int axisId, double value) const;

    // Axes

    QwtScaleEngine *axisScaleEngine(int axisId);
    const QwtScaleEngine *axisScaleEngine(int axisId) const;
    void setAxisScaleEngine(int axisId, QwtScaleEngine *);

    void setAxisAutoScale(int axisId);
    bool axisAutoScale(int axisId) const;

    void enableAxis(int axisId, bool tf = true);
    bool axisEnabled(int axisId) const;

    void setAxisFont(int axisId, const QFont &f);
    QFont axisFont(int axisId) const;

    void setAxisScale(int axisId, double min, double max, double step = 0);
    void setAxisScaleDiv(int axisId, const QwtScaleDiv &);
    void setAxisScaleDraw(int axisId, QwtScaleDraw *);

    double axisStepSize(int axisId) const;

    const QwtScaleDiv *axisScaleDiv(int axisId) const;
    QwtScaleDiv *axisScaleDiv(int axisId);

    const QwtScaleDraw *axisScaleDraw(int axisId) const;
    QwtScaleDraw *axisScaleDraw(int axisId);

    const QwtScaleWidget *axisWidget(int axisId) const;
    QwtScaleWidget *axisWidget(int axisId);

#if QT_VERSION < 0x040000
    void setAxisLabelAlignment(int axisId, int);
#else
    void setAxisLabelAlignment(int axisId, Qt::Alignment);
#endif
    void setAxisLabelRotation(int axisId, double rotation);

    void setAxisTitle(int axisId, const QString &);
    void setAxisTitle(int axisId, const QwtText &);
    QwtText axisTitle(int axisId) const;

    void setAxisMaxMinor(int axisId, int maxMinor);
    int axisMaxMajor(int axisId) const;
    void setAxisMaxMajor(int axisId, int maxMajor);
    int axisMaxMinor(int axisId) const;

    // Legend

    void insertLegend(QwtLegend *, LegendPosition = QwtPlot::RightLegend,
                      double ratio = -1.0);

    QwtLegend *legend();
    const QwtLegend *legend() const;

    // Misc

    virtual void polish();
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    virtual void updateLayout();

    virtual bool event(QEvent *);

signals:
    /*!
      A signal which is emitted when the user has clicked on
      a legend item, which is in QwtLegend::ClickableItem mode.

      \param plotItem Corresponding plot item of the
                 selected legend item

      \note clicks are disabled as default
      \sa QwtLegend::setItemMode, QwtLegend::itemMode
     */
    void legendClicked(QwtPlotItem *plotItem);

    /*!
      A signal which is emitted when the user has clicked on
      a legend item, which is in QwtLegend::CheckableItem mode

      \param plotItem Corresponding plot item of the
                 selected legend item
      \param on True when the legen item is checked

      \note clicks are disabled as default
      \sa QwtLegend::setItemMode, QwtLegend::itemMode
     */

    void legendChecked(QwtPlotItem *plotItem, bool on);

public slots:
    virtual void clear();

    virtual void replot();
    void autoRefresh();

protected slots:
    virtual void legendItemClicked();
    virtual void legendItemChecked(bool);

protected:
    static bool axisValid(int axisId);

    virtual void drawCanvas(QPainter *);
    virtual void drawItems(QPainter *, const QRect &,
                           const QwtScaleMap maps[axisCnt],
                           const QwtPlotPrintFilter &) const;

    virtual void updateTabOrder();

    void updateAxes();

    virtual void resizeEvent(QResizeEvent *e);

    virtual void printLegendItem(QPainter *,
                                 const QWidget *, const QRect &) const;

    virtual void printTitle(QPainter *, const QRect &) const;

    virtual void printScale(QPainter *, int axisId, int startDist, int endDist,
                            int baseDist, const QRect &) const;

    virtual void printCanvas(QPainter *,
                             const QRect &boundingRect, const QRect &canvasRect,
                             const QwtScaleMap maps[axisCnt], const QwtPlotPrintFilter &) const;

    virtual void printLegend(QPainter *, const QRect &) const;

private:
    void initAxesData();
    void deleteAxesData();
    void updateScaleDiv();

    void initPlot(const QwtText &title);

    class AxisData;
    AxisData *d_axisData[axisCnt];

    class PrivateData;
    PrivateData *d_data;
};

#endif
