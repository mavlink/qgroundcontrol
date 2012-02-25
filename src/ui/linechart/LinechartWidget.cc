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
#include <QSizePolicy>
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
#include "MainWindow.h"
#include "QGC.h"
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
    curveVariances(new QMap<QString, QLabel*>()),
    curveMenu(new QMenu(this)),
    logFile(new QFile()),
    logindex(1),
    logging(false),
    logStartTime(0),
    updateTimer(new QTimer()),
    selectedMAV(-1)
{
    // Add elements defined in Qt Designer
    ui.setupUi(this);
    this->setMinimumSize(200, 150);

    // Add and customize curve list elements (left side)
    curvesWidget = new QWidget(ui.curveListWidget);
    ui.curveListWidget->setWidget(curvesWidget);
    curvesWidgetLayout = new QGridLayout(curvesWidget);
    curvesWidgetLayout->setMargin(2);
    curvesWidgetLayout->setSpacing(4);
    //curvesWidgetLayout->setSizeConstraint(QSizePolicy::Expanding);
    curvesWidgetLayout->setAlignment(Qt::AlignTop);

    curvesWidgetLayout->setColumnStretch(0, 0);
    curvesWidgetLayout->setColumnStretch(1, 0);
    curvesWidgetLayout->setColumnStretch(2, 80);
    curvesWidgetLayout->setColumnStretch(3, 50);
    curvesWidgetLayout->setColumnStretch(4, 50);
    curvesWidgetLayout->setColumnStretch(5, 50);
//    horizontalLayout->setColumnStretch(median, 50);
    curvesWidgetLayout->setColumnStretch(6, 50);

    curvesWidget->setLayout(curvesWidgetLayout);

    // Create curve list headings
    QLabel* label;
    QLabel* value;
    QLabel* mean;
    QLabel* variance;

    connect(ui.recolorButton, SIGNAL(clicked()), this, SLOT(recolor()));
    connect(ui.shortNameCheckBox, SIGNAL(clicked(bool)), this, SLOT(setShortNames(bool)));

    int labelRow = curvesWidgetLayout->rowCount();

    selectAllCheckBox = new QCheckBox("", this);
    connect(selectAllCheckBox, SIGNAL(clicked(bool)), this, SLOT(selectAllCurves(bool)));
    curvesWidgetLayout->addWidget(selectAllCheckBox, labelRow, 0, 1, 2);

    label = new QLabel(this);
    label->setText("Name");
    curvesWidgetLayout->addWidget(label, labelRow, 2);

    // Value
    value = new QLabel(this);
    value->setText("Val");
    curvesWidgetLayout->addWidget(value, labelRow, 3);

    // Unit
    //curvesWidgetLayout->addWidget(new QLabel(tr("Unit")), labelRow, 4);

    // Mean
    mean = new QLabel(this);
    mean->setText("Mean");
    curvesWidgetLayout->addWidget(mean, labelRow, 5);

    // Variance
    variance = new QLabel(this);
    variance->setText("Variance");
    curvesWidgetLayout->addWidget(variance, labelRow, 6);

    // Add and customize plot elements (right side)

    // Create the layout
    createLayout();

    // Add the last actions
    //connect(this, SIGNAL(plotWindowPositionUpdated(int)), scrollbar, SLOT(setValue(int)));
    //connect(scrollbar, SIGNAL(sliderMoved(int)), this, SLOT(setPlotWindowPosition(int)));

    updateTimer->setInterval(updateInterval);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    connect(ui.uasSelectionBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectActiveSystem(int)));
    readSettings();
}

LinechartWidget::~LinechartWidget()
{
    writeSettings();
    stopLogging();
    if (activePlot) delete activePlot;
    activePlot = NULL;
    delete listedCurves;
    listedCurves = NULL;
}

void LinechartWidget::selectActiveSystem(int mav)
{
    // -1: Unitialized, 0: all
    if (mav != selectedMAV && (selectedMAV != -1))
    {
        // Delete all curves
        // FIXME
    }
    selectedMAV = mav;
}

