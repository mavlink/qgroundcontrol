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
 *   @brief Line chart plot widget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QDebug>
#include <QWidget>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QToolButton>
#include <QScrollBar>
#include <QLabel>
#include <QMenu>
#include <QSpinBox>
#include <QColor>
#include <QPalette>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>

#include "LinechartWidget.h"
#include "LinechartPlot.h"
#include "LogCompressor.h"
#include "MG.h"


LinechartWidget::LinechartWidget(int systemid, QWidget *parent) : QWidget(parent),
sysid(systemid),
activePlot(NULL),
curvesLock(new QReadWriteLock()),
plotWindowLock(),
curveListIndex(0),
curveListCounter(0),
listedCurves(new QList<QString>()),
curveLabels(new QMap<QString, QLabel*>()),
curveMeans(new QMap<QString, QLabel*>()),
curveMedians(new QMap<QString, QLabel*>()),
curveMenu(new QMenu(this)),
logFile(new QFile()),
logindex(1),
logging(false),
updateTimer(new QTimer())
{
    // Add elements defined in Qt Designer
    ui.setupUi(this);
    this->setMinimumSize(600, 300);

    // Add and customize curve list elements (left side)
    curvesWidget = new QWidget(ui.curveListWidget);
    ui.curveListWidget->setWidget(curvesWidget);
    curvesWidgetLayout = new QVBoxLayout(curvesWidget);
    curvesWidgetLayout->setMargin(2);
    curvesWidgetLayout->setSpacing(4);
    curvesWidgetLayout->setSizeConstraint(QLayout::SetMinimumSize);
    curvesWidget->setLayout(curvesWidgetLayout);

    // Add and customize plot elements (right side)

    // Create the layout
    createLayout();
    
    // Add the last actions
    connect(this, SIGNAL(plotWindowPositionUpdated(int)), scrollbar, SLOT(setValue(int)));
    connect(scrollbar, SIGNAL(sliderMoved(int)), this, SLOT(setPlotWindowPosition(int)));

    updateTimer->setInterval(100);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    updateTimer->start();
}

LinechartWidget::~LinechartWidget() {
    stopLogging();
    delete listedCurves;
    listedCurves = NULL;
}

void LinechartWidget::createLayout()
{
    // Create actions
    createActions();

    // Setup the plot group box area layout
    QGridLayout* layout = new QGridLayout(ui.diagramGroupBox);
    mainLayout = layout;
    layout->setSpacing(4);
    layout->setMargin(2);

    // Create plot container widget
    activePlot = new LinechartPlot(this, sysid);
    // Activate automatic scrolling
    activePlot->setAutoScroll(true);

    // TODO Proper Initialization needed
    //    activePlot = getPlot(0);
    //    plotContainer->setPlot(activePlot);

    layout->addWidget(activePlot, 0, 0, 1, 6);
    layout->setRowStretch(0, 10);
    layout->setRowStretch(1, 0);

    // Linear scaling button
    scalingLinearButton = createButton(this);
    scalingLinearButton->setDefaultAction(setScalingLinear);
    scalingLinearButton->setCheckable(true);
    layout->addWidget(scalingLinearButton, 1, 0);
    layout->setColumnStretch(0, 0);

    // Logarithmic scaling button
    scalingLogButton = createButton(this);
    scalingLogButton->setDefaultAction(setScalingLogarithmic);
    scalingLogButton->setCheckable(true);
    layout->addWidget(scalingLogButton, 1, 1);
    layout->setColumnStretch(1, 0);

    // Averaging spin box
    averageSpinBox = new QSpinBox(this);
    averageSpinBox->setValue(2);
    averageSpinBox->setMinimum(2);
    layout->addWidget(averageSpinBox, 1, 2);
    layout->setColumnStretch(2, 0);
    connect(averageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAverageWindow(int)));

    // Log Button
    logButton = new QToolButton(this);
    logButton->setText(tr("Start Logging"));
    layout->addWidget(logButton, 1, 3);
    layout->setColumnStretch(3, 0);
    connect(logButton, SIGNAL(clicked()), this, SLOT(startLogging()));

    // Ground time button
    QToolButton* timeButton = new QToolButton(this);
    timeButton->setText(tr("Ground Time"));
    timeButton->setCheckable(true);
    timeButton->setChecked(false);
    layout->addWidget(timeButton, 1, 4);
    layout->setColumnStretch(4, 0);
    connect(timeButton, SIGNAL(clicked(bool)), activePlot, SLOT(enforceGroundTime(bool)));

    // Create the scroll bar
    scrollbar = new QScrollBar(Qt::Horizontal, ui.diagramGroupBox);
    scrollbar->setMinimum(MIN_TIME_SCROLLBAR_VALUE);
    scrollbar->setMaximum(MAX_TIME_SCROLLBAR_VALUE);
    scrollbar->setPageStep(PAGESTEP_TIME_SCROLLBAR_VALUE);
    // Set scrollbar to maximum and disable it
    scrollbar->setValue(MIN_TIME_SCROLLBAR_VALUE);
    scrollbar->setDisabled(true);
    //    scrollbar->setFixedHeight(20);


    // Add scroll bar to layout and make sure it gets all available space
    layout->addWidget(scrollbar, 1, 5);
    layout->setColumnStretch(5, 10);

    ui.diagramGroupBox->setLayout(layout);

    // Add actions
    averageSpinBox->setValue(activePlot->getAverageWindow());

    // Connect notifications from the user interface to the plot
    connect(this, SIGNAL(curveRemoved(QString)), activePlot, SLOT(hideCurve(QString)));
    //connect(this, SIGNAL(curveSet(QString, int)), activePlot, SLOT(showshowCurveCurve(QString, int)));
    // FIXME

    // Connect notifications from the plot to the user interface
    connect(activePlot, SIGNAL(curveAdded(QString)), this, SLOT(addCurve(QString)));
    connect(activePlot, SIGNAL(curveRemoved(QString)), this, SLOT(removeCurve(QString)));

    // Scrollbar

    // Update scrollbar when plot window changes (via translator method setPlotWindowPosition()
    connect(activePlot, SIGNAL(windowPositionChanged(quint64)), this, SLOT(setPlotWindowPosition(quint64)));

    // Update plot when scrollbar is moved (via translator method setPlotWindowPosition()
    connect(this, SIGNAL(plotWindowPositionUpdated(quint64)), activePlot, SLOT(setWindowPosition(quint64)));

    // Set scaling
    connect(scalingLinearButton, SIGNAL(clicked()), activePlot, SLOT(setLinearScaling()));
    connect(scalingLogButton, SIGNAL(clicked()), activePlot, SLOT(setLogarithmicScaling()));
}

