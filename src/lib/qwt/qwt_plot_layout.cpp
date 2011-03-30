/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qscrollbar.h>
#include "qwt_text.h"
#include "qwt_text_label.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_widget.h"
#include "qwt_legend.h"
#include "qwt_plot_layout.h"

class QwtPlotLayout::LayoutData
{
public:
    void init(const QwtPlot *, const QRect &rect);

    struct t_legendData {
        int frameWidth;
        int vScrollBarWidth;
        int hScrollBarHeight;
        QSize hint;
    } legend;

    struct t_titleData {
        QwtText text;
        int frameWidth;
    } title;

    struct t_scaleData {
        bool isEnabled;
        const QwtScaleWidget *scaleWidget;
        QFont scaleFont;
        int start;
        int end;
        int baseLineOffset;
        int tickOffset;
        int dimWithoutTitle;
    } scale[QwtPlot::axisCnt];

    struct t_canvasData {
        int frameWidth;
    } canvas;
};

/*
  Extract all layout relevant data from the plot components
*/

void QwtPlotLayout::LayoutData::init(const QwtPlot *plot, const QRect &rect)
{
    // legend

    if ( plot->plotLayout()->legendPosition() != QwtPlot::ExternalLegend
            && plot->legend() ) {
        legend.frameWidth = plot->legend()->frameWidth();
        legend.vScrollBarWidth =
            plot->legend()->verticalScrollBar()->sizeHint().width();
        legend.hScrollBarHeight =
            plot->legend()->horizontalScrollBar()->sizeHint().height();

        const QSize hint = plot->legend()->sizeHint();

        int w = qwtMin(hint.width(), rect.width());
        int h = plot->legend()->heightForWidth(w);
        if ( h == 0 )
            h = hint.height();

        if ( h > rect.height() )
            w += legend.vScrollBarWidth;

        legend.hint = QSize(w, h);
    }

    // title

    title.frameWidth = 0;
    title.text = QwtText();

    if (plot->titleLabel() ) {
        const QwtTextLabel *label = plot->titleLabel();
        title.text = label->text();
        if ( !(title.text.testPaintAttribute(QwtText::PaintUsingTextFont)) )
            title.text.setFont(label->font());

        title.frameWidth = plot->titleLabel()->frameWidth();
    }

    // scales

    for (int axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        if ( plot->axisEnabled(axis) ) {
            const QwtScaleWidget *scaleWidget = plot->axisWidget(axis);

            scale[axis].isEnabled = true;

            scale[axis].scaleWidget = scaleWidget;

            scale[axis].scaleFont = scaleWidget->font();

            scale[axis].start = scaleWidget->startBorderDist();
            scale[axis].end = scaleWidget->endBorderDist();

            scale[axis].baseLineOffset = scaleWidget->margin();
            scale[axis].tickOffset = scaleWidget->margin();
            if ( scaleWidget->scaleDraw()->hasComponent(
                        QwtAbstractScaleDraw::Ticks) ) {
                scale[axis].tickOffset +=
                    (int)scaleWidget->scaleDraw()->majTickLength();
            }

            scale[axis].dimWithoutTitle = scaleWidget->dimForLength(
                                              QWIDGETSIZE_MAX, scale[axis].scaleFont);

            if ( !scaleWidget->title().isEmpty() ) {
                scale[axis].dimWithoutTitle -=
                    scaleWidget->titleHeightForWidth(QWIDGETSIZE_MAX);
            }
        } else {
            scale[axis].isEnabled = false;
            scale[axis].start = 0;
            scale[axis].end = 0;
            scale[axis].baseLineOffset = 0;
            scale[axis].tickOffset = 0;
            scale[axis].dimWithoutTitle = 0;
        }
    }

    // canvas

    canvas.frameWidth = plot->canvas()->frameWidth();
}

class QwtPlotLayout::PrivateData
{
public:
    PrivateData():
        margin(0),
        spacing(5),
        alignCanvasToScales(false) {
    }

    QRect titleRect;
    QRect legendRect;
    QRect scaleRect[QwtPlot::axisCnt];
    QRect canvasRect;

    QwtPlotLayout::LayoutData layoutData;

    QwtPlot::LegendPosition legendPos;
    double legendRatio;
    unsigned int margin;
    unsigned int spacing;
    unsigned int canvasMargin[QwtPlot::axisCnt];
    bool alignCanvasToScales;
};

/*!
  \brief Constructor
 */