void LinechartWidget::selectAllCurves(bool all)
{
    QMap<QString, QLabel*>::iterator i;
    for (i = curveLabels->begin(); i != curveLabels->end(); ++i) {
        activePlot->setVisible(i.key(), all);
    }
}

void LinechartWidget::writeSettings()
{
    QSettings settings;
    settings.beginGroup("LINECHART");
    if (timeButton) settings.setValue("ENFORCE_GROUNDTIME", timeButton->isChecked());
    if (unitsCheckBox) settings.setValue("SHOW_UNITS", unitsCheckBox->isChecked());
    if (ui.shortNameCheckBox) settings.setValue("SHORT_NAMES", ui.shortNameCheckBox->isChecked());
    settings.endGroup();
    settings.sync();
}

void LinechartWidget::readSettings()
{
    QSettings settings;
    settings.sync();
    settings.beginGroup("LINECHART");
    if (activePlot) {
        timeButton->setChecked(settings.value("ENFORCE_GROUNDTIME", timeButton->isChecked()).toBool());
        activePlot->enforceGroundTime(settings.value("ENFORCE_GROUNDTIME", timeButton->isChecked()).toBool());
        timeButton->setChecked(settings.value("ENFORCE_GROUNDTIME", timeButton->isChecked()).toBool());
    }
    if (unitsCheckBox) unitsCheckBox->setChecked(settings.value("SHOW_UNITS", unitsCheckBox->isChecked()).toBool());
    if (ui.shortNameCheckBox) ui.shortNameCheckBox->setChecked(settings.value("SHORT_NAMES", ui.shortNameCheckBox->isChecked()).toBool());
    settings.endGroup();
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
    layout->setRowStretch(1, 1);

    // Linear scaling button
    scalingLinearButton = createButton(this);
    scalingLinearButton->setDefaultAction(setScalingLinear);
    scalingLinearButton->setCheckable(true);
    scalingLinearButton->setToolTip(tr("Set linear scale for Y axis"));
    scalingLinearButton->setWhatsThis(tr("Set linear scale for Y axis"));
    layout->addWidget(scalingLinearButton, 1, 0);
    layout->setColumnStretch(0, 0);

    // Logarithmic scaling button
    scalingLogButton = createButton(this);
    scalingLogButton->setDefaultAction(setScalingLogarithmic);
    scalingLogButton->setCheckable(true);
    scalingLogButton->setToolTip(tr("Set logarithmic scale for Y axis"));
    scalingLogButton->setWhatsThis(tr("Set logarithmic scale for Y axis"));
    layout->addWidget(scalingLogButton, 1, 1);
    layout->setColumnStretch(1, 0);

    // Averaging spin box
    averageSpinBox = new QSpinBox(this);
    averageSpinBox->setToolTip(tr("Sliding window size to calculate mean and variance"));
    averageSpinBox->setWhatsThis(tr("Sliding window size to calculate mean and variance"));
    averageSpinBox->setMinimum(2);
    averageSpinBox->setValue(200);
    setAverageWindow(200);
    averageSpinBox->setMaximum(9999);
    layout->addWidget(averageSpinBox, 1, 2);
    layout->setColumnStretch(2, 0);
    connect(averageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAverageWindow(int)));

    // Log Button
    logButton = new QToolButton(this);
    logButton->setToolTip(tr("Start to log curve data into a CSV or TXT file"));
    logButton->setWhatsThis(tr("Start to log curve data into a CSV or TXT file"));
    logButton->setText(tr("Start Logging"));
    layout->addWidget(logButton, 1, 3);
    layout->setColumnStretch(3, 0);
    connect(logButton, SIGNAL(clicked()), this, SLOT(startLogging()));

    // Ground time button
    timeButton = new QCheckBox(this);
    timeButton->setText(tr("Ground Time"));
    timeButton->setToolTip(tr("Overwrite timestamp of data from vehicle with ground receive time. Helps if the plots are not visible because of missing or invalid onboard time."));
    timeButton->setWhatsThis(tr("Overwrite timestamp of data from vehicle with ground receive time. Helps if the plots are not visible because of missing or invalid onboard time."));
    layout->addWidget(timeButton, 1, 4);
    layout->setColumnStretch(4, 0);
    connect(timeButton, SIGNAL(clicked(bool)), activePlot, SLOT(enforceGroundTime(bool)));
    connect(timeButton, SIGNAL(clicked()), this, SLOT(writeSettings()));

    unitsCheckBox = new QCheckBox(this);
    unitsCheckBox->setText(tr("Show units"));
    unitsCheckBox->setChecked(true);
    unitsCheckBox->setToolTip(tr("Enable unit display in curve list"));
    unitsCheckBox->setWhatsThis(tr("Enable unit display in curve list"));
    layout->addWidget(unitsCheckBox, 1, 5);
    connect(unitsCheckBox, SIGNAL(clicked()), this, SLOT(writeSettings()));

    ui.diagramGroupBox->setLayout(layout);

    // Add actions
    averageSpinBox->setValue(activePlot->getAverageWindow());

    // Connect notifications from the user interface to the plot
    connect(this, SIGNAL(curveRemoved(QString)), activePlot, SLOT(hideCurve(QString)));

    // Update scrollbar when plot window changes (via translator method setPlotWindowPosition()