void LinechartWidget::appendData(int uasId, QString curve, double value, quint64 usec)
{
    // Order matters here, first append to plot, then update curve list
    activePlot->appendData(curve, usec, value);
    // Store data
    QLabel* label = curveLabels->value(curve, NULL);
    // Make sure the curve will be created if it does not yet exist
    if(!label)
    {
        addCurve(curve);
    }

    // Log data
    if (logging)
    {
        if (activePlot->isVisible(curve))
        {
            logFile->write(QString(QString::number(usec) + "\t" + QString::number(uasId) + "\t" + curve + "\t" + QString::number(value) + "\n").toLatin1());
            logFile->flush();
        }
    }
}

void LinechartWidget::refresh()
{
    QString str;

    QMap<QString, QLabel*>::iterator i;
    for (i = curveLabels->begin(); i != curveLabels->end(); ++i)
    {
        str.sprintf("%+.2f", activePlot->getCurrentValue(i.key()));
        // Value
        i.value()->setText(str);
    }
    // Mean
    QMap<QString, QLabel*>::iterator j;
    for (j = curveMeans->begin(); j != curveMeans->end(); ++j)
    {
        str.sprintf("%+.2f", activePlot->getMean(j.key()));
        j.value()->setText(str);
    }
    QMap<QString, QLabel*>::iterator k;
    for (k = curveMedians->begin(); k != curveMedians->end(); ++k)
    {
        // Median
        str.sprintf("%+.2f", activePlot->getMedian(k.key()));
        k.value()->setText(str);
    }
}