QwtPlotLayout::QwtPlotLayout()
{
    d_data = new PrivateData;

    setLegendPosition(QwtPlot::BottomLegend);
    setCanvasMargin(4);

    invalidate();
}

//! Destructor
QwtPlotLayout::~QwtPlotLayout()
{
    delete d_data;
}

/*!
  Change the margin of the plot. The margin is the space
  around all components.

  \param margin new margin
  \sa margin(), setSpacing(),
      QwtPlot::setMargin()
*/
void QwtPlotLayout::setMargin(int margin)
{
    if ( margin < 0 )
        margin = 0;
    d_data->margin = margin;
}

/*!
    \return margin
    \sa setMargin(), spacing(), QwtPlot::margin()
*/
int QwtPlotLayout::margin() const
{
    return d_data->margin;
}

/*!
  Change a margin of the canvas. The margin is the space
  above/below the scale ticks. A negative margin will
  be set to -1, excluding the borders of the scales.

  \param margin New margin
  \param axis One of QwtPlot::Axis. Specifies where the position of the margin.
              -1 means margin at all borders.
  \sa canvasMargin()

  \warning The margin will have no effect when alignCanvasToScales is true
*/

void QwtPlotLayout::setCanvasMargin(int margin, int axis)
{
    if ( margin < -1 )
        margin = -1;

    if ( axis == -1 ) {
        for (axis = 0; axis < QwtPlot::axisCnt; axis++)
            d_data->canvasMargin[axis] = margin;
    } else if ( axis >= 0 && axis < QwtPlot::axisCnt )
        d_data->canvasMargin[axis] = margin;
}

/*!
    \return Margin around the scale tick borders
    \sa setCanvasMargin()
*/
int QwtPlotLayout::canvasMargin(int axis) const
{
    if ( axis < 0 || axis >= QwtPlot::axisCnt )
        return 0;

    return d_data->canvasMargin[axis];
}

/*!
  Change the align-canvas-to-axis-scales setting. The canvas may:
  - extend beyond the axis scale ends to maximize its size,
  - align with the axis scale ends to control its size.

  \param alignCanvasToScales New align-canvas-to-axis-scales setting

  \sa setCanvasMargin()
  \note In this context the term 'scale' means the backbone of a scale.
  \warning In case of alignCanvasToScales == true canvasMargin will have
           no effect
*/
void QwtPlotLayout::setAlignCanvasToScales(bool alignCanvasToScales)
{
    d_data->alignCanvasToScales = alignCanvasToScales;
}

/*!
  Return the align-canvas-to-axis-scales setting. The canvas may:
  - extend beyond the axis scale ends to maximize its size
  - align with the axis scale ends to control its size.

  \return align-canvas-to-axis-scales setting
  \sa setAlignCanvasToScales, setCanvasMargin()
  \note In this context the term 'scale' means the backbone of a scale.
*/
bool QwtPlotLayout::alignCanvasToScales() const
{
    return d_data->alignCanvasToScales;
}

/*!
  Change the spacing of the plot. The spacing is the distance
  between the plot components.

  \param spacing new spacing
  \sa setMargin(), spacing()
*/
void QwtPlotLayout::setSpacing(int spacing)
{
    d_data->spacing = qwtMax(0, spacing);
}

/*!
  \return spacing
  \sa margin(), setSpacing()
*/
int QwtPlotLayout::spacing() const
{
    return d_data->spacing;
}

/*!
  \brief Specify the position of the legend
  \param pos The legend's position.
  \param ratio Ratio between legend and the bounding rect
               of title, canvas and axes. The legend will be shrinked
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.

  \sa QwtPlot::setLegendPosition()
*/

void QwtPlotLayout::setLegendPosition(QwtPlot::LegendPosition pos, double ratio)
{
    if ( ratio > 1.0 )
        ratio = 1.0;

    switch(pos) {
    case QwtPlot::TopLegend:
    case QwtPlot::BottomLegend:
        if ( ratio <= 0.0 )
            ratio = 0.33;
        d_data->legendRatio = ratio;
        d_data->legendPos = pos;
        break;
    case QwtPlot::LeftLegend:
    case QwtPlot::RightLegend:
        if ( ratio <= 0.0 )
            ratio = 0.5;
        d_data->legendRatio = ratio;
        d_data->legendPos = pos;
        break;
    case QwtPlot::ExternalLegend:
        d_data->legendRatio = ratio; // meaningless
        d_data->legendPos = pos;
    default:
        break;
    }
}