//    connect(activePlot, SIGNAL(windowPositionChanged(quint64)), this, SLOT(setPlotWindowPosition(quint64)));
    connect(activePlot, SIGNAL(curveRemoved(QString)), this, SLOT(removeCurve(QString)));

    // Update plot when scrollbar is moved (via translator method setPlotWindowPosition()
    connect(this, SIGNAL(plotWindowPositionUpdated(quint64)), activePlot, SLOT(setWindowPosition(quint64)));

    // Set scaling
    connect(scalingLinearButton, SIGNAL(clicked()), activePlot, SLOT(setLinearScaling()));
    connect(scalingLogButton, SIGNAL(clicked()), activePlot, SLOT(setLogarithmicScaling()));
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, qint8 value, quint64 usec)
{
    appendData(uasId, curve, unit, static_cast<qint64>(value), usec);
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, quint8 value, quint64 usec)
{
    appendData(uasId, curve, unit, static_cast<quint64>(value), usec);
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, qint16 value, quint64 usec)
{
    appendData(uasId, curve, unit, static_cast<qint64>(value), usec);
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, quint16 value, quint64 usec)
{
    appendData(uasId, curve, unit, static_cast<quint64>(value), usec);
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, qint32 value, quint64 usec)
{
    appendData(uasId, curve, unit, static_cast<qint64>(value), usec);
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, quint32 value, quint64 usec)
{
    appendData(uasId, curve, unit, static_cast<quint64>(value), usec);
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, qint64 value, quint64 usec)
{
    if ((selectedMAV == -1 && isVisible()) || (selectedMAV == uasId && isVisible()))
    {
        // Order matters here, first append to plot, then update curve list
        activePlot->appendData(curve+unit, usec, value);
        // Store data
        QLabel* label = curveLabels->value(curve+unit, NULL);
        // Make sure the curve will be created if it does not yet exist
        if(!label)
        {
            intData.insert(curve+unit, 0);
            addCurve(curve, unit);
        }

        // Add int data
        intData.insert(curve+unit, value);
    }

    // Log data
    if (logging)
    {
        if (activePlot->isVisible(curve+unit))
        {
            if (usec == 0) usec = QGC::groundTimeMilliseconds();
            if (logStartTime == 0) logStartTime = usec;
            qint64 time = usec - logStartTime;
            if (time < 0) time = 0;

            logFile->write(QString(QString::number(time) + "\t" + QString::number(uasId) + "\t" + curve + "\t" + QString::number(value) + "\n").toLatin1());
            logFile->flush();
        }
    }
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, quint64 value, quint64 usec)
{
    if ((selectedMAV == -1 && isVisible()) || (selectedMAV == uasId && isVisible()))
    {
        // Order matters here, first append to plot, then update curve list
        activePlot->appendData(curve+unit, usec, value);
        // Store data
        QLabel* label = curveLabels->value(curve+unit, NULL);
        // Make sure the curve will be created if it does not yet exist
        if(!label)
        {
            intData.insert(curve+unit, 0);
            addCurve(curve, unit);
        }

        // Add int data
        intData.insert(curve+unit, value);
    }

    // Log data
    if (logging)
    {
        if (activePlot->isVisible(curve+unit))
        {
            if (usec == 0) usec = QGC::groundTimeMilliseconds();
            if (logStartTime == 0) logStartTime = usec;
            qint64 time = usec - logStartTime;
            if (time < 0) time = 0;

            logFile->write(QString(QString::number(time) + "\t" + QString::number(uasId) + "\t" + curve + "\t" + QString::number(value) + "\n").toLatin1());
            logFile->flush();
        }
    }
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, double value, quint64 usec)
{
    if ((selectedMAV == -1 && isVisible()) || (selectedMAV == uasId && isVisible()))
    {
        // Order matters here, first append to plot, then update curve list
        activePlot->appendData(curve+unit, usec, value);
        // Store data
        QLabel* label = curveLabels->value(curve+unit, NULL);
        // Make sure the curve will be created if it does not yet exist
        if(!label)
        {
            //qDebug() << "ADDING CURVE IN APPENDDATE DOUBLE";
            addCurve(curve, unit);
        }
    }

    // Log data
    if (logging)
    {
        if (activePlot->isVisible(curve+unit))
        {
            if (usec == 0) usec = QGC::groundTimeMilliseconds();
            if (logStartTime == 0) logStartTime = usec;
            qint64 time = usec - logStartTime;
            if (time < 0) time = 0;

            logFile->write(QString(QString::number(time) + "\t" + QString::number(uasId) + "\t" + curve + "\t" + QString::number(value,'g',18) + "\n").toLatin1());
            logFile->flush();
        }
    }
}