void LinechartWidget::startLogging()
{
    // Let user select the log file name
    QDate date(QDate::currentDate());
    // QString("./pixhawk-log-" + date.toString("yyyy-MM-dd") + "-" + QString::number(logindex) + ".log")
    QString fileName = QFileDialog::getSaveFileName(this, tr("Specify log file name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("Logfile (*.txt, *.csv);;"));
    // Store reference to file
    // Append correct file ending if needed
    bool abort = false;
    while (!(fileName.endsWith(".txt") || fileName.endsWith(".csv")))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Unsuitable file extension for logfile");
        msgBox.setInformativeText("Please choose .txt or .csv as file extension. Click OK to change the file extension, cancel to not start logging.");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            abort = true;
            break;
        }
        fileName = QFileDialog::getSaveFileName(this, tr("Specify log file name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("Logfile (*.txt, *.csv);;"));

    }

    // Check if the user did not abort the file save dialog
    if (!abort && fileName != "")
    {
        logFile = new QFile(fileName);
        if (logFile->open(QIODevice::WriteOnly | QIODevice::Text))
        {
            logging = true;
            logindex++;
            logButton->setText(tr("Stop logging"));
            disconnect(logButton, SIGNAL(clicked()), this, SLOT(startLogging()));
            connect(logButton, SIGNAL(clicked()), this, SLOT(stopLogging()));
        }
    }
}

void LinechartWidget::stopLogging()
{
    logging = false;
    if (logFile->isOpen())
    {
        logFile->flush();
        logFile->close();
        // Postprocess log file
        compressor = new LogCompressor(logFile->fileName());
    }
    logButton->setText(tr("Start logging"));
    disconnect(logButton, SIGNAL(clicked()), this, SLOT(stopLogging()));
    connect(logButton, SIGNAL(clicked()), this, SLOT(startLogging()));
}

/**
 * The average window size defines the width of the sliding average
 * filter. It also defines the width of the sliding median filter.
 *
 * @param windowSize with (in values) of the sliding average/median filter. Minimum is 2
 */
void LinechartWidget::setAverageWindow(int windowSize)
{
    if (windowSize > 1) activePlot->setAverageWindow(windowSize);
}

void LinechartWidget::createActions()
{
    setScalingLogarithmic = new QAction("LOG", this);
    setScalingLinear = new QAction("LIN", this);
}

/**
 * @brief Add a curve to the curve list
 *
 * @param curve The id-string of the curve
 * @see removeCurve()
 **/
void LinechartWidget::addCurve(QString curve)
{
    curvesWidgetLayout->addWidget(createCurveItem(curve));
}

QWidget* LinechartWidget::createCurveItem(QString curve)
{
    LinechartPlot* plot = activePlot;
    QWidget* form = new QWidget(this);
    QHBoxLayout *horizontalLayout;
    QCheckBox *checkBox;
    QLabel* label;
    QLabel* value;
    QLabel* mean;
    QLabel* median;

    form->setAutoFillBackground(false);
    horizontalLayout = new QHBoxLayout(form);
    horizontalLayout->setSpacing(5);
    horizontalLayout->setMargin(0);
    horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);

    checkBox = new QCheckBox(form);
    checkBox->setCheckable(true);
    checkBox->setObjectName(curve);

    horizontalLayout->addWidget(checkBox);

    QWidget* colorIcon = new QWidget(form);
    colorIcon->setMinimumSize(QSize(5, 14));
    colorIcon->setMaximumSize(4, 14);

    horizontalLayout->addWidget(colorIcon);

    label = new QLabel(form);
    horizontalLayout->addWidget(label);

    //checkBox->setText(QString());
    label->setText(curve);
    QColor color = plot->getColorForCurve(curve);
    if(color.isValid()) {
        QString colorstyle;
        colorstyle = colorstyle.sprintf("QWidget { background-color: #%X%X%X; }", color.red(), color.green(), color.blue());
        colorIcon->setStyleSheet(colorstyle);
        colorIcon->setAutoFillBackground(true);
    }

    // Value
    value = new QLabel(form);
    value->setNum(0.00);
    curveLabels->insert(curve, value);
    horizontalLayout->addWidget(value);

    // Mean
    mean = new QLabel(form);
    mean->setNum(0.00);
    curveMeans->insert(curve, mean);
    horizontalLayout->addWidget(mean);

    // Median
    median = new QLabel(form);
    value->setNum(0.00);
    curveMedians->insert(curve, median);
    horizontalLayout->addWidget(median);

    /* Color picker
    QColor color = QColorDialog::getColor(Qt::green, this);
         if (color.isValid()) {
             colorLabel->setText(color.name());
             colorLabel->setPalette(QPalette(color));
             colorLabel->setAutoFillBackground(true);
         }
        */

    // Set stretch factors so that the label gets the whole space
    horizontalLayout->setStretchFactor(checkBox, 0);
    horizontalLayout->setStretchFactor(colorIcon, 0);
    horizontalLayout->setStretchFactor(label, 80);
    horizontalLayout->setStretchFactor(value, 50);
    horizontalLayout->setStretchFactor(mean, 50);
    horizontalLayout->setStretchFactor(median, 50);

    // Connect actions
    QObject::connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(takeButtonClick(bool)));
    QObject::connect(this, SIGNAL(curveVisible(QString, bool)), plot, SLOT(setVisible(QString, bool)));

    // Set UI components to initial state
    checkBox->setChecked(false);
    plot->setVisible(curve, false);

    return form;
}

/**
 * @brief Remove the curve from the curve list.
 *
 * @param curve The curve to remove
 * @see addCurve()
 **/
void LinechartWidget::removeCurve(QString curve)
{
    Q_UNUSED(curve)
    //TODO @todo Ensure that the button for a curve gets deleted when the original curve is deleted
    // Remove name
    }

void LinechartWidget::setActive(bool active)
{
    if (activePlot)
    {
        activePlot->setActive(active);
    }
    if (active)
    {
        updateTimer->start();
    }
    else
    {
        updateTimer->stop();
    }
}