/*!
  \brief Specify the position of the legend
  \param pos The legend's position. Valid values are
      \c QwtPlot::LeftLegend, \c QwtPlot::RightLegend,
      \c QwtPlot::TopLegend, \c QwtPlot::BottomLegend.

  \sa QwtPlot::setLegendPosition()
*/
void QwtPlotLayout::setLegendPosition(QwtPlot::LegendPosition pos)
{
    setLegendPosition(pos, 0.0);
}

/*!
  \return Position of the legend
  \sa setLegendPosition(), QwtPlot::setLegendPosition(),
      QwtPlot::legendPosition()
*/
QwtPlot::LegendPosition QwtPlotLayout::legendPosition() const
{
    return d_data->legendPos;
}

/*!
  Specify the relative size of the legend in the plot
  \param ratio Ratio between legend and the bounding rect
               of title, canvas and axes. The legend will be shrinked
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.
*/
void QwtPlotLayout::setLegendRatio(double ratio)
{
    setLegendPosition(legendPosition(), ratio);
}

/*!
  \return The relative size of the legend in the plot.
  \sa setLegendPosition()
*/
double QwtPlotLayout::legendRatio() const
{
    return d_data->legendRatio;
}

/*!
  \return Geometry for the title
  \sa activate(), invalidate()
*/

const QRect &QwtPlotLayout::titleRect() const
{
    return d_data->titleRect;
}

/*!
  \return Geometry for the legend
  \sa activate(), invalidate()
*/

const QRect &QwtPlotLayout::legendRect() const
{
    return d_data->legendRect;
}

/*!
  \param axis Axis index
  \return Geometry for the scale
  \sa activate(), invalidate()
*/

const QRect &QwtPlotLayout::scaleRect(int axis) const
{
    if ( axis < 0 || axis >= QwtPlot::axisCnt ) {
        static QRect dummyRect;
        return dummyRect;
    }
    return d_data->scaleRect[axis];
}

/*!
  \return Geometry for the canvas
  \sa activate(), invalidate()
*/

const QRect &QwtPlotLayout::canvasRect() const
{
    return d_data->canvasRect;
}

/*!
  Invalidate the geometry of all components.
  \sa activate()
*/
void QwtPlotLayout::invalidate()
{
    d_data->titleRect = d_data->legendRect = d_data->canvasRect = QRect();
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++ )
        d_data->scaleRect[axis] = QRect();
}

/*!
  \brief Return a minimum size hint
  \sa QwtPlot::minimumSizeHint()
*/

