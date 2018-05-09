#include <qevent.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <Scrollbar.h>
#include <ScrollZoomer.h>

class ScrollData
{
public:
    ScrollData():
        scrollBar(NULL),
        position(ScrollZoomer::OppositeToScale),
#if QT_VERSION < 0x040000
        mode(QScrollView::Auto)
#else
        mode(Qt::ScrollBarAsNeeded)
#endif
    {
    }

    ~ScrollData() {
        delete scrollBar;
    }

    ScrollBar *scrollBar;
    ScrollZoomer::ScrollBarPosition position;
#if QT_VERSION < 0x040000
    QScrollView::ScrollBarMode mode;
#else
    Qt::ScrollBarPolicy mode;
#endif
};

ScrollZoomer::ScrollZoomer(QwtPlotCanvas *canvas):
    QwtPlotZoomer(canvas),
    d_cornerWidget(NULL),
    d_hScrollData(NULL),
    d_vScrollData(NULL),
    d_inZoom(false)
{
    if ( !canvas )
        return;

    d_hScrollData = new ScrollData;
    d_vScrollData = new ScrollData;
}

ScrollZoomer::~ScrollZoomer()
{
    delete d_cornerWidget;
    delete d_vScrollData;
    delete d_hScrollData;
}

void ScrollZoomer::rescale()
{
    QwtScaleWidget *xScale = plot()->axisWidget(xAxis());
    QwtScaleWidget *yScale = plot()->axisWidget(yAxis());

    if ( zoomRectIndex() <= 0 ) {
        if ( d_inZoom ) {
            xScale->setMinBorderDist(0, 0);
            yScale->setMinBorderDist(0, 0);
            d_inZoom = false;
        }
    } else {
        if ( !d_inZoom ) {
            /*
             We set a minimum border distance.
             Otherwise the canvas size changes when scrolling,
             between situations where the major ticks are at
             the canvas borders (requiring extra space for the label)
             and situations where all labels can be painted below/top
             or left/right of the canvas.
             */
            int start, end;

            xScale->getBorderDistHint(start, end);
            xScale->setMinBorderDist(start, end);

            yScale->getBorderDistHint(start, end);
            yScale->setMinBorderDist(start, end);

            d_inZoom = false;
        }
    }

    QwtPlotZoomer::rescale();
    updateScrollBars();
}

ScrollBar *ScrollZoomer::scrollBar(Qt::Orientation o)
{
    ScrollBar *&sb = (o == Qt::Vertical)
                     ? d_vScrollData->scrollBar : d_hScrollData->scrollBar;

    if ( sb == NULL ) {
        sb = new ScrollBar(o, canvas());
        sb->hide();
        connect(sb, &ScrollBar::valueChanged, this, &ScrollZoomer::scrollBarMoved);
    }
    return sb;
}

ScrollBar *ScrollZoomer::horizontalScrollBar() const
{
    return d_hScrollData->scrollBar;
}

ScrollBar *ScrollZoomer::verticalScrollBar() const
{
    return d_vScrollData->scrollBar;
}

void ScrollZoomer::setHScrollBarMode(Qt::ScrollBarPolicy mode)
{
    if ( hScrollBarMode() != mode ) {
        d_hScrollData->mode = mode;
        updateScrollBars();
    }
}

void ScrollZoomer::setVScrollBarMode(Qt::ScrollBarPolicy mode)
{
    if ( vScrollBarMode() != mode ) {
        d_vScrollData->mode = mode;
        updateScrollBars();
    }
}

Qt::ScrollBarPolicy ScrollZoomer::hScrollBarMode() const
{
    return d_hScrollData->mode;
}

Qt::ScrollBarPolicy ScrollZoomer::vScrollBarMode() const
{
    return d_vScrollData->mode;
}

void ScrollZoomer::setHScrollBarPosition(ScrollBarPosition pos)
{
    if ( d_hScrollData->position != pos ) {
        d_hScrollData->position = pos;
        updateScrollBars();
    }
}

void ScrollZoomer::setVScrollBarPosition(ScrollBarPosition pos)
{
    if ( d_vScrollData->position != pos ) {
        d_vScrollData->position = pos;
        updateScrollBars();
    }
}

ScrollZoomer::ScrollBarPosition ScrollZoomer::hScrollBarPosition() const
{
    return d_hScrollData->position;
}

