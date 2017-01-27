/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Line chart plot widget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Thomas Gubler <thomasgubler@student.ethz.ch>
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
#include <QStandardPaths>
#include <QShortcut>

#include "LinechartWidget.h"
#include "LinechartPlot.h"
#include "LogCompressor.h"
#include "QGC.h"
#include "MG.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"
#include "QGCApplication.h"

LinechartWidget::LinechartWidget(int systemid, QWidget *parent) : QWidget(parent),
    sysid(systemid),
    activePlot(NULL),
    curvesLock(new QReadWriteLock()),
    plotWindowLock(),
    curveListIndex(0),
    curveListCounter(0),
    curveLabels(new QMap<QString, QLabel*>()),
    curveMeans(new QMap<QString, QLabel*>()),
    curveMedians(new QMap<QString, QLabel*>()),
    curveVariances(new QMap<QString, QLabel*>()),
    logFile(new QFile()),
    logindex(1),
    logging(false),
    logStartTime(0),
    updateTimer(new QTimer()),
    selectedMAV(-1),
    lastTimestamp(0)
{
    // Add elements defined in Qt Designer
    ui.setupUi(this);
    this->setMinimumSize(600, 400);

    // Add and customize curve list elements (left side)
    curvesWidget = new QWidget(ui.curveListWidget);
    ui.curveListWidget->setWidget(curvesWidget);
    curvesWidgetLayout = new QGridLayout(curvesWidget);
    curvesWidgetLayout->setMargin(6);
    curvesWidgetLayout->setSpacing(6);
    curvesWidgetLayout->setAlignment(Qt::AlignTop);
    curvesWidgetLayout->setColumnMinimumWidth(0, 10);

    curvesWidgetLayout->setColumnStretch(0, 0);
    curvesWidgetLayout->setColumnStretch(1, 10);
    curvesWidgetLayout->setColumnStretch(2, 80);
    curvesWidgetLayout->setColumnStretch(3, 50);
    curvesWidgetLayout->setColumnStretch(4, 50);
    curvesWidgetLayout->setColumnStretch(5, 50);
    curvesWidgetLayout->setColumnStretch(6, 50);

    curvesWidget->setLayout(curvesWidgetLayout);

    // Create curve list headings
    connect(ui.recolorButton, &QPushButton::clicked, this, &LinechartWidget::recolor);
    connect(ui.shortNameCheckBox, &QCheckBox::clicked, this, &LinechartWidget::setShortNames);
    connect(ui.plotFilterLineEdit, &QLineEdit::textChanged, this, &LinechartWidget::filterCurves);
    QShortcut *shortcut  = new QShortcut(this);
    shortcut->setKey(QKeySequence(Qt::CTRL + Qt::Key_F));
    connect(shortcut, &QShortcut::activated, this, &LinechartWidget::setPlotFilterLineEditFocus);

    int labelRow = curvesWidgetLayout->rowCount();

    selectAllCheckBox = new QCheckBox(this);
    connect(selectAllCheckBox, &QCheckBox::clicked, this, &LinechartWidget::selectAllCurves);
    curvesWidgetLayout->addWidget(selectAllCheckBox, labelRow, 0);

    QWidget* colorIcon = new QWidget(this);
    colorIcon->setMinimumSize(QSize(5, 14));
    colorIcon->setMaximumSize(QSize(5, 14));
    curvesWidgetLayout->addWidget(colorIcon, labelRow, 1);

    curvesWidgetLayout->addWidget(new QLabel(tr("Name")),     labelRow, 2);
    curvesWidgetLayout->addWidget(new QLabel(tr("Val")),      labelRow, 3, Qt::AlignRight);

    QLabel* pUnit = new QLabel(tr("Unit"));
    curvesWidgetLayout->addWidget(pUnit,                      labelRow, 4);

    curvesWidgetLayout->addWidget(new QLabel(tr("Mean")),     labelRow, 5, Qt::AlignRight);
    curvesWidgetLayout->addWidget(new QLabel(tr("Variance")), labelRow, 6, Qt::AlignRight);


    // Create the layout
    createLayout();

    // And make sure we're listening for future style changes
    connect(qgcApp(), &QGCApplication::styleChanged, this, &LinechartWidget::recolor);

    updateTimer->setInterval(updateInterval);
    connect(updateTimer, &QTimer::timeout, this, &LinechartWidget::refresh);
    connect(ui.uasSelectionBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LinechartWidget::selectActiveSystem);

    readSettings();
    pUnit->setVisible(ui.showUnitsCheckBox->isChecked());
    connect(ui.showUnitsCheckBox, &QCheckBox::clicked, pUnit, &QLabel::setVisible);
}