QSize QwtPlotLayout::minimumSizeHint(const QwtPlot *plot) const
{
    class ScaleData
    {
    public:
        ScaleData() {
            w = h = minLeft = minRight = tickOffset = 0;
        }

        int w;
        int h;
        int minLeft;
        int minRight;
        int tickOffset;
    } scaleData[QwtPlot::axisCnt];

    int canvasBorder[QwtPlot::axisCnt];

    int axis;
    for ( axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        if ( plot->axisEnabled(axis) ) {
            const QwtScaleWidget *scl = plot->axisWidget(axis);
            ScaleData &sd = scaleData[axis];

            const QSize hint = scl->minimumSizeHint();
            sd.w = hint.width();
            sd.h = hint.height();
            scl->getBorderDistHint(sd.minLeft, sd.minRight);
            sd.tickOffset = scl->margin();
            if ( scl->scaleDraw()->hasComponent(QwtAbstractScaleDraw::Ticks) )
                sd.tickOffset += scl->scaleDraw()->majTickLength();
        }

        canvasBorder[axis] = plot->canvas()->frameWidth() +
                             d_data->canvasMargin[axis] + 1;

    }


    for ( axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        ScaleData &sd = scaleData[axis];
        if ( sd.w && (axis == QwtPlot::xBottom || axis == QwtPlot::xTop) ) {
            if ( (sd.minLeft > canvasBorder[QwtPlot::yLeft])
                    && scaleData[QwtPlot::yLeft].w ) {
                int shiftLeft = sd.minLeft - canvasBorder[QwtPlot::yLeft];
                if ( shiftLeft > scaleData[QwtPlot::yLeft].w )
                    shiftLeft = scaleData[QwtPlot::yLeft].w;

                sd.w -= shiftLeft;
            }
            if ( (sd.minRight > canvasBorder[QwtPlot::yRight])
                    && scaleData[QwtPlot::yRight].w ) {
                int shiftRight = sd.minRight - canvasBorder[QwtPlot::yRight];
                if ( shiftRight > scaleData[QwtPlot::yRight].w )
                    shiftRight = scaleData[QwtPlot::yRight].w;

                sd.w -= shiftRight;
            }
        }

        if ( sd.h && (axis == QwtPlot::yLeft || axis == QwtPlot::yRight) ) {
            if ( (sd.minLeft > canvasBorder[QwtPlot::xBottom]) &&
                    scaleData[QwtPlot::xBottom].h ) {
                int shiftBottom = sd.minLeft - canvasBorder[QwtPlot::xBottom];
                if ( shiftBottom > scaleData[QwtPlot::xBottom].tickOffset )
                    shiftBottom = scaleData[QwtPlot::xBottom].tickOffset;

                sd.h -= shiftBottom;
            }
            if ( (sd.minLeft > canvasBorder[QwtPlot::xTop]) &&
                    scaleData[QwtPlot::xTop].h ) {
                int shiftTop = sd.minRight - canvasBorder[QwtPlot::xTop];
                if ( shiftTop > scaleData[QwtPlot::xTop].tickOffset )
                    shiftTop = scaleData[QwtPlot::xTop].tickOffset;

                sd.h -= shiftTop;
            }
        }
    }

    const QwtPlotCanvas *canvas = plot->canvas();
    const QSize minCanvasSize = canvas->minimumSize();

    int w = scaleData[QwtPlot::yLeft].w + scaleData[QwtPlot::yRight].w;
    int cw = qwtMax(scaleData[QwtPlot::xBottom].w, scaleData[QwtPlot::xTop].w)
             + 2 * (canvas->frameWidth() + 1);
    w += qwtMax(cw, minCanvasSize.width());

    int h = scaleData[QwtPlot::xBottom].h + scaleData[QwtPlot::xTop].h;
    int ch = qwtMax(scaleData[QwtPlot::yLeft].h, scaleData[QwtPlot::yRight].h)
             + 2 * (canvas->frameWidth() + 1);
    h += qwtMax(ch, minCanvasSize.height());

    const QwtTextLabel *title = plot->titleLabel();
    if (title && !title->text().isEmpty()) {
        // If only QwtPlot::yLeft or QwtPlot::yRight is showing,
        // we center on the plot canvas.
        const bool centerOnCanvas = !(plot->axisEnabled(QwtPlot::yLeft)
                                      && plot->axisEnabled(QwtPlot::yRight));

        int titleW = w;
        if ( centerOnCanvas ) {
            titleW -= scaleData[QwtPlot::yLeft].w
                      + scaleData[QwtPlot::yRight].w;
        }

        int titleH = title->heightForWidth(titleW);
        if ( titleH > titleW ) { // Compensate for a long title
            w = titleW = titleH;
            if ( centerOnCanvas ) {
                w += scaleData[QwtPlot::yLeft].w
                     + scaleData[QwtPlot::yRight].w;
            }

            titleH = title->heightForWidth(titleW);
        }
        h += titleH + d_data->spacing;
    }

    // Compute the legend contribution

    const QwtLegend *legend = plot->legend();
    if ( d_data->legendPos != QwtPlot::ExternalLegend
            && legend && !legend->isEmpty() ) {
        if ( d_data->legendPos == QwtPlot::LeftLegend
                || d_data->legendPos == QwtPlot::RightLegend ) {
            int legendW = legend->sizeHint().width();
            int legendH = legend->heightForWidth(legendW);

            if ( legend->frameWidth() > 0 )
                w += d_data->spacing;

            if ( legendH > h )
                legendW += legend->verticalScrollBar()->sizeHint().height();

            if ( d_data->legendRatio < 1.0 )
                legendW = qwtMin(legendW, int(w / (1.0 - d_data->legendRatio)));

            w += legendW;
        } else { // QwtPlot::Top, QwtPlot::Bottom
            int legendW = qwtMin(legend->sizeHint().width(), w);
            int legendH = legend->heightForWidth(legendW);

            if ( legend->frameWidth() > 0 )
                h += d_data->spacing;

            if ( d_data->legendRatio < 1.0 )
                legendH = qwtMin(legendH, int(h / (1.0 - d_data->legendRatio)));

            h += legendH;
        }
    }

    w += 2 * d_data->margin;
    h += 2 * d_data->margin;

    return QSize( w, h );
}

/*!
  Find the geometry for the legend
  \param options Options how to layout the legend
  \param rect Rectangle where to place the legend
  \return Geometry for the legend
*/