ScrollZoomer::ScrollBarPosition ScrollZoomer::vScrollBarPosition() const
{
    return d_vScrollData->position;
}

void ScrollZoomer::setCornerWidget(QWidget *w)
{
    if ( w != d_cornerWidget ) {
        if ( canvas() ) {
            delete d_cornerWidget;
            d_cornerWidget = w;
            if ( d_cornerWidget->parent() != canvas() ) {
                d_cornerWidget->setParent(canvas());
            }

            updateScrollBars();
        }
    }
}

QWidget *ScrollZoomer::cornerWidget() const
{
    return d_cornerWidget;
}

bool ScrollZoomer::eventFilter(QObject *o, QEvent *e)
{
    if (  o == canvas() ) {
        switch(e->type()) {
        case QEvent::Resize: {
            const int fw = ((QwtPlotCanvas *)canvas())->frameWidth();

            QRect rect;
            rect.setSize(((QResizeEvent *)e)->size());
            rect.setRect(rect.x() + fw, rect.y() + fw,
                         rect.width() - 2 * fw, rect.height() - 2 * fw);

            layoutScrollBars(rect);
            break;
        }
        case QEvent::ChildRemoved: {
            const QObject *child = ((QChildEvent *)e)->child();
            if ( child == d_cornerWidget )
                d_cornerWidget = NULL;
            else if ( child == d_hScrollData->scrollBar )
                d_hScrollData->scrollBar = NULL;
            else if ( child == d_vScrollData->scrollBar )
                d_vScrollData->scrollBar = NULL;
            break;
        }
        default:
            break;
        }
    }
    return QwtPlotZoomer::eventFilter(o, e);
}

bool ScrollZoomer::needScrollBar(Qt::Orientation o) const
{
    Qt::ScrollBarPolicy mode;
    double zoomMin, zoomMax, baseMin, baseMax;

    if ( o == Qt::Horizontal ) {
        mode = d_hScrollData->mode;
        baseMin = zoomBase().left();
        baseMax = zoomBase().right();
        zoomMin = zoomRect().left();
        zoomMax = zoomRect().right();
    } else {
        mode = d_vScrollData->mode;
        baseMin = zoomBase().top();
        baseMax = zoomBase().bottom();
        zoomMin = zoomRect().top();
        zoomMax = zoomRect().bottom();
    }

    bool needed = false;
    switch(mode) {
    case Qt::ScrollBarAlwaysOn:
        needed = true;
        break;
    case Qt::ScrollBarAlwaysOff:
        needed = false;
        break;
    default: {
        if ( baseMin < zoomMin || baseMax > zoomMax )
            needed = true;
        break;
    }
    }
    return needed;
}

void ScrollZoomer::updateScrollBars()
{
    if ( !canvas() )
        return;

    const int xAxis = QwtPlotZoomer::xAxis();
    const int yAxis = QwtPlotZoomer::yAxis();

    int xScrollBarAxis = xAxis;
    if ( hScrollBarPosition() == OppositeToScale )
        xScrollBarAxis = oppositeAxis(xScrollBarAxis);

    int yScrollBarAxis = yAxis;
    if ( vScrollBarPosition() == OppositeToScale )
        yScrollBarAxis = oppositeAxis(yScrollBarAxis);


    QwtPlotLayout *layout = plot()->plotLayout();

    bool showHScrollBar = needScrollBar(Qt::Horizontal);
    if ( showHScrollBar ) {
        ScrollBar *sb = scrollBar(Qt::Horizontal);

        sb->setPalette(plot()->palette());

        const QwtScaleEngine *se = plot()->axisScaleEngine(xAxis);
        sb->setInverted(se->testAttribute(QwtScaleEngine::Inverted));

        sb->setBase(zoomBase().left(), zoomBase().right());
        sb->moveSlider(zoomRect().left(), zoomRect().right());

        if ( !sb->isVisibleTo(canvas()) ) {
            sb->show();
            layout->setCanvasMargin(layout->canvasMargin(xScrollBarAxis)
                                    + sb->extent(), xScrollBarAxis);
        }
    } else {
        if ( horizontalScrollBar() ) {
            horizontalScrollBar()->hide();
            layout->setCanvasMargin(layout->canvasMargin(xScrollBarAxis)
                                    - horizontalScrollBar()->extent(), xScrollBarAxis);
        }
    }

    bool showVScrollBar = needScrollBar(Qt::Vertical);
    if ( showVScrollBar ) {
        ScrollBar *sb = scrollBar(Qt::Vertical);

        sb->setPalette(plot()->palette());

        const QwtScaleEngine *se = plot()->axisScaleEngine(xAxis);
        sb->setInverted(!(se->testAttribute(QwtScaleEngine::Inverted)));

        sb->setBase(zoomBase().top(), zoomBase().bottom());
        sb->moveSlider(zoomRect().top(), zoomRect().bottom());

        if ( !sb->isVisibleTo(canvas()) ) {
            sb->show();
            layout->setCanvasMargin(layout->canvasMargin(yScrollBarAxis)
                                    + sb->extent(), yScrollBarAxis);
        }
    } else {
        if ( verticalScrollBar() ) {
            verticalScrollBar()->hide();
            layout->setCanvasMargin(layout->canvasMargin(yScrollBarAxis)
                                    - verticalScrollBar()->extent(), yScrollBarAxis);
        }
    }

    if ( showHScrollBar && showVScrollBar ) {
        if ( d_cornerWidget == NULL ) {
            d_cornerWidget = new QWidget(canvas());
            d_cornerWidget->setPalette(plot()->palette());
        }
        d_cornerWidget->show();
    } else {
        if ( d_cornerWidget )
            d_cornerWidget->hide();
    }

    layoutScrollBars(((QwtPlotCanvas *)canvas())->contentsRect());
    plot()->updateLayout();
}

