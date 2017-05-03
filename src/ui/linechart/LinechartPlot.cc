 /*=====================================================================
======================================================================*/

/**
 * @file
 *   @brief Line chart for vehicle data
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include "float.h"
#include <QDebug>
#include <QTimer>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_zoomer.h>
#include <qwt_symbol.h>
#include <LinechartPlot.h>
#include <MG.h>
#include <QPaintEngine>
#include "ChartPlot.h"

#include "QGC.h"

/**
 * @brief The default constructor
 *
 * @param parent The parent widget
 * @param interval The maximum interval for which data is stored (default: 30 minutes) in milliseconds
 **/
LinechartPlot::LinechartPlot(QWidget *parent, int plotid, quint64 interval):
    ChartPlot(parent),
    minTime(0),
    lastTime(0),
    maxTime(100),
    maxInterval(MAX_STORAGE_INTERVAL),
    plotPosition(0),
    timeScaleStep(DEFAULT_SCALE_INTERVAL), // 10 seconds
    automaticScrollActive(false),
    m_active(false),
    m_groundTime(true),
    d_data(NULL),
    d_curve(NULL)
{
    this->plotid = plotid;
    this->plotInterval = interval;

    maxValue = -DBL_MAX;
    minValue = DBL_MAX;

    //lastMaxTimeAdded = QTime();

    data = QMap<QString, TimeSeriesData*>();
    scaleMaps = QMap<QString, QwtScaleMap*>();

    yScaleEngine = new QwtLinearScaleEngine();
    setAxisScaleEngine(QwtPlot::yLeft, yScaleEngine);

    // Set left scale
    //setAxisOptions(QwtPlot::yLeft, QwtAutoScale::Logarithmic);

    // Set bottom scale
    setAxisScaleDraw(QwtPlot::xBottom, new TimeScaleDraw());
    setAxisLabelRotation(QwtPlot::xBottom, -25.0);
    setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignLeft | Qt::AlignBottom);

    // Add some space on the left and right side of the scale to prevent flickering

    QwtScaleWidget* bottomScaleWidget = axisWidget(QwtPlot::xBottom);
    const int fontMetricsX = QFontMetrics(bottomScaleWidget->font()).height();
    bottomScaleWidget->setMinBorderDist(fontMetricsX * 2, fontMetricsX / 2);

    plotLayout()->setAlignCanvasToScales(true);

    // Start QTimer for plot update
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &LinechartPlot::paintRealtime);
    //updateTimer->start(DEFAULT_REFRESH_RATE);

    connect(&timeoutTimer, &QTimer::timeout, this, &LinechartPlot::removeTimedOutCurves);
    //timeoutTimer.start(5000);
}

LinechartPlot::~LinechartPlot()
{
//    datalock.lock();
//    // Delete curves
//    QMap<QString, QwtPlotCurve*>::iterator i;
//    for(i = curves.begin(); i != curves.end(); ++i) {
//        // Remove from curve list
//        QwtPlotCurve* curve = curves.take(i.key());
//        // Delete the object
//        delete curve;
//        // Set the pointer null
//        curve = NULL;
//    }

//    // Delete data
//    QMap<QString, TimeSeriesData*>::iterator j;
//    for(j = data.begin(); j != data.end(); ++j) {
//        // Remove from data list
//        TimeSeriesData* d = data.take(j.key());
//        // Delete the object
//        delete d;
//        // Set the pointer null
//        d = NULL;
//    }
//    datalock.unlock();
}

void LinechartPlot::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    updateTimer->start(DEFAULT_REFRESH_RATE);
}

void LinechartPlot::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    updateTimer->stop();
}

int LinechartPlot::getPlotId()
{
    return this->plotid;
}

/**
 * @param id curve identifier
 */
double LinechartPlot::getCurrentValue(QString id)
{
    return data.value(id)->getCurrentValue();
}

/**
 * @param id curve identifier
 */
double LinechartPlot::getMean(QString id)
{
    return data.value(id)->getMean();
}

/**
 * @param id curve identifier
 */
double LinechartPlot::getMedian(QString id)
{
    return data.value(id)->getMedian();
}

/**
 * @param id curve identifier
 */
double LinechartPlot::getVariance(QString id)
{
    return data.value(id)->getVariance();
}

int LinechartPlot::getAverageWindow()
{
    return averageWindowSize;
}

