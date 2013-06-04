#include "ChartPlot.h"
#include "MainWindow.h"

const int ChartPlot::numBaseColors = 20;

ChartPlot::ChartPlot(QWidget *parent):
    QwtPlot(parent)/*,
    symbolWidth(1.2f),
    curveWidth(1.0f),
    gridWidth(0.8f),
    scaleWidth(1.0f)*/
{
//    for (int i = 0; i < numBaseColors; ++i)
//    {
//        colors[i] = baseColors[i];
//    }
    // Set up the colorscheme used by this plotter.
//    nextColor = 0;

    // Set the grid. The colorscheme was already set in generateColorScheme().
    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->attach(this);

//    // Enable zooming
    zoomer = new ScrollZoomer(canvas());

    colors = QList<QColor>();
    nextColor = 0;

    ///> Color map for plots, includes 20 colors
    ///> Map will start from beginning when the first 20 colors are exceeded
    colors.append(QColor(242,255,128));
    colors.append(QColor(70,80,242));
    colors.append(QColor(232,33,47));
    colors.append(QColor(116,251,110));
    colors.append(QColor(81,183,244));
    colors.append(QColor(234,38,107));
    colors.append(QColor(92,247,217));
    colors.append(QColor(151,59,239));
    colors.append(QColor(231,72,28));
    colors.append(QColor(236,48,221));
    colors.append(QColor(75,133,243));
    colors.append(QColor(203,254,121));
    colors.append(QColor(104,64,240));
    colors.append(QColor(200,54,238));
    colors.append(QColor(104,250,138));
    colors.append(QColor(235,43,165));
    colors.append(QColor(98,248,176));
    colors.append(QColor(161,252,116));
    colors.append(QColor(87,231,246));
    colors.append(QColor(230,126,23));

    // Now that all objects have been initialized, color everything.
    applyColorScheme(((MainWindow*)parent)->getStyle());

    // And make sure we're listening for future style changes
    connect(parent, SIGNAL(styleChanged(MainWindow::QGC_MAINWINDOW_STYLE)),
            this, SLOT(applyColorScheme(MainWindow::QGC_MAINWINDOW_STYLE)));
}

ChartPlot::~ChartPlot()
{

}

QColor ChartPlot::getNextColor()
{
    /* Return current color and increment counter for next round */
    nextColor++;
    if(nextColor >= colors.count()) nextColor = 0;
    return colors[nextColor++];
}

QColor ChartPlot::getColorForCurve(QString id)
{
    return curves.value(id)->pen().color();
}

void ChartPlot::shuffleColors()
{
    foreach (QwtPlotCurve* curve, curves)
    {
        QPen pen(curve->pen());
        pen.setColor(getNextColor());
        curve->setPen(pen);
    }
}

void ChartPlot::applyColorScheme(MainWindow::QGC_MAINWINDOW_STYLE style)
{
    // Generate the color list for curves
    for (int i = 0; i < numBaseColors; ++i)
    {
        if (style == MainWindow::QGC_MAINWINDOW_STYLE_LIGHT) {
//            colors[i] = baseColors[i].lighter(150);
        }
        else
        {
//            colors[i] = baseColors[i].darker(150);
        }
    }

    // Configure the rest of the UI colors based on the current theme.
    if (style == MainWindow::QGC_MAINWINDOW_STYLE_LIGHT)
    {
        // Set the coloring of the area selector for zooming.
        zoomer->setRubberBandPen(QPen(Qt::darkRed, 3.0f, Qt::DotLine));
        zoomer->setTrackerPen(QPen(Qt::darkRed));

        // Set canvas background
        setCanvasBackground(QColor(0xFF, 0xFF, 0xFF));

        // Configure the plot grid.
        grid->setMinPen(QPen(QColor(0x55, 0x55, 0x55), 0.8f, Qt::DotLine));
        grid->setMajPen(QPen(QColor(0x22, 0x22, 0x22), 0.8f, Qt::DotLine));
    }
    else
    {
        // Set the coloring of the area selector for zooming.
        zoomer->setRubberBandPen(QPen(Qt::red, 3.0f, Qt::DotLine));
        zoomer->setTrackerPen(QPen(Qt::red));

        // Set canvas background
        setCanvasBackground(QColor(0, 0, 0));

        // Configure the plot grid.
        grid->setMinPen(QPen(QColor(0xAA, 0xAA, 0xAA), 0.8f, Qt::DotLine));
        grid->setMajPen(QPen(QColor(0xDD, 0xDD, 0xDD), 0.8f, Qt::DotLine));
    }

    // And finally refresh the widget to make sure all color changes are redrawn.
    replot();
}