void ScrollZoomer::layoutScrollBars(const QRect &rect)
{
    int hPos = xAxis();
    if ( hScrollBarPosition() == OppositeToScale )
        hPos = oppositeAxis(hPos);

    int vPos = yAxis();
    if ( vScrollBarPosition() == OppositeToScale )
        vPos = oppositeAxis(vPos);

    ScrollBar *hScrollBar = horizontalScrollBar();
    ScrollBar *vScrollBar = verticalScrollBar();

    const int hdim = hScrollBar ? hScrollBar->extent() : 0;
    const int vdim = vScrollBar ? vScrollBar->extent() : 0;

    if ( hScrollBar && hScrollBar->isVisible() ) {
        int x = rect.x();
        int y = (hPos == QwtPlot::xTop)
                ? rect.top() : rect.bottom() - hdim + 1;
        int w = rect.width();

        if ( vScrollBar && vScrollBar->isVisible() ) {
            if ( vPos == QwtPlot::yLeft )
                x += vdim;
            w -= vdim;
        }

        hScrollBar->setGeometry(x, y, w, hdim);
    }
    if ( vScrollBar && vScrollBar->isVisible() ) {
        //-- TODO: What is this "pos"? It only gets
        //   assigned but never used within this
        //   scope.
        int pos = yAxis();
        if ( vScrollBarPosition() == OppositeToScale )
            pos = oppositeAxis(pos);

        int x = (vPos == QwtPlot::yLeft)
                ? rect.left() : rect.right() - vdim + 1;
        int y = rect.y();

        int h = rect.height();

        if ( hScrollBar && hScrollBar->isVisible() ) {
            if ( hPos == QwtPlot::xTop )
                y += hdim;

            h -= hdim;
        }

        vScrollBar->setGeometry(x, y, vdim, h);
    }
    if ( hScrollBar && hScrollBar->isVisible() &&
            vScrollBar && vScrollBar->isVisible() ) {
        if ( d_cornerWidget ) {
            QRect cornerRect(
                vScrollBar->pos().x(), hScrollBar->pos().y(),
                vdim, hdim);
            d_cornerWidget->setGeometry(cornerRect);
        }
    }
}

void ScrollZoomer::scrollBarMoved(Qt::Orientation o, double min, double)
{
    if ( o == Qt::Horizontal )
        move(QPoint(min, zoomRect().top()));
    else
        move(QPoint(zoomRect().left(), min));

    emit zoomed(zoomRect());
}

int ScrollZoomer::oppositeAxis(int axis) const
{
    switch(axis) {
    case QwtPlot::xBottom:
        return QwtPlot::xTop;
    case QwtPlot::xTop:
        return QwtPlot::xBottom;
    case QwtPlot::yLeft:
        return QwtPlot::yRight;
    case QwtPlot::yRight:
        return QwtPlot::yLeft;
    default:
        break;
    }

    return axis;
}