/**
 * @brief Set the plot refresh rate
 * The default refresh rate is defined by LinechartPlot::DEFAULT_REFRESH_RATE.
 * @param ms The refresh rate in milliseconds
 **/
void LinechartPlot::setRefreshRate(int ms)
{
    updateTimer->setInterval(ms);
}

void LinechartPlot::setActive(bool active)
{
    m_active = active;
}

void LinechartPlot::removeTimedOutCurves()
{
    foreach(const QString &key, lastUpdate.keys())
    {
        quint64 time = lastUpdate.value(key);
        if (QGC::groundTimeMilliseconds() - time > 10000)
        {
            // Remove this curve
            // Delete curves
            QwtPlotCurve* curve = _curves.take(key);
            // Delete the object
            delete curve;
            // Set the pointer null
            curve = NULL;

            // Notify connected components about the removal
            emit curveRemoved(key);

            // Remove from data list
            TimeSeriesData* d = data.take(key);
            // Delete the object
            delete d;
            // Set the pointer null
            d = NULL;
            emit curveRemoved(key);
        }
    }
}

/**
 * @brief Set the zero (center line) value
 * The zero value defines the centerline of the plot.
 *
 * @param id The id of the curve
 * @param zeroValue The zero value
 **/
void LinechartPlot::setZeroValue(QString id, double zeroValue)
{
    if(data.contains(id)) {
        data.value(id)->setZeroValue(zeroValue);
    } else {
        data.insert(id, new TimeSeriesData(this, id, maxInterval, zeroValue));
    }
}

void LinechartPlot::appendData(QString dataname, quint64 ms, double value)
{
    /* Lock resource to ensure data integrity */
    datalock.lock();

    /* Check if dataset identifier already exists */
    if(!data.contains(dataname)) {
        addCurve(dataname);
        enforceGroundTime(m_groundTime);
//        qDebug() << "ADDING CURVE WITH" << dataname << ms << value;
//        qDebug() << "MINTIME:" << minTime << "MAXTIME:" << maxTime;
//        qDebug() << "LASTTIME:" << lastTime;
    }

    // Add new value
    TimeSeriesData* dataset = data.value(dataname);

    quint64 time;

    // Append data
    if (!m_groundTime)
    {
        // Use timestamp from dataset
        time = ms;
    }
    else
    {
        time = QGC::groundTimeMilliseconds();
    }
    dataset->append(time, value);

    lastUpdate.insert(dataname, time);

    // Scaling values
    if(ms < minTime) minTime = ms;
    if(ms > maxTime) maxTime = ms;
    storageInterval = maxTime - minTime;

    if(time > lastTime)
    {
        //qDebug() << "UPDATED LAST TIME!" << dataname << time << lastTime;
        lastTime = time;
    }

    //
    if (value < minValue) minValue = value;
    if (value > maxValue) maxValue = value;
    valueInterval = maxValue - minValue;

    // Assign dataset to curve
    QwtPlotCurve* curve = _curves.value(dataname);
    curve->setRawSamples(dataset->getPlotX(), dataset->getPlotY(), dataset->getPlotCount());

    //    qDebug() << "mintime" << minTime << "maxtime" << maxTime << "last max time" << "window position" << getWindowPosition();

    datalock.unlock();
}

/**
 * @param enforce true to reset the data timestamp with the receive / ground timestamp
 */
void LinechartPlot::enforceGroundTime(bool enforce)
{
    m_groundTime = enforce;

    if (enforce)
    {
        lastTime = QGC::groundTimeMilliseconds();
        plotPosition = lastTime;
        maxTime = lastTime;
    }
    else
    {
        lastTime = 0;
        plotPosition = 0;
        minTime = 0;
        maxTime = 100;
    }
}

/**
 * @return True if the data points are stamped with the packet receive time
 */
bool LinechartPlot::groundTime()
{
    return m_groundTime;
}