QRect QwtPlotLayout::layoutLegend(int options,
                                  const QRect &rect) const
{
    const QSize hint(d_data->layoutData.legend.hint);

    int dim;
    if ( d_data->legendPos == QwtPlot::LeftLegend
            || d_data->legendPos == QwtPlot::RightLegend ) {
        // We don't allow vertical legends to take more than
        // half of the available space.

        dim = qwtMin(hint.width(), int(rect.width() * d_data->legendRatio));

        if ( !(options & IgnoreScrollbars) ) {
            if ( hint.height() > rect.height() ) {
                // The legend will need additional
                // space for the vertical scrollbar.

                dim += d_data->layoutData.legend.vScrollBarWidth;
            }
        }
    } else {
        dim = qwtMin(hint.height(), int(rect.height() * d_data->legendRatio));
        dim = qwtMax(dim, d_data->layoutData.legend.hScrollBarHeight);
    }

    QRect legendRect = rect;
    switch(d_data->legendPos) {
    case QwtPlot::LeftLegend:
        legendRect.setWidth(dim);
        break;
    case QwtPlot::RightLegend:
        legendRect.setX(rect.right() - dim + 1);
        legendRect.setWidth(dim);
        break;
    case QwtPlot::TopLegend:
        legendRect.setHeight(dim);
        break;
    case QwtPlot::BottomLegend:
        legendRect.setY(rect.bottom() - dim + 1);
        legendRect.setHeight(dim);
        break;
    case QwtPlot::ExternalLegend:
        break;
    }

    return legendRect;
}

/*!
  Align the legend to the canvas
  \param canvasRect Geometry of the canvas
  \param legendRect Maximum geometry for the legend
  \return Geometry for the aligned legend
*/
QRect QwtPlotLayout::alignLegend(const QRect &canvasRect,
                                 const QRect &legendRect) const
{
    QRect alignedRect = legendRect;

    if ( d_data->legendPos == QwtPlot::BottomLegend
            || d_data->legendPos == QwtPlot::TopLegend ) {
        if ( d_data->layoutData.legend.hint.width() < canvasRect.width() ) {
            alignedRect.setX(canvasRect.x());
            alignedRect.setWidth(canvasRect.width());
        }
    } else {
        if ( d_data->layoutData.legend.hint.height() < canvasRect.height() ) {
            alignedRect.setY(canvasRect.y());
            alignedRect.setHeight(canvasRect.height());
        }
    }

    return alignedRect;
}