void LinechartWidget::refresh()
{
    setUpdatesEnabled(false);
    QString str;
    // Value
    QMap<QString, QLabel*>::iterator i;
    for (i = curveLabels->begin(); i != curveLabels->end(); ++i) {
        if (intData.contains(i.key())) {
            str.sprintf("% 11i", intData.value(i.key()));
        } else {
            double val = activePlot->getCurrentValue(i.key());
            int intval = static_cast<int>(val);
            if (intval >= 100000 || intval <= -100000) {
                str.sprintf("% 11i", intval);
            } else if (intval >= 10000 || intval <= -10000) {
                str.sprintf("% 11.2f", val);
            } else if (intval >= 1000 || intval <= -1000) {
                str.sprintf("% 11.4f", val);
            } else {
                str.sprintf("% 11.6f", val);
            }
        }
        // Value
        i.value()->setText(str);
    }
    // Mean
    QMap<QString, QLabel*>::iterator j;
    for (j = curveMeans->begin(); j != curveMeans->end(); ++j) {
        double val = activePlot->getMean(j.key());
        int intval = static_cast<int>(val);
        if (intval >= 100000 || intval <= -100000) {
            str.sprintf("% 11i", intval);
        } else if (intval >= 10000 || intval <= -10000) {
            str.sprintf("% 11.2f", val);
        } else if (intval >= 1000 || intval <= -1000) {
            str.sprintf("% 11.4f", val);
        } else {
            str.sprintf("% 11.6f", val);
        }
        j.value()->setText(str);
    }
//    QMap<QString, QLabel*>::iterator k;
//    for (k = curveMedians->begin(); k != curveMedians->end(); ++k)
//    {
//        // Median
//        str.sprintf("%+.2f", activePlot->getMedian(k.key()));
//        k.value()->setText(str);
//    }
    QMap<QString, QLabel*>::iterator l;
    for (l = curveVariances->begin(); l != curveVariances->end(); ++l) {
        // Variance
        str.sprintf("% 8.3e", activePlot->getVariance(l.key()));
        l.value()->setText(str);
    }
    setUpdatesEnabled(true);
}