void LinechartPlot::addCurve(QString id)
{
    QColor currentColor = getNextColor();

    // Create new curve and set style
    QwtPlotCurve* curve = new QwtPlotCurve(id);
    // Add curve to list
    _curves.insert(id, curve);

    curve->setStyle(QwtPlotCurve::Lines);
    curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    setCurveColor(id, currentColor);
    //curve->setBrush(currentColor); Leads to a filled curve
    //    curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    curve->attach(this);
    //@TODO Color differentiation between the curves will be necessary

    /* Create symbol for datapoints on curve */
    /*
         * Symbols have significant performance penalty, better avoid them
         *
        QwtSymbol sym = QwtSymbol();
        sym.setStyle(QwtSymbol::Ellipse);
        sym.setPen(currentColor);
        sym.setSize(3);
        curve->setSymbol(sym);*/

    // Create dataset
    TimeSeriesData* dataset = new TimeSeriesData(this, id, this->plotInterval, maxInterval);

    // Add dataset to list
    data.insert(id, dataset);

    // Notify connected components about new curve
    emit curveAdded(id);
}

/**
 * @brief Set the time window for the plot
 * The time window defines which data is shown in the plot.
 *
 * @param end The end of the interval in milliseconds
 * */
void LinechartPlot::setWindowPosition(quint64 end)
{
    windowLock.lock();
    if(end <= this->getMaxTime() && end >= (this->getMinTime() + this->getPlotInterval())) {
        plotPosition = end;
        setAxisScale(QwtPlot::xBottom, (plotPosition - getPlotInterval()), plotPosition, timeScaleStep);
    }
    //@TODO Update the rest of the plot and update drawing
    windowLock.unlock();
}

/**
 * @brief Get the time window position
 * The position marks the right edge of the plot window
 *
 * @return The position of the plot window, in milliseconds
 **/
quint64 LinechartPlot::getWindowPosition()
{
    return plotPosition;
}

/**
 * @brief Set the scaling of the (vertical) y axis
 * The mapping of the variable values on the drawing pane can be
 * adjusted with this method. The default is that the y axis will the chosen
 * to fit all curves in their normal base units. This can however hide all
 * details if large differences in the data values exist.
 *
 * The scaling can be changed to best fit, which fits all curves in a +100 to -100 interval.
 * The logarithmic scaling does not fit the variables, but instead applies a log10
 * scaling to all variables.
 *
 * @param scaling LinechartPlot::SCALE_ABSOLUTE for linear scaling, LinechartPlot::SCALE_BEST_FIT for the best fit scaling and LinechartPlot::SCALE_LOGARITHMIC for the logarithmic scaling.
 **/
void LinechartPlot::setScaling(int scaling)
{
    this->scaling = scaling;
    switch (scaling) {
    case LinechartPlot::SCALE_ABSOLUTE:
        setLinearScaling();
        break;
    case LinechartPlot::SCALE_LOGARITHMIC:
        setLogarithmicScaling();
        break;
    }
}

/**
 * @brief Change the visibility of a curve
 *
 * @param id The string id of the curve
 * @param visible The visibility: True to make it visible
 **/
void LinechartPlot::setVisibleById(QString id, bool visible)
{
    if(_curves.contains(id)) {
        _curves.value(id)->setVisible(visible);
        if(visible)
        {
            _curves.value(id)->attach(this);
        }
        else
        {
            _curves.value(id)->detach();
        }
    }
}

/**
 * @brief Hide a curve.
 *
 * This is a convenience method and maps to setVisible(id, false).
 *
 * @param id The curve to hide
 * @see setVisible() For the implementation
 **/
void LinechartPlot::hideCurve(QString id)
{
    setVisibleById(id, false);
}

/**
 * @brief Show a curve.
 *
 * This is a convenience method and maps to setVisible(id, true);
 *
 * @param id The curve to show
 * @see setVisible() For the implementation
 **/
void LinechartPlot::showCurve(QString id)
{
    setVisibleById(id, true);
}

//void LinechartPlot::showCurve(QString id, int position)
//{
//    //@TODO Implement this position-dependent
//    curves.value(id)->show();
//}

/**
 * @brief Set the color of a curve and its symbols.
 *
 * @param id The id-string of the curve
 * @param color The newly assigned color
 **/
void LinechartPlot::setCurveColor(QString id, QColor color)
{
    QwtPlotCurve* curve = _curves.value(id);
    // Change the color of the curve.
    curve->setPen(QPen(QBrush(color), _curveWidth));

    //qDebug() << "Setting curve" << id << "to" << color;

    // And change the color of the symbol, making sure to preserve the symbol style
    const QwtSymbol *oldSymbol = curve->symbol();
    QwtSymbol *newSymbol = NULL;
    if (oldSymbol) {
        newSymbol = new QwtSymbol(oldSymbol->style(), QBrush(color), QPen(color, _symbolWidth), QSize(_symbolWidth, _symbolWidth));
    }
    curve->setSymbol(newSymbol);
}