/*!
  Expand all line breaks in text labels, and calculate the height
  of their widgets in orientation of the text.

  \param options Options how to layout the legend
  \param rect Bounding rect for title, axes and canvas.
  \param dimTitle Expanded height of the title widget
  \param dimAxis Expanded heights of the axis in axis orientation.
*/
void QwtPlotLayout::expandLineBreaks(int options, const QRect &rect,
                                     int &dimTitle, int dimAxis[QwtPlot::axisCnt]) const
{
    dimTitle = 0;
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
        dimAxis[axis] = 0;

    int backboneOffset[QwtPlot::axisCnt];
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        backboneOffset[axis] = 0;
        if ( !d_data->alignCanvasToScales )
            backboneOffset[axis] += d_data->canvasMargin[axis];
        if ( !(options & IgnoreFrames) )
            backboneOffset[axis] += d_data->layoutData.canvas.frameWidth;
    }

    bool done = false;
    while (!done) {
        done = true;

        // the size for the 4 axis depend on each other. Expanding
        // the height of a horizontal axis will shrink the height
        // for the vertical axis, shrinking the height of a vertical
        // axis will result in a line break what will expand the
        // width and results in shrinking the width of a horizontal
        // axis what might result in a line break of a horizontal
        // axis ... . So we loop as long until no size changes.

        if ( !d_data->layoutData.title.text.isEmpty() ) {
            int w = rect.width();

            if ( d_data->layoutData.scale[QwtPlot::yLeft].isEnabled
                    != d_data->layoutData.scale[QwtPlot::yRight].isEnabled ) {
                // center to the canvas
                w -= dimAxis[QwtPlot::yLeft] + dimAxis[QwtPlot::yRight];
            }

            int d = d_data->layoutData.title.text.heightForWidth(w);
            if ( !(options & IgnoreFrames) )
                d += 2 * d_data->layoutData.title.frameWidth;

            if ( d > dimTitle ) {
                dimTitle = d;
                done = false;
            }
        }

        for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
            const struct LayoutData::t_scaleData &scaleData =
                        d_data->layoutData.scale[axis];

            if (scaleData.isEnabled) {
                int length;
                if ( axis == QwtPlot::xTop || axis == QwtPlot::xBottom ) {
                    length = rect.width() - dimAxis[QwtPlot::yLeft]
                             - dimAxis[QwtPlot::yRight];
                    length -= scaleData.start + scaleData.end;

                    if ( dimAxis[QwtPlot::yRight] > 0 )
                        length -= 1;

                    length += qwtMin(dimAxis[QwtPlot::yLeft],
                                     scaleData.start - backboneOffset[QwtPlot::yLeft]);
                    length += qwtMin(dimAxis[QwtPlot::yRight],
                                     scaleData.end - backboneOffset[QwtPlot::yRight]);
                } else { // QwtPlot::yLeft, QwtPlot::yRight
                    length = rect.height() - dimAxis[QwtPlot::xTop]
                             - dimAxis[QwtPlot::xBottom];
                    length -= scaleData.start + scaleData.end;
                    length -= 1;

                    if ( dimAxis[QwtPlot::xBottom] <= 0 )
                        length -= 1;
                    if ( dimAxis[QwtPlot::xTop] <= 0 )
                        length -= 1;

                    if ( dimAxis[QwtPlot::xBottom] > 0 ) {
                        length += qwtMin(
                                      d_data->layoutData.scale[QwtPlot::xBottom].tickOffset,
                                      scaleData.start - backboneOffset[QwtPlot::xBottom]);
                    }
                    if ( dimAxis[QwtPlot::xTop] > 0 ) {
                        length += qwtMin(
                                      d_data->layoutData.scale[QwtPlot::xTop].tickOffset,
                                      scaleData.end - backboneOffset[QwtPlot::xTop]);
                    }

                    if ( dimTitle > 0 )
                        length -= dimTitle + d_data->spacing;
                }

                int d = scaleData.dimWithoutTitle;
                if ( !scaleData.scaleWidget->title().isEmpty() ) {
                    d += scaleData.scaleWidget->titleHeightForWidth(length);
                }


                if ( d > dimAxis[axis] ) {
                    dimAxis[axis] = d;
                    done = false;
                }
            }
        }
    }
}

/*!
  Align the ticks of the axis to the canvas borders using
  the empty corners.
*/

