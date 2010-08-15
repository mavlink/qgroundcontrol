#ifndef INCREMENTALPLOT_H
#define INCREMENTALPLOT_H

#include <QTimer>
#include <qwt_array.h>
#include <qwt_plot.h>
#include <QMap>
#include "ScrollZoomer.h"

class QwtPlotCurve;

class CurveData
{
    // A container class for growing data
public:

    CurveData();

    void append(double *x, double *y, int count);

    int count() const;
    int size() const;
    const double *x() const;
    const double *y() const;

private:
    int d_count;
    QwtArray<double> d_x;
    QwtArray<double> d_y;
    QTimer *d_timer;
    int d_timerCount;
};

class IncrementalPlot : public QwtPlot
{
    Q_OBJECT
public:
    IncrementalPlot(QWidget *parent = NULL);
    virtual ~IncrementalPlot();

    void appendData(QString key, double x, double y);
    void appendData(QString key, double *x, double *y, int size);

    void resetScaling();
    void removeData();

    /** @brief Get color map of this plot */
    QList<QColor> IncrementalPlot::getColorMap();
    /** @brief Get next color of color map */
    QColor IncrementalPlot::getNextColor();
    /** @brief Get color for curve id */
    QColor IncrementalPlot::getColorForCurve(QString id);

protected:
    QList<QColor> colors;
    int nextColor;
    ScrollZoomer* zoomer;
    double xmin;
    double xmax;
    double ymin;
    double ymax;

private:
    QMap<QString, CurveData* > d_data;
    QMap<QString, QwtPlotCurve* > d_curve;
};

#endif /* INCREMENTALPLOT_H */