/**
 * @brief Check the visibility of a curve
 *
 * @param id The id of the curve
 * @return The visibility, true if it is visible, false otherwise
 **/
bool LinechartPlot::isVisible(QString id)
{
    return _curves.value(id)->isVisible();
}

/**
 * @return The visibility, true if it is visible, false otherwise
 **/
bool LinechartPlot::anyCurveVisible()
{
    bool visible = false;
    foreach (const QString &key, _curves.keys())
    {
        if (_curves.value(key)->isVisible())
        {
            visible = true;
        }
    }

    return visible;
}

/**
 * @brief Allows to block interference of the automatic scrolling with user interaction
 * When the plot is updated very fast (at 1 ms for example) with new data, it might
 * get impossible for an user to interact. Therefore the automatic scrolling must be
 * explicitly activated.
 *
 * @param active The status of automatic scrolling, true to turn it on
 **/
void LinechartPlot::setAutoScroll(bool active)
{
    automaticScrollActive = active;
}

/**
 * @brief Get a list of all curves (visible and not visible curves)
 *
 * @return The list of curves
 **/
QList<QwtPlotCurve*> LinechartPlot::getCurves()
{
    return _curves.values();
}

/**
 * @brief Get the smallest time value in all datasets
 *
 * @return The smallest time value
 **/
quint64 LinechartPlot::getMinTime()
{
    return minTime;
}

/**
 * @brief Get the biggest time value in all datasets
 *
 * @return The biggest time value
 **/
quint64 LinechartPlot::getMaxTime()
{
    return maxTime;
}

/**
 * @brief Get the plot interval
 * The plot interval is the time interval which is displayed on the plot
 *
 * @return The plot inteval in milliseconds
 * @see setPlotInterval()
 * @see getDataInterval() To get the interval for which data is available
 **/
quint64 LinechartPlot::getPlotInterval()
{
    return plotInterval;
}

/**
 * @brief Set the plot interval
 *
 * @param interval The time interval to plot, in milliseconds
 * @see getPlotInterval()
 **/
void LinechartPlot::setPlotInterval(int interval)
{
    //Only ever increase the amount of stored data,
    // so that we allow the user to change between
    // different intervals without constantly losing
    // data points
    if((unsigned)interval > plotInterval) {

        QMap<QString, TimeSeriesData*>::iterator j;
        for(j = data.begin(); j != data.end(); ++j)
        {
            TimeSeriesData* d = data.value(j.key());
            d->setInterval(interval);
        }
    }
    plotInterval = interval;
    if(plotInterval > 5*60*1000) //If the interval is longer than 4 minutes, change the time scale step to 2 minutes
        timeScaleStep = 2*60*1000;
    else if(plotInterval >= 4*60*1000) //If the interval is longer than 4 minutes, change the time scale step to 1 minutes
        timeScaleStep = 1*60*1000;
    else if(plotInterval >= 60*1000) //If the interval is longer than a minute, change the time scale step to 30 seconds
        timeScaleStep = 30*1000;
    else
        timeScaleStep = DEFAULT_SCALE_INTERVAL;

}

/**
 * @brief Get the data interval
 * The data interval is defined by the time interval for which data
 * values are available.
 *
 * @return The data interval
 * @see getPlotInterval() To get the time interval which is currently displayed by the plot
 **/
quint64 LinechartPlot::getDataInterval()
{
    return storageInterval;
}

/**
 * @brief Set logarithmic scaling for the curve
 **/
void LinechartPlot::setLogarithmicScaling()
{
    yScaleEngine = new QwtLogScaleEngine();
    setAxisScaleEngine(QwtPlot::yLeft, yScaleEngine);
}

/**
 * @brief Set linear scaling for the curve
 **/
void LinechartPlot::setLinearScaling()
{
    yScaleEngine = new QwtLinearScaleEngine();
    setAxisScaleEngine(QwtPlot::yLeft, yScaleEngine);
}

void LinechartPlot::setAverageWindow(int windowSize)
{
    this->averageWindowSize = windowSize;
    foreach(TimeSeriesData* series, data)
    {
        series->setAverageWindowSize(windowSize);
    }
}

/**
 * @brief Paint immediately the plot
 * This method is a replacement for replot(). In contrast to replot(), it takes the
 * time window size and eventual zoom interaction into account.
 **/