LinechartWidget::~LinechartWidget()
{
    writeSettings();
    stopLogging();
    if (activePlot) delete activePlot;
    activePlot = NULL;
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
        activePlot->setVisibleById(i.key(), all);
    }
}

void LinechartWidget::writeSettings()
{
    QSettings settings;
    settings.beginGroup("LINECHART");
    bool enforceGT = (!autoGroundTimeSet && timeButton->isChecked()) ? true : false;
    if (timeButton) settings.setValue("ENFORCE_GROUNDTIME", enforceGT);
    if (ui.showUnitsCheckBox) settings.setValue("SHOW_UNITS", ui.showUnitsCheckBox->isChecked());
    if (ui.shortNameCheckBox) settings.setValue("SHORT_NAMES", ui.shortNameCheckBox->isChecked());
    settings.endGroup();
}

void LinechartWidget::readSettings()
{
    QSettings settings;
    settings.beginGroup("LINECHART");
    if (activePlot) {
        timeButton->setChecked(settings.value("ENFORCE_GROUNDTIME", timeButton->isChecked()).toBool());
        activePlot->enforceGroundTime(settings.value("ENFORCE_GROUNDTIME", timeButton->isChecked()).toBool());
        timeButton->setChecked(settings.value("ENFORCE_GROUNDTIME", timeButton->isChecked()).toBool());
        //userGroundTimeSet = settings.value("USER_GROUNDTIME", timeButton->isChecked()).toBool();
    }
    if (ui.showUnitsCheckBox) ui.showUnitsCheckBox->setChecked(settings.value("SHOW_UNITS", ui.showUnitsCheckBox->isChecked()).toBool());
    if (ui.shortNameCheckBox) ui.shortNameCheckBox->setChecked(settings.value("SHORT_NAMES", ui.shortNameCheckBox->isChecked()).toBool());
    settings.endGroup();
}

