/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Plot of a Linechart
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _LINECHARTPLOT_H_
#define _LINECHARTPLOT_H_

#define QUINT64_MIN Q_UINT64_C(0)
#define QUINT64_MAX Q_UINT64_C(18446744073709551615)

#include <QMap>
#include <QList>
#include <QMutex>
#include <QTime>
#include <qwt_plot_panner.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_engine.h>
#include <qwt_array.h>
#include <qwt_plot.h>
#include <ScrollZoomer.h>
#include <MG.h>

class TimeScaleDraw: public QwtScaleDraw
{
public:

    virtual QwtText label(double v) const
    {
        QDateTime time = MG::TIME::msecToQDateTime(static_cast<quint64>(v));
        return time.toString("hh:mm:ss"); // was hh:mm:ss:zzz
        // Show seconds since system startup
        //return QString::number(static_cast<int>(v)/1000000);
    }

};

/**
 * @brief Zoomer for plot
 */
class Zoomer: public ScrollZoomer
{
public:
    Zoomer(QwtPlotCanvas *canvas) : ScrollZoomer(canvas) {}
    virtual void rescale() {

        QwtScaleWidget *scaleWidget = plot()->axisWidget(yAxis());
        QwtScaleDraw *sd = scaleWidget->scaleDraw();
        int minExtent = 0;
        if ( zoomRectIndex() > 0 ) {

            // When scrolling in vertical direction
            // the plot is jumping in horizontal direction
            // because of the different widths of the labels
            // So we better use a fixed extent.

            minExtent = sd->spacing() + sd->majTickLength() + 1;
            minExtent += sd->labelSize(scaleWidget->font(), 1000).width();
        }

        sd->setMinimumExtent(minExtent);
        ScrollZoomer::rescale();

    }

};


/**
 * @brief Data container
 */
class QwtPlotCurve;

/**
 * @brief Container class for the time series data
 *
 **/
class TimeSeriesData
{
public:

    TimeSeriesData(QwtPlot* plot, QString friendlyName = "data", quint64 plotInterval = 30000, quint64 maxInterval = 0, double zeroValue = 0);
    ~TimeSeriesData();

    void append(quint64 ms, double value);

    QwtScaleMap* getScaleMap();

    int getCount() const;
    int size() const;
    const double* getX() const;
    const double* getY() const;

    const double* getPlotX() const;
    const double* getPlotY() const;
    int getPlotCount() const;

    int getID();
    QString getFriendlyName();
    double getMinValue();
    double getMaxValue();
    double getZeroValue();
    /** @brief Get the short-term mean */
    double getMean();
    /** @brief Get the short-term median */
    double getMedian();
    /** @brief Get the current value */
    double getCurrentValue();
    void setZeroValue(double zeroValue);
    void setInterval(quint64 ms);
    void setAverageWindowSize(int windowSize);

protected:
    QwtPlot* plot;
    quint64 startTime;
    quint64 stopTime;
    quint64 interval;
    quint64 plotInterval;
    quint64 maxInterval;
    int id;
    quint64 plotCount;
    QString friendlyName;

    double lastValue; ///< The last inserted value
    double minValue;  ///< The smallest value in the dataset
    double maxValue;  ///< The largest value in the dataset
    double zeroValue; ///< The expected value in the dataset

    QMutex dataMutex;

    QwtScaleMap* scaleMap;

    void updateScaleMap();

private:
    quint64 count;
    QwtArray<double> ms;
    QwtArray<double> value;
    double mean;
    double median;
    unsigned int averageWindow;
    QwtArray<double> outputMs;
    QwtArray<double> outputValue;
};





/**
 * @brief Time series plot
 **/
class LinechartPlot : public QwtPlot
{
    Q_OBJECT
public:
    LinechartPlot(QWidget *parent = NULL, int plotid=0, quint64 interval = LinechartPlot::DEFAULT_PLOT_INTERVAL);
    virtual ~LinechartPlot();

    void setZeroValue(QString id, double zeroValue);
    void removeAllData();

    QList<QwtPlotCurve*> getCurves();
    bool isVisible(QString id);

    int getPlotId();
    /** @brief Get the number of values to average over */
    int getAverageWindow();

    quint64 getMinTime();
    quint64 getMaxTime();
    quint64 getPlotInterval();
    quint64 getDataInterval();
    quint64 getWindowPosition();
    QList<QColor> getColorMap();