void QwtPlotLayout::alignScales(int options,
                                QRect &canvasRect, QRect scaleRect[QwtPlot::axisCnt]) const
{
    int axis;

    int backboneOffset[QwtPlot::axisCnt];
    for (axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        backboneOffset[axis] = 0;
        if ( !d_data->alignCanvasToScales )
            backboneOffset[axis] += d_data->canvasMargin[axis];
        if ( !(options & IgnoreFrames) )
            backboneOffset[axis] += d_data->layoutData.canvas.frameWidth;
    }

    for (axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        if ( !scaleRect[axis].isValid() )
            continue;

        const int startDist = d_data->layoutData.scale[axis].start;
        const int endDist = d_data->layoutData.scale[axis].end;

        QRect &axisRect = scaleRect[axis];

        if ( axis == QwtPlot::xTop || axis == QwtPlot::xBottom ) {
            const int leftOffset =
                backboneOffset[QwtPlot::yLeft] - startDist;

            if ( scaleRect[QwtPlot::yLeft].isValid() ) {
                int minLeft = scaleRect[QwtPlot::yLeft].left();
                int left = axisRect.left() + leftOffset;
                axisRect.setLeft(qwtMax(left, minLeft));
            } else {
                if ( d_data->alignCanvasToScales && leftOffset < 0 ) {
                    canvasRect.setLeft(qwtMax(canvasRect.left(),
                                              axisRect.left() - leftOffset));
                } else {
                    if ( leftOffset > 0 )
                        axisRect.setLeft(axisRect.left() + leftOffset);
                }
            }

            const int rightOffset =
                backboneOffset[QwtPlot::yRight] - endDist + 1;

            if ( scaleRect[QwtPlot::yRight].isValid() ) {
                int maxRight = scaleRect[QwtPlot::yRight].right();
                int right = axisRect.right() - rightOffset;
                axisRect.setRight(qwtMin(right, maxRight));
            } else {
                if ( d_data->alignCanvasToScales && rightOffset < 0 ) {
                    canvasRect.setRight( qwtMin(canvasRect.right(),
                                                axisRect.right() + rightOffset) );
                } else {
                    if ( rightOffset > 0 )
                        axisRect.setRight(axisRect.right() - rightOffset);
                }
            }
        } else { // QwtPlot::yLeft, QwtPlot::yRight
            const int bottomOffset =
                backboneOffset[QwtPlot::xBottom] - endDist + 1;

            if ( scaleRect[QwtPlot::xBottom].isValid() ) {
                int maxBottom = scaleRect[QwtPlot::xBottom].top() +
                                d_data->layoutData.scale[QwtPlot::xBottom].tickOffset;

                int bottom = axisRect.bottom() - bottomOffset;
                axisRect.setBottom(qwtMin(bottom, maxBottom));
            } else {
                if ( d_data->alignCanvasToScales && bottomOffset < 0 ) {
                    canvasRect.setBottom(qwtMin(canvasRect.bottom(),
                                                axisRect.bottom() + bottomOffset));
                } else {
                    if ( bottomOffset > 0 )
                        axisRect.setBottom(axisRect.bottom() - bottomOffset);
                }
            }

            const int topOffset = backboneOffset[QwtPlot::xTop] - startDist;

            if ( scaleRect[QwtPlot::xTop].isValid() ) {
                int minTop = scaleRect[QwtPlot::xTop].bottom() -
                             d_data->layoutData.scale[QwtPlot::xTop].tickOffset;

                int top = axisRect.top() + topOffset;
                axisRect.setTop(qwtMax(top, minTop));
            } else {
                if ( d_data->alignCanvasToScales && topOffset < 0 ) {
                    canvasRect.setTop(qwtMax(canvasRect.top(),
                                             axisRect.top() - topOffset));
                } else {
                    if ( topOffset > 0 )
                        axisRect.setTop(axisRect.top() + topOffset);
                }
            }
        }
    }

    if ( d_data->alignCanvasToScales ) {
        /*
          The canvas has been aligned to the scale with largest
          border distances. Now we have to realign the other scale.
         */

        int fw = 0;
        if ( !(options & IgnoreFrames) )
            fw = d_data->layoutData.canvas.frameWidth;

        if ( scaleRect[QwtPlot::xBottom].isValid() &&
                scaleRect[QwtPlot::xTop].isValid() ) {
            for ( int axis = QwtPlot::xBottom; axis <= QwtPlot::xTop; axis++ ) {
                scaleRect[axis].setLeft(canvasRect.left() + fw
                                        - d_data->layoutData.scale[axis].start);
                scaleRect[axis].setRight(canvasRect.right() - fw - 1
                                         + d_data->layoutData.scale[axis].end);
            }
        }

        if ( scaleRect[QwtPlot::yLeft].isValid() &&
                scaleRect[QwtPlot::yRight].isValid() ) {
            for ( int axis = QwtPlot::yLeft; axis <= QwtPlot::yRight; axis++ ) {
                scaleRect[axis].setTop(canvasRect.top() + fw
                                       - d_data->layoutData.scale[axis].start);
                scaleRect[axis].setBottom(canvasRect.bottom() - fw - 1
                                          + d_data->layoutData.scale[axis].end);
            }
        }
    }
}

