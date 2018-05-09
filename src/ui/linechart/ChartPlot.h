#ifndef CHARTPLOT_H
#define CHARTPLOT_H

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include "MainWindow.h"
#include "ScrollZoomer.h"

class ChartPlot : public QwtPlot
{
    Q_OBJECT
public:
    ChartPlot(QWidget *parent = NULL);
    virtual ~ChartPlot();

    /** @brief Get next color of color map */
    QColor getNextColor();

    /** @brief Get color for curve id */
    QColor getColorForCurve(const QString &id);

    /** @brief Reset color map */
    void shuffleColors();

public slots:

    /** @brief Generate coloring for this plot canvas based on current window theme */
    void styleChanged(bool styleIsDark);

protected:
    const static int numColors = 20;
    const static QColor baseColors[numColors];

    QList<QColor>                   _colors;         ///< Colormap for curves
    int                             _nextColorIndex; ///< Next index in color map
    QMap<QString, QwtPlotCurve* >   _curves;         ///< Plot curves
    QwtPlotGrid*                    _grid;           ///< Plot grid

    float _symbolWidth;  ///< Width of curve symbols in pixels
    float _curveWidth;   ///< Width of curve lines in pixels
    float _gridWidth;    ///< Width of gridlines in pixels
};

#endif // CHARTPLOT_H