void LinechartWidget::startLogging()
{
    // Store reference to file
    // Append correct file ending if needed
    bool abort = false;

    // Check if any curve is enabled
    if (!activePlot->anyCurveVisible()) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("No curves selected for logging.");
        msgBox.setInformativeText("Please check all curves you want to log. Currently no data would be logged, aborting the logging.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    // Let user select the log file name
    //QDate date(QDate::currentDate());
    // QString("./pixhawk-log-" + date.toString("yyyy-MM-dd") + "-" + QString::number(logindex) + ".log")
    QString fileName = QFileDialog::getSaveFileName(this, tr("Specify log file name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("Logfile (*.csv *.txt);;"));

    while (!(fileName.endsWith(".txt") || fileName.endsWith(".csv")) && !abort && fileName != "") {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Unsuitable file extension for logfile");
        msgBox.setInformativeText("Please choose .txt or .csv as file extension. Click OK to change the file extension, cancel to not start logging.");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        if(msgBox.exec() != QMessageBox::Ok)
        {
            abort = true;
            break;
        }
        fileName = QFileDialog::getSaveFileName(this, tr("Specify log file name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("Logfile (*.txt *.csv);;"));
    }

    qDebug() << "SAVE FILE" << fileName;

    // Check if the user did not abort the file save dialog
    if (!abort && fileName != "") {
        logFile = new QFile(fileName);
        if (logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            logging = true;
            logStartTime = 0;
            curvesWidget->setEnabled(false);
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
    curvesWidget->setEnabled(true);
    if (logFile->isOpen()) {
        logFile->flush();
        logFile->close();
        // Postprocess log file
        compressor = new LogCompressor(logFile->fileName(), logFile->fileName());
        connect(compressor, SIGNAL(finishedFile(QString)), this, SIGNAL(logfileWritten(QString)));
        connect(compressor, SIGNAL(logProcessingStatusChanged(QString)), MainWindow::instance(), SLOT(showStatusMessage(QString)));

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(tr("Starting Log Compression"));
        msgBox.setInformativeText(tr("Should empty fields (e.g. due to packet drops) be filled with the previous value of the same variable (zero order hold)?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        bool fill;
        if (ret == QMessageBox::Yes)
        {
            fill = true;
        }
        else
        {
            fill = false;
        }

        compressor->startCompression(fill);
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
void LinechartWidget::addCurve(const QString& curve, const QString& unit)
{
    LinechartPlot* plot = activePlot;
//    QHBoxLayout *horizontalLayout;
    QCheckBox *checkBox;
    QLabel* label;
    QLabel* value;
    QLabel* unitLabel;
    QLabel* mean;
    QLabel* variance;

    curveNames.insert(curve+unit, curve);

    int labelRow = curvesWidgetLayout->rowCount();

    checkBox = new QCheckBox(this);
    checkBox->setCheckable(true);
    checkBox->setObjectName(curve+unit);
    checkBox->setToolTip(tr("Enable the curve in the graph window"));
    checkBox->setWhatsThis(tr("Enable the curve in the graph window"));

    curvesWidgetLayout->addWidget(checkBox, labelRow, 0);

    QWidget* colorIcon = new QWidget(this);
    colorIcons.insert(curve+unit, colorIcon);
    colorIcon->setMinimumSize(QSize(5, 14));
    colorIcon->setMaximumSize(4, 14);

    curvesWidgetLayout->addWidget(colorIcon, labelRow, 1);

    label = new QLabel(this);
    curvesWidgetLayout->addWidget(label, labelRow, 2);

    //checkBox->setText(QString());
    label->setText(getCurveName(curve+unit, ui.shortNameCheckBox->isChecked()));
    QColor color(Qt::gray);// = plot->getColorForCurve(curve+unit);
    QString colorstyle;
    colorstyle = colorstyle.sprintf("QWidget { background-color: #%X%X%X; }", color.red(), color.green(), color.blue());
    colorIcon->setStyleSheet(colorstyle);
    colorIcon->setAutoFillBackground(true);

    // Label
    curveNameLabels.insert(curve+unit, label);

    // Value
    value = new QLabel(this);
    value->setNum(0.00);
    value->setStyleSheet(QString("QLabel {font-family:\"Courier\"; font-weight: bold;}"));
    value->setToolTip(tr("Current value of %1 in %2 units").arg(curve, unit));
    value->setWhatsThis(tr("Current value of %1 in %2 units").arg(curve, unit));
    curveLabels->insert(curve+unit, value);
    curvesWidgetLayout->addWidget(value, labelRow, 3);

    // Unit
    unitLabel = new QLabel(this);
    unitLabel->setText(unit);
    unitLabel->setStyleSheet(QString("QLabel {color: %1;}").arg("#AAAAAA"));
    //qDebug() << "UNIT" << unit;
    unitLabel->setToolTip(tr("Unit of ") + curve);
    unitLabel->setWhatsThis(tr("Unit of ") + curve);
    curvesWidgetLayout->addWidget(unitLabel, labelRow, 4);
    unitLabel->setVisible(unitsCheckBox->isChecked());
    connect(unitsCheckBox, SIGNAL(clicked(bool)), unitLabel, SLOT(setVisible(bool)));

    // Mean
    mean = new QLabel(this);
    mean->setNum(0.00);
    mean->setStyleSheet(QString("QLabel {font-family:\"Courier\"; font-weight: bold;}"));
    mean->setToolTip(tr("Arithmetic mean of %1 in %2 units").arg(curve, unit));
    mean->setWhatsThis(tr("Arithmetic mean of %1 in %2 units").arg(curve, unit));
    curveMeans->insert(curve+unit, mean);
    curvesWidgetLayout->addWidget(mean, labelRow, 5);

//    // Median
//    median = new QLabel(form);
//    value->setNum(0.00);
//    curveMedians->insert(curve, median);
//    horizontalLayout->addWidget(median);

    // Variance
    variance = new QLabel(this);
    variance->setNum(0.00);
    variance->setStyleSheet(QString("QLabel {font-family:\"Courier\"; font-weight: bold;}"));
    variance->setToolTip(tr("Variance of %1 in (%2)^2 units").arg(curve, unit));
    variance->setWhatsThis(tr("Variance of %1 in (%2)^2 units").arg(curve, unit));
    curveVariances->insert(curve+unit, variance);
    curvesWidgetLayout->addWidget(variance, labelRow, 6);

    /* Color picker
    QColor color = QColorDialog::getColor(Qt::green, this);
         if (color.isValid()) {
             colorLabel->setText(color.name());
             colorLabel->setPalette(QPalette(color));
             colorLabel->setAutoFillBackground(true);
         }
        */

    // Set stretch factors so that the label gets the whole space


    // Load visibility settings
    // TODO

    // Connect actions
    connect(selectAllCheckBox, SIGNAL(clicked(bool)), checkBox, SLOT(setChecked(bool)));
    QObject::connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(takeButtonClick(bool)));
    QObject::connect(this, SIGNAL(curveVisible(QString, bool)), plot, SLOT(setVisible(QString, bool)));

    // Set UI components to initial state
    checkBox->setChecked(false);
    plot->setVisible(curve+unit, false);
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

    QWidget* widget = NULL;
    widget = curveLabels->take(curve);
    curvesWidgetLayout->removeWidget(widget);
    widget->deleteLater();
    widget = curveMeans->take(curve);
    curvesWidgetLayout->removeWidget(widget);
    widget->deleteLater();
    widget = curveMedians->take(curve);
    curvesWidgetLayout->removeWidget(widget);
    widget->deleteLater();
    widget = curveVariances->take(curve);
    curvesWidgetLayout->removeWidget(widget);
    widget->deleteLater();
//    widget = colorIcons->take(curve);
//    curvesWidgetLayout->removeWidget(colorIcons->take(curve));
    widget->deleteLater();
//    intData->remove(curve);
}

void LinechartWidget::recolor()
{
    activePlot->shuffleColors();

    foreach (QString key, colorIcons.keys())
    {

        // FIXME
//        if (activePlot)
        QString colorstyle;
        QColor color = activePlot->getColorForCurve(key);
        colorstyle = colorstyle.sprintf("QWidget { background-color: #%X%X%X; }", color.red(), color.green(), color.blue());
        QWidget* colorIcon = colorIcons.value(key, 0);
        if (colorIcon)
        {
            colorIcon->setStyleSheet(colorstyle);
            colorIcon->setAutoFillBackground(true);
        }
    }
}

QString LinechartWidget::getCurveName(const QString& key, bool shortEnabled)
{
    if (shortEnabled)
    {
        QString name;
        QStringList parts = curveNames.value(key).split(".");
        if (parts.length() > 1)
        {
            name = parts.at(1);
        }
        else
        {
            name = parts.at(0);
        }

        const int sizeLimit = 20;

        // Replace known words with abbreviations
        if (name.length() > sizeLimit)
        {
            name.replace("gyroscope", "gyro");
            name.replace("accelerometer", "acc");
            name.replace("magnetometer", "mag");
            name.replace("distance", "dist");
            name.replace("ailerons", "ail");
            name.replace("altitude", "alt");
            name.replace("waypoint", "wp");
            name.replace("throttle", "thr");
            name.replace("elevator", "elev");
            name.replace("rudder", "rud");
            name.replace("error", "err");
            name.replace("version", "ver");
            name.replace("message", "msg");
            name.replace("count", "cnt");
            name.replace("value", "val");
            name.replace("source", "src");
            name.replace("index", "idx");
            name.replace("type", "typ");
            name.replace("mode", "mod");
        }

        // Check if sub-part is still exceeding N chars
        if (name.length() > sizeLimit)
        {
            name.replace("a", "");
            name.replace("e", "");
            name.replace("i", "");
            name.replace("o", "");
            name.replace("u", "");
        }

        return name;
    }
    else
    {
        return curveNames.value(key);
    }
}

void LinechartWidget::setShortNames(bool enable)
{
    foreach (QString key, curveNames.keys())
    {
        curveNameLabels.value(key)->setText(getCurveName(key, enable));
    }
}

void LinechartWidget::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    setActive(true);
}

void LinechartWidget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    setActive(false);
}

void LinechartWidget::setActive(bool active)
{
    if (activePlot) {
        activePlot->setActive(active);
    }
    if (active) {
        updateTimer->start(updateInterval);
    } else {
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
void LinechartWidget::setPlotWindowPosition(int scrollBarValue)
{
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
void LinechartWidget::setPlotWindowPosition(quint64 position)
{
    plotWindowLock.lockForWrite();
    // Calculate the relative position
    double pos;

    // A relative position makes only sense if the plot is filled
    if(activePlot->getDataInterval() > activePlot->getPlotInterval()) {
        //TODO @todo Implement the scrollbar enabling in a more elegant way
        //scrollbar->setDisabled(false);
        quint64 scrollInterval = position - activePlot->getMinTime() - activePlot->getPlotInterval();



        pos = (static_cast<double>(scrollInterval) / (activePlot->getDataInterval() - activePlot->getPlotInterval()));
    } else {
        //scrollbar->setDisabled(true);
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
void LinechartWidget::setPlotInterval(quint64 interval)
{
    activePlot->setPlotInterval(interval);
}

/**
 * @brief Take the click of a curve activation / deactivation button.
 * This method allows to map a button to a plot curve.The text of the
 * button must equal the curve name to activate / deactivate.
 *
 * @param checked The visibility of the curve: true to display the curve, false otherwise
 **/
void LinechartWidget::takeButtonClick(bool checked)
{

    QCheckBox* button = qobject_cast<QCheckBox*>(QObject::sender());

    if(button != NULL)
    {
        activePlot->setVisible(button->objectName(), checked);

        QColor color = activePlot->getColorForCurve(button->objectName());
        if(color.isValid())
        {
            QString colorstyle;
            colorstyle = colorstyle.sprintf("QWidget { background-color: #%X%X%X; }", color.red(), color.green(), color.blue());
            QWidget* colorIcon = colorIcons.value(button->objectName(), 0);
            if (colorIcon)
            {
                colorIcon->setStyleSheet(colorstyle);
                colorIcon->setAutoFillBackground(true);
            }
        }
    }
}

/**
 * @brief Factory method to create a new button.
 *
 * @param imagename The name of the image (should be placed at the standard icon location)
 * @param text The button text
 * @param parent The parent object (to ensure that the memory is freed after the deletion of the button)
 **/
QToolButton* LinechartWidget::createButton(QWidget* parent)
{
    QToolButton* button = new QToolButton(parent);
    button->setMinimumSize(QSize(20, 20));
    button->setMaximumSize(60, 20);
    button->setGeometry(button->x(), button->y(), 20, 20);
    return button;
}