/*!
  \brief Recalculate the geometry of all components.

  \param plot Plot to be layout
  \param plotRect Rect where to place the components
  \param options Options

  \sa invalidate(), titleRect(),
      legendRect(), scaleRect(), canvasRect()
*/
void QwtPlotLayout::activate(const QwtPlot *plot,
                             const QRect &plotRect, int options)
{
    invalidate();

    QRect rect(plotRect);  // undistributed rest of the plot rect

    if ( !(options & IgnoreMargin) ) {
        // subtract the margin

        rect.setRect(
            rect.x() + d_data->margin,
            rect.y() + d_data->margin,
            rect.width() - 2 * d_data->margin,
            rect.height() - 2 * d_data->margin
        );
    }

    // We extract all layout relevant data from the widgets,
    // filter them through pfilter and save them to d_data->layoutData.

    d_data->layoutData.init(plot, rect);

    if (!(options & IgnoreLegend)
            && d_data->legendPos != QwtPlot::ExternalLegend
            && plot->legend() && !plot->legend()->isEmpty() ) {
        d_data->legendRect = layoutLegend(options, rect);

        // subtract d_data->legendRect from rect

        const QRegion region(rect);
        rect = region.subtract(d_data->legendRect).boundingRect();

        if ( d_data->layoutData.legend.frameWidth &&
                !(options & IgnoreFrames ) ) {
            // In case of a frame we have to insert a spacing.
            // Otherwise the leading of the font separates
            // legend and scale/canvas

            switch(d_data->legendPos) {
            case QwtPlot::LeftLegend:
                rect.setLeft(rect.left() + d_data->spacing);
                break;
            case QwtPlot::RightLegend:
                rect.setRight(rect.right() - d_data->spacing);
                break;
            case QwtPlot::TopLegend:
                rect.setTop(rect.top() + d_data->spacing);
                break;
            case QwtPlot::BottomLegend:
                rect.setBottom(rect.bottom() - d_data->spacing);
                break;
            case QwtPlot::ExternalLegend:
                break; // suppress compiler warning
            }
        }
    }

#ifdef __GNUC__
#endif
    /*
     +---+-----------+---+
     |       Title       |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
     | A |           | A |
     | x |  Canvas   | x |
     | i |           | i |
     | s |           | s |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
    */

    // axes and title include text labels. The height of each
    // label depends on its line breaks, that depend on the width
    // for the label. A line break in a horizontal text will reduce
    // the available width for vertical texts and vice versa.
    // expandLineBreaks finds the height/width for title and axes
    // including all line breaks.

    int dimTitle, dimAxes[QwtPlot::axisCnt];
    expandLineBreaks(options, rect, dimTitle, dimAxes);

    if (dimTitle > 0 ) {
        d_data->titleRect = QRect(rect.x(), rect.y(),
                                  rect.width(), dimTitle);

        if ( d_data->layoutData.scale[QwtPlot::yLeft].isEnabled !=
                d_data->layoutData.scale[QwtPlot::yRight].isEnabled ) {
            // if only one of the y axes is missing we align
            // the title centered to the canvas

            d_data->titleRect.setX(rect.x() + dimAxes[QwtPlot::yLeft]);
            d_data->titleRect.setWidth(rect.width()
                                       - dimAxes[QwtPlot::yLeft] - dimAxes[QwtPlot::yRight]);
        }

        // subtract title
        rect.setTop(rect.top() + dimTitle + d_data->spacing);
    }

    d_data->canvasRect.setRect(
        rect.x() + dimAxes[QwtPlot::yLeft],
        rect.y() + dimAxes[QwtPlot::xTop],
        rect.width() - dimAxes[QwtPlot::yRight] - dimAxes[QwtPlot::yLeft],
        rect.height() - dimAxes[QwtPlot::xBottom] - dimAxes[QwtPlot::xTop]);

    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
        // set the rects for the axes

        if ( dimAxes[axis] ) {
            int dim = dimAxes[axis];
            QRect &scaleRect = d_data->scaleRect[axis];

            scaleRect = d_data->canvasRect;
            switch(axis) {
            case QwtPlot::yLeft:
                scaleRect.setX(d_data->canvasRect.left() - dim);
                scaleRect.setWidth(dim);
                break;
            case QwtPlot::yRight:
                scaleRect.setX(d_data->canvasRect.right() + 1);
                scaleRect.setWidth(dim);
                break;
            case QwtPlot::xBottom:
                scaleRect.setY(d_data->canvasRect.bottom() + 1);
                scaleRect.setHeight(dim);
                break;
            case QwtPlot::xTop:
                scaleRect.setY(d_data->canvasRect.top() - dim);
                scaleRect.setHeight(dim);
                break;
            }
#if QT_VERSION < 0x040000
            scaleRect = scaleRect.normalize();
#else
            scaleRect = scaleRect.normalized();
#endif
        }
    }

    // +---+-----------+---+
    // |  <-   Axis   ->   |
    // +-^-+-----------+-^-+
    // | | |           | | |
    // |   |           |   |
    // | A |           | A |
    // | x |  Canvas   | x |
    // | i |           | i |
    // | s |           | s |
    // |   |           |   |
    // | | |           | | |
    // +-V-+-----------+-V-+
    // |   <-  Axis   ->   |
    // +---+-----------+---+

    // The ticks of the axes - not the labels above - should
    // be aligned to the canvas. So we try to use the empty
    // corners to extend the axes, so that the label texts
    // left/right of the min/max ticks are moved into them.

    alignScales(options, d_data->canvasRect, d_data->scaleRect);

    if (!d_data->legendRect.isEmpty() ) {
        // We prefer to align the legend to the canvas - not to
        // the complete plot - if possible.

        d_data->legendRect = alignLegend(d_data->canvasRect, d_data->legendRect);
    }
}