void LinechartWidget::createLayout()
{
    // Create actions
    createActions();

    // Setup the plot group box area layout
    QVBoxLayout* vlayout = new QVBoxLayout(ui.diagramGroupBox);
    vlayout->setSpacing(4);
    vlayout->setMargin(2);

    // Create plot container widget
    activePlot = new LinechartPlot(this, sysid);
    // Activate automatic scrolling
    activePlot->setAutoScroll(true);

    // TODO Proper Initialization needed
    //    activePlot = getPlot(0);
    //    plotContainer->setPlot(activePlot);

    vlayout->addWidget(activePlot);

    QHBoxLayout *hlayout = new QHBoxLayout;
    vlayout->addLayout(hlayout);

    // Logarithmic scaling button
    scalingLogButton = createButton(this);
    scalingLogButton->setText(tr("LOG"));
    scalingLogButton->setCheckable(true);
    scalingLogButton->setToolTip(tr("Set logarithmic scale for Y axis"));
    scalingLogButton->setWhatsThis(tr("Set logarithmic scale for Y axis"));
    hlayout->addWidget(scalingLogButton);

    // Averaging spin box
    averageSpinBox = new QSpinBox(this);
    averageSpinBox->setToolTip(tr("Sliding window size to calculate mean and variance"));
    averageSpinBox->setWhatsThis(tr("Sliding window size to calculate mean and variance"));
    averageSpinBox->setMinimum(2);
    averageSpinBox->setValue(200);
    setAverageWindow(200);
    averageSpinBox->setMaximum(9999);
    hlayout->addWidget(averageSpinBox);
    connect(averageSpinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LinechartWidget::setAverageWindow);

    // Log Button
    logButton = new QToolButton(this);
    logButton->setToolTip(tr("Start to log curve data into a CSV or TXT file"));
    logButton->setWhatsThis(tr("Start to log curve data into a CSV or TXT file"));
    logButton->setText(tr("Start Logging"));
    hlayout->addWidget(logButton);
    connect(logButton, &QToolButton::clicked, this, &LinechartWidget::startLogging);

    // Ground time button
    timeButton = new QCheckBox(this);
    timeButton->setText(tr("Ground Time"));
    timeButton->setToolTip(tr("Overwrite timestamp of data from vehicle with ground receive time. Helps if the plots are not visible because of missing or invalid onboard time."));
    timeButton->setWhatsThis(tr("Overwrite timestamp of data from vehicle with ground receive time. Helps if the plots are not visible because of missing or invalid onboard time."));
    hlayout->addWidget(timeButton);
    connect(timeButton.data(), &QCheckBox::clicked, activePlot, &LinechartPlot::enforceGroundTime);
    connect(timeButton.data(), &QCheckBox::clicked, this, &LinechartWidget::writeSettings);

    hlayout->addStretch();

    QLabel *timeScaleLabel = new QLabel("Time axis:");
    hlayout->addWidget(timeScaleLabel);

    timeScaleCmb = new QComboBox(this);
    timeScaleCmb->addItem("10 seconds", 10);
    timeScaleCmb->addItem("20 seconds", 20);
    timeScaleCmb->addItem("30 seconds", 30);
    timeScaleCmb->addItem("40 seconds", 40);
    timeScaleCmb->addItem("50 seconds", 50);
    timeScaleCmb->addItem("1 minute", 60);
    timeScaleCmb->addItem("2 minutes", 60*2);
    timeScaleCmb->addItem("3 minutes", 60*3);
    timeScaleCmb->addItem("4 minutes", 60*4);
    timeScaleCmb->addItem("5 minutes", 60*5);
    timeScaleCmb->addItem("10 minutes", 60*10);
    //timeScaleCmb->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    timeScaleCmb->setMinimumContentsLength(12);

    hlayout->addWidget(timeScaleCmb);
    connect(timeScaleCmb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &LinechartWidget::timeScaleChanged);

    // Initialize the "Show units" checkbox. This is configured in the .ui file, so all
    // we do here is attach the clicked() signal.
    connect(ui.showUnitsCheckBox, &QCheckBox::clicked, this, &LinechartWidget::writeSettings);

    // Add actions
    averageSpinBox->setValue(activePlot->getAverageWindow());

    // Connect notifications from the user interface to the plot
    connect(this, &LinechartWidget::curveRemoved, activePlot, &LinechartPlot::hideCurve);

    // Update scrollbar when plot window changes (via translator method setPlotWindowPosition()
//    connect(activePlot, SIGNAL(windowPositionChanged(quint64)), this, SLOT(setPlotWindowPosition(quint64)));
    connect(activePlot, &LinechartPlot::curveRemoved, this, &LinechartWidget::removeCurve);

    // Update plot when scrollbar is moved (via translator method setPlotWindowPosition()
    //TODO: impossible to
    connect(this, static_cast<void (LinechartWidget::*)(quint64)>(&LinechartWidget::plotWindowPositionUpdated),
            activePlot, &LinechartPlot::setWindowPosition);

    // Set scaling
    connect(scalingLogButton, &QToolButton::toggled, this, &LinechartWidget::toggleLogarithmicScaling);
}

void LinechartWidget::timeScaleChanged(int index)
{
    activePlot->setPlotInterval(timeScaleCmb->itemData(index).toInt()*1000);
}