void LinechartPlot::paintRealtime()
{
    if (m_active) {
#if (QGC_EVENTLOOP_DEBUG)
        static quint64 timestamp = 0;
        qDebug() << "EVENTLOOP: (" << MG::TIME::getGroundTimeNow() - timestamp << ")" << __FILE__ << __LINE__;
        timestamp = MG::TIME::getGroundTimeNow();
#endif
        // Update plot window value to new max time if the last time was also the max time
        windowLock.lock();
        if (automaticScrollActive)
        {

            // FIXME Check, but commenting this out should have been
            // beneficial (does only add complexity)
            //            if (MG::TIME::getGroundTimeNow() > maxTime && abs(MG::TIME::getGroundTimeNow() - maxTime) < 5000000)
            //            {
            //                plotPosition = MG::TIME::getGroundTimeNow();
            //            }
            //            else
            //            {
            plotPosition = lastTime;// + lastMaxTimeAdded.msec();
            //            }
            setAxisScale(QwtPlot::xBottom, plotPosition - plotInterval, plotPosition, timeScaleStep);

            // FIXME Last fix for scroll zoomer is here
            //setAxisScale(QwtPlot::yLeft, minValue + minValue * 0.05, maxValue + maxValue * 0.05f, (maxValue - minValue) / 10.0);
            /* Notify about change. Even if the window position was not changed
                     * itself, the relative position of the window to the interval must
                     * have changed, as the interval likely increased in length */
            emit windowPositionChanged(getWindowPosition());
        }

        windowLock.unlock();

        replot();

        /*
        QMap<QString, QwtPlotCurve*>::iterator i;
        for(i = curves.begin(); i != curves.end(); ++i) {
                const bool cacheMode = canvas()->testPaintAttribute(QwtPlotCanvas::PaintCached);
                canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, false);
                i.value()->drawItems();
                canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, cacheMode);
        }*/


    }
}

/**
 * @brief Removes all data and curves from the plot
 **/
void LinechartPlot::removeAllData()
{
    datalock.lock();
    // Delete curves
    QMap<QString, QwtPlotCurve*>::iterator i;
    for(i = _curves.begin(); i != _curves.end(); ++i)
    {
        // Remove from curve list
        QwtPlotCurve* curve = _curves.take(i.key());
        // Delete the object
        delete curve;
        // Set the pointer null
        curve = NULL;

        // Notify connected components about the removal
        emit curveRemoved(i.key());
    }

    // Delete data
    QMap<QString, TimeSeriesData*>::iterator j;
    for(j = data.begin(); j != data.end(); ++j)
    {
        // Remove from data list
        TimeSeriesData* d = data.take(j.key());
        // Delete the object
        delete d;
        // Set the pointer null
        d = NULL;
    }
    datalock.unlock();
    replot();
}


TimeSeriesData::TimeSeriesData(QwtPlot* plot, QString friendlyName, quint64 plotInterval, quint64 maxInterval, double zeroValue):
    minValue(DBL_MAX),
    maxValue(DBL_MIN),
    zeroValue(0),
    count(0),
    mean(0.0),
    median(0.0),
    variance(0.0),
    averageWindow(50)
{
    this->plot = plot;
    this->friendlyName = friendlyName;
    this->maxInterval = maxInterval;
    this->zeroValue = zeroValue;
    this->plotInterval = plotInterval;

    /* initialize time */
    startTime = QUINT64_MAX;
    stopTime = QUINT64_MIN;

    plotCount = 0;
}

TimeSeriesData::~TimeSeriesData()
{

}

void TimeSeriesData::setInterval(quint64 ms)
{
    plotInterval = ms;
}

void TimeSeriesData::setAverageWindowSize(int windowSize)
{
    this->averageWindow = windowSize;
}

/**
 * @brief Append a data point to this data set
 *
 * @param ms The time in milliseconds
 * @param value The data value
 **/