    /** @brief Get the short-term mean of a curve */
    double getMean(QString id);
    /** @brief Get the short-term median of a curve */
    double getMedian(QString id);
    /** @brief Get the last inserted value */
    double getCurrentValue(QString id);

    static const int SCALE_ABSOLUTE = 0;
    static const int SCALE_BEST_FIT = 1;
    static const int SCALE_LOGARITHMIC = 2;

    static const int DEFAULT_REFRESH_RATE = 40; ///< The default refresh rate is 25 Hz / every 100 ms
    static const int DEFAULT_PLOT_INTERVAL = 1000 * 10; ///< The default plot interval is 10 seconds
    static const int DEFAULT_SCALE_INTERVAL = 1000 * 5;

public slots:
    void setRefreshRate(int ms);
    /**
     * @brief Append data to the plot
     *
     * The new data point is appended to the curve with the id-String id. If the curve
     * doesn't yet exist it is created and added to the plot.
     *
     * @param uasId id of originating UAS
     * @param dataname unique string (also used to label the data)
     * @param ms time measure of the data point, in milliseconds
     * @param value value of the data point
     */
    void appendData(QString dataname, quint64 ms, double value);
    void hideCurve(QString id);
    void showCurve(QString id);
    /** @brief Enable auto-refreshing of plot */
    void setActive(bool active);

    // Functions referring to the currently active plot
    void setVisible(QString id, bool visible);
    //void showCurve(QString id, int position);
    void setCurveColor(QString id, QColor color);

    /** @brief Enforce the use of the receive timestamp */
    void enforceGroundTime(bool enforce);

    // General interaction
    void setWindowPosition(quint64 end);
    void setPlotInterval(int interval);
    void setScaling(int scaling);
    void setAutoScroll(bool active);
    void paintRealtime();

    /** @brief Set logarithmic plot y-axis scaling */
    void setLogarithmicScaling();
    /** @brief Set linear plot y-axis scaling */
    void setLinearScaling();

    /** @brief Set the number of values to average over */
    void setAverageWindow(int windowSize);

    QColor getColorForCurve(QString id);

protected:
    QMap<QString, QwtPlotCurve*> curves;
    QMap<QString, TimeSeriesData*> data;
    QMap<QString, QwtScaleMap*> scaleMaps;
    ScrollZoomer* zoomer;

    QList<QColor> colors;
    int nextColor;

    //static const quint64 MAX_STORAGE_INTERVAL = Q_UINT64_C(300000);
    static const quint64 MAX_STORAGE_INTERVAL = Q_UINT64_C(0);  ///< The maximum interval which is stored
    // TODO CHECK THIS!!!
    int scaling;
    QwtScaleEngine* yScaleEngine;
    quint64 minTime; ///< The smallest timestamp occured so far
    quint64 maxTime; ///< The biggest timestamp occured so far
    quint64 maxInterval;
    quint64 storageInterval;

    double maxValue;
    double minValue;
    double valueInterval;

    int averageWindowSize; ///< Size of sliding average / sliding median

    quint64 plotInterval;
    quint64 plotPosition;
    QTimer* updateTimer;
    QMutex datalock;
    QMutex windowLock;
    quint64 timeScaleStep;
    bool automaticScrollActive;
    QTime lastMaxTimeAdded;
    int plotid;
    bool m_active; ///< Decides wether the plot is active or not
    bool m_groundTime; ///< Enforce the use of the receive timestamp instead of the data timestamp

    // Methods
    void addCurve(QString id);
    QColor getNextColor();

private:
    TimeSeriesData* d_data;
    QwtPlotCurve* d_curve;

signals:

    /**
         * @brief This signal is emitted when a new curve is added
         *
         * @param color The curve color in the diagram
         **/
    void curveAdded(QString idstring);
    /**
         * @brief This signal is emitted when a curve gets assigned a color
         *
         * @param idstring The id-string of the curve
         * @param color The curve color in the diagram
         **/
    void colorSet(QString idstring, QColor color);
    /**
         * @brief This signal is emitted when a curve is removed
         *
         * @param name The id-string of the curve
         **/
    void curveRemoved(QString name);
    /**
         * @brief This signal is emitted when the plot window position changes
         *
         * @param position The position of the right edge of the window, in milliseconds
         **/
    void windowPositionChanged(quint64 position);
};

#endif // _LINECHARTPLOT_H_