void LinechartWidget::toggleLogarithmicScaling(bool checked)
{
    if(checked)
        activePlot->setLogarithmicScaling();
    else
        activePlot->setLinearScaling();
}

void LinechartWidget::appendData(int uasId, const QString& curve, const QString& unit, const QVariant &variant, quint64 usec)
{
    QMetaType::Type type = static_cast<QMetaType::Type>(variant.type());
    bool ok;
    double value = variant.toDouble(&ok);
    if(!ok || type == QMetaType::QByteArray || type == QMetaType::QString)
        return;
    bool isDouble = type == QMetaType::Float || type == QMetaType::Double;
    QString curveID = curve + unit;

    if ((selectedMAV == -1 && isVisible()) || (selectedMAV == uasId && isVisible()))
    {
        // Order matters here, first append to plot, then update curve list
        activePlot->appendData(curveID, usec, value);
        // Store data
        QLabel* label = curveLabels->value(curveID, NULL);
        // Make sure the curve will be created if it does not yet exist
        if(!label)
        {
            if(!isDouble)
                intData.insert(curveID, 0);
            addCurve(curve, unit);
        }

        // Add int data
        if(!isDouble)
            intData.insert(curveID, variant.toInt());
    }

    if (lastTimestamp == 0 && usec != 0)
    {
        lastTimestamp = usec;
    } else if (usec != 0) {
        // Difference larger than 3 secs, enforce ground time
        if (((qint64)usec - (qint64)lastTimestamp) > 3000)
        {
            autoGroundTimeSet = true;
            // Tick ground time checkbox, but avoid state switching
            timeButton->blockSignals(true);
            timeButton->setChecked(true);
            timeButton->blockSignals(false);
            if (activePlot) activePlot->enforceGroundTime(true);
        }
        lastTimestamp = usec;
    }

    // Log data
    if (logging)
    {
        if (activePlot->isVisible(curveID))
        {
            if (usec == 0) usec = QGC::groundTimeMilliseconds();
            if (logStartTime == 0) logStartTime = usec;
            qint64 time = usec - logStartTime;
            if (time < 0) time = 0;

            QString line = QString("%1\t%2\t%3\t%4\n").arg(time).arg(uasId).arg(curve).arg(value, 0, 'e', 15);
            logFile->write(line.toLatin1());
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
    // Check if any curve is enabled
    if (!activePlot->anyCurveVisible()) {
        QGCMessageBox::critical(
            tr("No curves selected for logging."),
            tr("Please check all curves you want to log. Currently no data would be logged. Aborting the logging."));
        return;
    }

    // Let user select the log file name
    // QDate date(QDate::currentDate());
    // QString("./pixhawk-log-" + date.toString("yyyy-MM-dd") + "-" + QString::number(logindex) + ".log")
    QString fileName = QGCFileDialog::getSaveFileName(this,
        tr("Save Log File"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        tr("Log Files (*.log)"),
        "log"); // Default type

    qDebug() << "SAVE FILE " << fileName;

    if (!fileName.isEmpty()) {
        logFile = new QFile(fileName);
        if (logFile->open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text)) {
            logging = true;
            logStartTime = 0;
            curvesWidget->setEnabled(false);
            logindex++;
            logButton->setText(tr("Stop logging"));
            disconnect(logButton, &QToolButton::clicked, this, &LinechartWidget::startLogging);
            connect(logButton, &QToolButton::clicked, this, &LinechartWidget::stopLogging);
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
        connect(compressor, &LogCompressor::finishedFile, this, &LinechartWidget::logfileWritten);

        QMessageBox::StandardButton button = QGCMessageBox::question(
            tr("Starting Log Compression"),
            tr("Should empty fields (e.g. due to packet drops) be filled with the previous value of the same variable (zero order hold)?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        bool fill = (button == QMessageBox::Yes);

        compressor->startCompression(fill);
    }
    logButton->setText(tr("Start logging"));
    disconnect(logButton, &QToolButton::clicked, this, &LinechartWidget::stopLogging);
    connect(logButton, &QToolButton::clicked, this, &LinechartWidget::startLogging);
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
    QString curveID = curve + unit;
    curveNames.insert(curveID, curve);
    int labelRow = curvesWidgetLayout->rowCount();

    // Checkbox
    QCheckBox* checkBox = new QCheckBox(this);
    checkBox->setCheckable(true);
    checkBox->setObjectName(curveID);
    checkBox->setToolTip(tr("Enable the curve in the graph window"));
    checkBox->setWhatsThis(tr("Enable the curve in the graph window"));
    checkBoxes.insert(curveID, checkBox);
    curvesWidgetLayout->addWidget(checkBox, labelRow, 0);

    // Icon
    QWidget* colorIcon = new QWidget(this);
    colorIcons.insert(curveID, colorIcon);
    colorIcon->setMinimumSize(QSize(5, 14));
    colorIcon->setMaximumSize(QSize(5, 14));
    curvesWidgetLayout->addWidget(colorIcon, labelRow, 1);

    // Label
    QLabel* label = new QLabel(this);
    label->setText(getCurveName(curveID, ui.shortNameCheckBox->isChecked()));
    curveNameLabels.insert(curveID, label);
    curvesWidgetLayout->addWidget(label, labelRow, 2);

    // Value
    QLabel* value = new QLabel(this);
    value->setNum(0.00);
    value->setStyleSheet(QString("QLabel {font-family:\"Courier\"; font-weight: bold;}"));
    value->setToolTip(tr("Current value of %1 in %2 units").arg(curve, unit));
    value->setWhatsThis(tr("Current value of %1 in %2 units").arg(curve, unit));
    curveLabels->insert(curveID, value);
    curvesWidgetLayout->addWidget(value, labelRow, 3, Qt::AlignRight);

    // Unit
    QLabel* unitLabel = new QLabel(this);
    unitLabel->setText(unit);
    unitLabel->setToolTip(tr("Unit of ") + curve);
    unitLabel->setWhatsThis(tr("Unit of ") + curve);
    curveUnits.insert(curveID, unitLabel);
    curvesWidgetLayout->addWidget(unitLabel, labelRow, 4);
    unitLabel->setVisible(ui.showUnitsCheckBox->isChecked());
    connect(ui.showUnitsCheckBox, &QCheckBox::clicked, unitLabel, &QLabel::setVisible);

    // Mean
    QLabel* mean = new QLabel(this);
    mean->setNum(0.00);
    mean->setStyleSheet(QString("QLabel {font-family:\"Courier\"; font-weight: bold;}"));
    mean->setToolTip(tr("Arithmetic mean of %1 in %2 units").arg(curve, unit));
    mean->setWhatsThis(tr("Arithmetic mean of %1 in %2 units").arg(curve, unit));
    curveMeans->insert(curveID, mean);
    curvesWidgetLayout->addWidget(mean, labelRow, 5, Qt::AlignRight);

//    // Median
//    median = new QLabel(form);
//    value->setNum(0.00);
//    curveMedians->insert(curve, median);
//    horizontalLayout->addWidget(median);

    // Variance
    QLabel* variance = new QLabel(this);
    variance->setNum(0.00);
    variance->setStyleSheet(QString("QLabel {font-family:\"Courier\"; font-weight: bold;}"));
    variance->setToolTip(tr("Variance of %1 in (%2)^2 units").arg(curve, unit));
    variance->setWhatsThis(tr("Variance of %1 in (%2)^2 units").arg(curve, unit));
    curveVariances->insert(curveID, variance);
    curvesWidgetLayout->addWidget(variance, labelRow, 6, Qt::AlignRight);

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
    connect(selectAllCheckBox, &QCheckBox::clicked, checkBox, &QCheckBox::setChecked);
    QObject::connect(checkBox, &QCheckBox::clicked, this, &LinechartWidget::takeButtonClick);
    QObject::connect(this, &LinechartWidget::curveVisible, plot, &LinechartPlot::setVisibleById);

    // Set UI components to initial state
    checkBox->setChecked(false);
    plot->setVisibleById(curveID, false);
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
    widget = colorIcons.take(curve);
    curvesWidgetLayout->removeWidget(widget);
    widget->deleteLater();
    widget = curveNameLabels.take(curve);
    curvesWidgetLayout->removeWidget(widget);
    widget->deleteLater();
    widget = curveUnits.take(curve);
    curvesWidgetLayout->removeWidget(widget);
    widget->deleteLater();
    QCheckBox* checkbox;
    checkbox = checkBoxes.take(curve);
    curvesWidgetLayout->removeWidget(checkbox);
    checkbox->deleteLater();
//    intData->remove(curve);
}

void LinechartWidget::recolor()
{
    activePlot->styleChanged(qgcApp()->styleIsDark());
    foreach (const QString &key, colorIcons.keys())
    {
        QWidget* colorIcon = colorIcons.value(key, 0);
        if (colorIcon && !colorIcon->styleSheet().isEmpty())
        {
            QString colorstyle;
            QColor color = activePlot->getColorForCurve(key);
            colorstyle = colorstyle.sprintf("QWidget { background-color: #%02X%02X%02X; }", color.red(), color.green(), color.blue());
            colorIcon->setStyleSheet(colorstyle);
        }
    }
}

void LinechartWidget::setPlotFilterLineEditFocus()
{
    ui.plotFilterLineEdit->setFocus(Qt::ShortcutFocusReason);
}

void LinechartWidget::filterCurve(const QString &key, bool match)
{
        if (!checkBoxes[key]->isChecked())
        {
            colorIcons[key]->setVisible(match);
            curveNameLabels[key]->setVisible(match);
            (*curveLabels)[key]->setVisible(match);
            (*curveMeans)[key]->setVisible(match);
            (*curveVariances)[key]->setVisible(match);
            curveUnits[key]->setVisible(match && ui.showUnitsCheckBox->isChecked());
            checkBoxes[key]->setVisible(match);
        }
}

void LinechartWidget::filterCurves(const QString &filter)
{
    //qDebug() << "filterCurves: filter: " << filter;

    if (filter != "")
    {
        /* Hide Elements which do not match the filter pattern */
        QStringMatcher stringMatcher(filter, Qt::CaseInsensitive);
        foreach (const QString &key, colorIcons.keys())
        {
            if (stringMatcher.indexIn(key) < 0)
            {
                filterCurve(key, false);
            }
            else
            {
                filterCurve(key, true);
            }
        }
    }
    else
    {
        /* Show all Elements */
        foreach (const QString &key, colorIcons.keys())
        {
            filterCurve(key, true);
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
    foreach (const QString &key, curveNames.keys())
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
 * This method allows to map a button to a plot curve. The text of the
 * button must equal the curve name to activate / deactivate. If the checkbox
 * was clicked, show the curve color, otherwise clear the coloring.
 *
 * @param checked The visibility of the curve: true to display the curve, false otherwise
 **/
void LinechartWidget::takeButtonClick(bool checked)
{

    QCheckBox* button = qobject_cast<QCheckBox*>(QObject::sender());

    if(button != NULL)
    {
        activePlot->setVisibleById(button->objectName(), checked);
        QWidget* colorIcon = colorIcons.value(button->objectName(), 0);
        if (colorIcon)
        {
            if (checked)
            {
                QColor color = activePlot->getColorForCurve(button->objectName());
                if (color.isValid())
                {
                    QString colorstyle;
                    colorstyle = colorstyle.sprintf("QWidget { background-color: #%02X%02X%02X; }", color.red(), color.green(), color.blue());
                    colorIcon->setStyleSheet(colorstyle);
                }
            }
            else
            {
                colorIcon->setStyleSheet("");
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
