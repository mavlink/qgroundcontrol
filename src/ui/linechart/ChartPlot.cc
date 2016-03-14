#include "ChartPlot.h"
#include "QGCApplication.h"

const QColor ChartPlot::baseColors[numColors] = {
    QColor(242,255,128),
    QColor(70,80,242),
    QColor(232,33,47),
    QColor(116,251,110),
    QColor(81,183,244),
    QColor(234,38,107),
    QColor(92,247,217),
    QColor(151,59,239),
    QColor(231,72,28),
    QColor(236,48,221),
    QColor(75,133,243),
    QColor(203,254,121),
    QColor(104,64,240),
    QColor(200,54,238),
    QColor(104,250,138),
    QColor(235,43,165),
    QColor(98,248,176),
    QColor(161,252,116),
    QColor(87,231,246),
    QColor(230,126,23)
};

ChartPlot::ChartPlot(QWidget *parent):
    QwtPlot(parent),
    nextColorIndex(0),
    symbolWidth(2.0f),
    curveWidth(2.0f),
    gridWidth(0.8f)
{
    // Initialize the list of curves.
    curves = QMap<QString, QwtPlotCurve*>();

    // Set the grid. The colorscheme was already set in generateColorScheme().
    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->attach(this);

    colors = QList<QColor>();

    ///> Color map for plots, includes 20 colors
    ///> Map will start from beginning when the first 20 colors are exceeded
    for (int i = 0; i < numColors; ++i)
    {
        colors.append(baseColors[i]);
    }

    // Now that all objects have been initialized, color everything.
    styleChanged(qgcApp()->styleIsDark());
}

ChartPlot::~ChartPlot()
{

}

QColor ChartPlot::getNextColor()
{
    if(nextColorIndex >= colors.count())
    {
        nextColorIndex = 0;
    }
    return colors[nextColorIndex++];
}

QColor ChartPlot::getColorForCurve(const QString &id)
{
    return curves.value(id)->pen().color();
}

void ChartPlot::shuffleColors()
{
    foreach (QwtPlotCurve* curve, curves)
    {
        if (curve->isVisible()) {
            QPen pen(curve->pen());
            pen.setColor(getNextColor());
            curve->setPen(pen);
        }
    }
}

void ChartPlot::styleChanged(bool styleIsDark)
{
    // Generate a new color list for curves and recolor them.
    for (int i = 0; i < numColors; ++i)
    {
        colors[i] = styleIsDark ? baseColors[i].lighter(150) : baseColors[i].darker(150);
    }
    shuffleColors();

    // Configure the rest of the UI colors based on the current theme.
    if (styleIsDark)
    {
        // Set canvas background
        setCanvasBackground(QColor(0, 0, 0));
        
        // Configure the plot grid.
        grid->setMinorPen(QPen(QColor(0xAA, 0xAA, 0xAA), gridWidth, Qt::DotLine));
        grid->setMajorPen(QPen(QColor(0xDD, 0xDD, 0xDD), gridWidth, Qt::DotLine));
    }
    else
    {
        // Set canvas background
        setCanvasBackground(QColor(0xFF, 0xFF, 0xFF));
        
        // Configure the plot grid.
        grid->setMinorPen(QPen(QColor(0x55, 0x55, 0x55), gridWidth, Qt::DotLine));
        grid->setMajorPen(QPen(QColor(0x22, 0x22, 0x22), gridWidth, Qt::DotLine));
    }

    // And finally refresh the widget to make sure all color changes are redrawn.
    replot();
}
