#ifndef CHARTPLOT_H
#define CHARTPLOT_H

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
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
    QColor getColorForCurve(QString id);

    /** @brief Reset color map */
    void shuffleColors();

protected:
    const static int numBaseColors;
    QList<QColor> colors;  ///< Colormap for curves
    int nextColor;         ///< Next index in color map
    QMap<QString, QwtPlotCurve* > curves;  ///< Plot curves
    ScrollZoomer* zoomer;  ///< Zoomer class for widget
    QwtPlotGrid* grid;     ///< Plot grid

protected slots:

    /** @brief Generate coloring for this plot canvas based on current window theme */
    void applyColorScheme(MainWindow::QGC_MAINWINDOW_STYLE style);
};

#endif // CHARTPLOT_H
