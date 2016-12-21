#include "ChartPlot.h"
#include "QGCApplication.h"

const QColor ChartPlot::baseColors[numColors] = {
    QColor(242, 255, 128),
    QColor(70, 80, 242),
    QColor(232, 33, 47),
    QColor(116, 251, 110),
    QColor(81, 183, 244),
    QColor(234, 38, 107),
    QColor(92, 247, 217),
    QColor(151, 59, 239),
    QColor(231, 72, 28),
    QColor(236, 48, 221),
    QColor(75, 133, 243),
    QColor(203, 254, 121),
    QColor(104, 64, 240),
    QColor(200, 54, 238),
    QColor(104, 250, 138),
    QColor(235, 43, 165),
    QColor(98, 248, 176),
    QColor(161, 252, 116),
    QColor(87, 231, 246),
    QColor(230, 126, 23)
};

ChartPlot::ChartPlot(QWidget* parent):
    QwtPlot(parent),
    _nextColorIndex(0),
    _symbolWidth(2.0f),
    _curveWidth(2.0f),
    _gridWidth(0.8f)
{
    // Initialize the list of curves.
    _curves = QMap<QString, QwtPlotCurve*>();
    // Set the grid. The colorscheme was already set in generateColorScheme().
    _grid = new QwtPlotGrid;
    _grid->enableXMin(true);
    _grid->attach(this);
    _colors = QList<QColor>();
    ///> Color map for plots, includes 20 colors
    ///> Map will start from beginning when the first 20 colors are exceeded
    for(int i = 0; i < numColors; ++i) {
        _colors.append(baseColors[i]);
    }
    // Now that all objects have been initialized, color everything.
    styleChanged(qgcApp()->styleIsDark());
}

ChartPlot::~ChartPlot()
{
}

QColor ChartPlot::getNextColor()
{
    if(_nextColorIndex >= _colors.count()) {
        _nextColorIndex = 0;
    }
    return _colors[_nextColorIndex++];
}

QColor ChartPlot::getColorForCurve(const QString& id)
{
    return _curves.value(id)->pen().color();
}

void ChartPlot::shuffleColors()
{
    foreach(QwtPlotCurve* curve, _curves) {
        if(curve->isVisible()) {
            QPen pen(curve->pen());
            pen.setColor(getNextColor());
            curve->setPen(pen);
        }
    }
}

void ChartPlot::styleChanged(bool styleIsDark)
{
    // Generate a new color list for curves and recolor them.
    for(int i = 0; i < numColors; ++i) {
        _colors[i] = styleIsDark ? baseColors[i].lighter(150) : baseColors[i].darker(150);
    }
    shuffleColors();
    // Configure the rest of the UI colors based on the current theme.
    if(styleIsDark) {
        // Set canvas background
        setCanvasBackground(QColor(0, 0, 0));
        // Configure the plot grid.
        _grid->setMinorPen(QPen(QColor(64, 64, 64), _gridWidth, Qt::SolidLine));
        _grid->setMajorPen(QPen(QColor(96, 96, 96), _gridWidth, Qt::SolidLine));
    } else {
        // Set canvas background
        setCanvasBackground(QColor(0xFF, 0xFF, 0xFF));
        // Configure the plot grid.
        _grid->setMinorPen(QPen(QColor(192, 192, 192), _gridWidth, Qt::SolidLine));
        _grid->setMajorPen(QPen(QColor(128, 128, 128), _gridWidth, Qt::SolidLine));
    }
    // And finally refresh the widget to make sure all color changes are redrawn.
    replot();
}