/**
 * @brief Set the position of the plot window.
 * The plot covers only a portion of the complete time series. The scrollbar
 * allows to select a window of the time series. The right edge of the window is
 * defined proportional to the position of the scrollbar.
 *
 * @param scrollBarValue The value of the scrollbar, in the range from MIN_TIME_SCROLLBAR_VALUE to MAX_TIME_SCROLLBAR_VALUE
 **/
void LinechartWidget::setPlotWindowPosition(int scrollBarValue) {
    plotWindowLock.lockForWrite();
    // Disable automatic scrolling immediately
    int scrollBarRange = (MAX_TIME_SCROLLBAR_VALUE - MIN_TIME_SCROLLBAR_VALUE);
    double position = (static_cast<double>(scrollBarValue) - MIN_TIME_SCROLLBAR_VALUE) / scrollBarRange;
    quint64 scrollInterval;

    // Activate automatic scrolling if scrollbar is at the right edge
    if(scrollBarValue > MAX_TIME_SCROLLBAR_VALUE - (MAX_TIME_SCROLLBAR_VALUE - MIN_TIME_SCROLLBAR_VALUE) * 0.01f) {
        activePlot->setAutoScroll(true);
    } else {
        activePlot->setAutoScroll(false);
        quint64 rightPosition;
        /* If the data exceeds the plot window, choose the position according to the scrollbar position */
        if(activePlot->getDataInterval() > activePlot->getPlotInterval()) {
            scrollInterval = activePlot->getDataInterval() - activePlot->getPlotInterval();
            rightPosition = activePlot->getMinTime() + activePlot->getPlotInterval() + (scrollInterval * position);
        } else {
            /* If the data interval is smaller as the plot interval, clamp the scrollbar to the right */
            rightPosition = activePlot->getMinTime() + activePlot->getPlotInterval();
        }
        emit plotWindowPositionUpdated(rightPosition);
    }


    // The slider position must be mapped onto an interval of datainterval - plotinterval,
    // because the slider position defines the right edge of the plot window. The leftmost
    // slider position must therefore map to the start of the data interval + plot interval
    // to ensure that the plot is not empty

    //  start> |-- plot interval --||-- (data interval - plotinterval) --| <end

    //@TODO Add notification of scrollbar here
    //plot->setWindowPosition(rightPosition);

    plotWindowLock.unlock();
}

/**
 * @brief Receive an updated plot window position.
 * The plot window can be changed by the arrival of new data or by
 * other user interaction. The scrollbar and other UI components
 * can be notified by calling this method.
 *
 * @param position The absolute position of the right edge of the plot window, in milliseconds
 **/
void LinechartWidget::setPlotWindowPosition(quint64 position) {
    plotWindowLock.lockForWrite();
    // Calculate the relative position
    double pos;

    // A relative position makes only sense if the plot is filled
    if(activePlot->getDataInterval() > activePlot->getPlotInterval()) {
        //TODO @todo Implement the scrollbar enabling in a more elegant way
        scrollbar->setDisabled(false);
        quint64 scrollInterval = position - activePlot->getMinTime() - activePlot->getPlotInterval();



        pos = (static_cast<double>(scrollInterval) / (activePlot->getDataInterval() - activePlot->getPlotInterval()));
    } else {
        scrollbar->setDisabled(true);
        pos = 1;
    }
    plotWindowLock.unlock();

    emit plotWindowPositionUpdated(static_cast<int>(pos * (MAX_TIME_SCROLLBAR_VALUE - MIN_TIME_SCROLLBAR_VALUE)));
}

/**
 * @brief Set the time interval the plot displays.
 * The time interval of the plot can be adjusted by this method. If the
 * data covers less time than the interval, the plot will be filled from
 * the right to left
 *
 * @param interval The time interval to plot
 **/
void LinechartWidget::setPlotInterval(quint64 interval) {
    activePlot->setPlotInterval(interval);
}

/**
 * @brief Take the click of a curve activation / deactivation button.
 * This method allows to map a button to a plot curve.The text of the
 * button must equal the curve name to activate / deactivate.
 *
 * @param checked The visibility of the curve: true to display the curve, false otherwise
 **/
void LinechartWidget::takeButtonClick(bool checked) {

    QCheckBox* button = qobject_cast<QCheckBox*>(QObject::sender());

    if(button != NULL)
    {
        activePlot->setVisible(button->objectName(), checked);
    }
}

/**
 * @brief Factory method to create a new button.
 *
 * @param imagename The name of the image (should be placed at the standard icon location)
 * @param text The button text
 * @param parent The parent object (to ensure that the memory is freed after the deletion of the button)
 **/
QToolButton* LinechartWidget::createButton(QWidget* parent) {
    QToolButton* button = new QToolButton(parent);
    button->setMinimumSize(QSize(20, 20));
    button->setMaximumSize(60, 20);
    button->setGeometry(button->x(), button->y(), 20, 20);
    return button;
}