void TimeSeriesData::append(quint64 ms, double value)
{
    dataMutex.lock();
    // Qt will automatically use a smart growth strategy: http://doc.qt.io/qt-5/containers.html#growth-strategies
    this->ms.append(ms);
    this->value.append(value);
    this->lastValue = value;
    this->mean = 0;
    //QList<double> medianList = QList<double>();
    for (unsigned int i = 0; (i < averageWindow) && (((int)count - (int)i) >= 0); ++i) {
        this->mean += this->value[count-i];
        //medianList.append(this->value[count-i]);
    }
    this->mean = mean / static_cast<double>(qMin(averageWindow,static_cast<unsigned int>(count)));

    this->variance = 0;
    for (unsigned int i = 0; (i < averageWindow) && (((int)count - (int)i) >= 0); ++i) {
        this->variance += (this->value[count-i] - mean) * (this->value[count-i] - mean);
    }
    this->variance = this->variance / static_cast<double>(qMin(averageWindow,static_cast<unsigned int>(count)));

//    qSort(medianList);

//    if (medianList.count() > 2)
//    {
//        if (medianList.count() % 2 == 0)
//        {
//            median = (medianList.at(medianList.count()/2) + medianList.at(medianList.count()/2+1)) / 2.0;
//        }
//        else
//        {
//            median = medianList.at(medianList.count()/2+1);
//        }
//    }

    // Update statistical values
    if(ms < startTime) startTime = ms;
    if(ms > stopTime) stopTime = ms;
    interval = stopTime - startTime;

    if (interval > plotInterval) {
        while (this->ms[count - plotCount] < stopTime - plotInterval) {
            plotCount--;
        }
    }

    count++;
    plotCount++;

    if(minValue > value) minValue = value;
    if(maxValue < value) maxValue = value;

    // Trim dataset if necessary
    if(maxInterval > 0) {
        // maxInterval = 0 means infinite

        if(interval > maxInterval && !this->ms.isEmpty() && !this->value.isEmpty()) {
            // The time at which this time series should be cut
            double minTime = stopTime - maxInterval;
            // Delete elements from the start of the list as long the time
            // value of this elements is before the cut time
            while(this->ms.first() < minTime) {
                this->ms.pop_front();
                this->value.pop_front();
            }
        }
    }
    dataMutex.unlock();
}

/**
 * @brief Get the id of this data set
 *
 * @return The id-string
 **/
int TimeSeriesData::getID()
{
    return id;
}

/**
 * @brief Get the minimum value in the data set
 *
 * @return The minimum value
 **/
double TimeSeriesData::getMinValue()
{
    return minValue;
}

/**
 * @brief Get the maximum value in the data set
 *
 * @return The maximum value
 **/
double TimeSeriesData::getMaxValue()
{
    return maxValue;
}

/**
 * @return the mean
 */
double TimeSeriesData::getMean()
{
    return mean;
}

/**
 * @return the median
 */
double TimeSeriesData::getMedian()
{
    return median;
}

/**
 * @return the variance
 */
double TimeSeriesData::getVariance()
{
    return variance;
}

double TimeSeriesData::getCurrentValue()
{
    return lastValue;
}

/**
 * @brief Get the zero (center) value in the data set
 * The zero value is not a statistical value, but instead manually defined
 * when creating the data set.
 * @return The zero value
 **/
double TimeSeriesData::getZeroValue()
{
    return zeroValue;
}

/**
 * @brief Set the zero (center) value
 *
 * @param zeroValue The zero value
 * @see getZeroValue()
 **/
void TimeSeriesData::setZeroValue(double zeroValue)
{
    this->zeroValue = zeroValue;
}

/**
 * @brief Get the number of points in the dataset
 *
 * @return The number of points
 **/
int TimeSeriesData::getCount() const
{
    return count;
}

/**
 * @brief Get the number of points in the plot selection
 *
 * @return The number of points
 **/
int TimeSeriesData::getPlotCount() const
{
    return plotCount;
}

/**
 * @brief Get the data array size
 * The data array size is \e NOT equal to the number of items in the data set, as
 * array space is pre-allocated. Use getCount() to get the number of data points.
 *
 * @return The data array size
 * @see getCount()
 **/
int TimeSeriesData::size() const
{
    return ms.size();
}

/**
 * @brief Get the X (time) values
 *
 * @return The x values
 **/
const double* TimeSeriesData::getX() const
{
    return ms.data();
}

const double* TimeSeriesData::getPlotX() const
{
    return ms.data() + (count - plotCount);
}

/**
 * @brief Get the Y (data) values
 *
 * @return The y values
 **/
const double* TimeSeriesData::getY() const
{
    return value.data();
}

const double* TimeSeriesData::getPlotY() const
{
    return value.data() + (count - plotCount);
}
