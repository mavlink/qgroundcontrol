#ifndef INCREMENTALPLOT_H
#define INCREMENTALPLOT_H

#include <QTimer>
#include <qwt_array.h>
#include <qwt_plot.h>
#include <qwt_legend.h>
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

    /** @brief Get color map of this plot */
    QList<QColor> getColorMap();
    /** @brief Get next color of color map */
    QColor getNextColor();
    /** @brief Get color for curve id */
    QColor getColorForCurve(QString id);

public slots:
    void appendData(QString key, double x, double y);
    void appendData(QString key, double *x, double *y, int size);

    void resetScaling();
    void removeData();
    /** @brief Show the plot legend */
    void showLegend(bool show);
    /** @brief Set new plot style */
    void setStyleText(QString style);

protected:
    QList<QColor> colors;
    int nextColor;
    ScrollZoomer* zoomer;
    QwtLegend* legend;
    double xmin;
    double xmax;
    double ymin;
    double ymax;

private:
    QMap<QString, CurveData* > d_data;
    QMap<QString, QwtPlotCurve* > d_curve;
};

#endif /* INCREMENTALPLOT_H */
