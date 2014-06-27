/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Defition of class IncrementalPlot
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef INCREMENTALPLOT_H
#define INCREMENTALPLOT_H

#include <QTimer>
#include <qwt_plot.h>
#include <qwt_legend.h>
#include <QMap>
#include "ChartPlot.h"

class QwtPlotCurve;

/**
 * @brief Plot data container for growing data
 */
class CurveData
{
public:
    CurveData();

    void append(double *x, double *y, int count);

    /** @brief The number of datasets held in the data structure */
    int count() const;
    /** @brief The reserved size of the data structure in units */
    int size() const;
    const double *x() const;
    const double *y() const;

private:
    int d_count;
    QVector<double> d_x;
    QVector<double> d_y;
};

/**
 * @brief Incremental plotting widget
 *
 * This widget plots data incrementally when new data arrives.
 * It will only repaint the minimum screen content necessary to avoid
 * a too high CPU consumption. It auto-scales the plot to new data.
 */
class IncrementalPlot : public ChartPlot
{
    Q_OBJECT
public:
    /** @brief Create a new, empty incremental plot */
    IncrementalPlot(QWidget *parent = NULL);
    virtual ~IncrementalPlot();

    /** @brief Get the state of the grid */
    bool gridEnabled() const;

    /** @brief Read out data from a curve */
    int data(const QString &key, double* r_x, double* r_y, int maxSize);

public slots:
    /** @brief Append one data point */
    void appendData(const QString &key, double x, double y);

    /** @brief Append multiple data points */
    void appendData(const QString &key, double* x, double* y, int size);

    /** @brief Reset the plot scaling to the default value */
    void resetScaling();

    /** @brief Update the plot scale based on current data/symmetric mode */
    void updateScale();

    /** @brief Remove all data from the plot and repaint */
    void removeData();

    /** @brief Show the plot legend */
    void showLegend(bool show);

    /** @brief Show the plot grid */
    void showGrid(bool show);

    /** @brief Set new plot style */
    void setStyleText(const QString &style);

    /** @brief Set symmetric axis scaling mode */
    void setSymmetric(bool symmetric);

protected slots:
    /** @brief Handle the click on a legend item */
    void handleLegendClick(QwtPlotItem* item, bool on);

protected:
    bool symmetric;        ///< Enable symmetric plotting
    QwtLegend* legend;     ///< Plot legend
    double xmin;           ///< Minimum x value seen
    double xmax;           ///< Maximum x value seen
    double ymin;           ///< Minimum y value seen
    double ymax;           ///< Maximum y value seen
    QString styleText;     ///< Curve style set by setStyleText

private:
    QMap<QString, CurveData* > d_data;      ///< Data points
    /** Helper function to apply styleText style to the given curve */
    void updateStyle(QwtPlotCurve *curve);
};

#endif /* INCREMENTALPLOT_H */
