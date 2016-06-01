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
 *   @brief Implementation of QGCDataPlot2D
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTemporaryFile>
#ifndef __mobile__
#include <QPrintDialog>
#include <QPrinter>
#endif
#include <QProgressDialog>
#include <QHBoxLayout>
#include <QSvgGenerator>
#include <QStandardPaths>
#include <QDebug>

#include <cmath>

#include "QGCDataPlot2D.h"
#include "ui_QGCDataPlot2D.h"
#include "MG.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"

QGCDataPlot2D::QGCDataPlot2D(QWidget *parent) :
    QWidget(parent),
    plot(new IncrementalPlot(parent)),
    logFile(NULL),
    ui(new Ui::QGCDataPlot2D)
{
    ui->setupUi(this);

    // Add plot to ui
    QHBoxLayout* layout = new QHBoxLayout(ui->plotFrame);
    layout->addWidget(plot);
    ui->plotFrame->setLayout(layout);
    ui->gridCheckBox->setChecked(plot->gridEnabled());

    // Connect user actions
    connect(ui->selectFileButton, &QPushButton::clicked, this, &QGCDataPlot2D::selectFile);
    connect(ui->saveCsvButton, &QPushButton::clicked, this, &QGCDataPlot2D::saveCsvLog);
    connect(ui->reloadButton, &QPushButton::clicked, this, &QGCDataPlot2D::reloadFile);
    connect(ui->savePlotButton, &QPushButton::clicked, this, &QGCDataPlot2D::savePlot);
    connect(ui->printButton, &QPushButton::clicked, this, &QGCDataPlot2D::print);
    connect(ui->legendCheckBox, &QCheckBox::clicked, plot, &IncrementalPlot::showLegend);
    connect(ui->symmetricCheckBox,&QCheckBox::clicked, plot, &IncrementalPlot::setSymmetric);
    connect(ui->gridCheckBox, &QCheckBox::clicked, plot, &IncrementalPlot::showGrid);

    connect(ui->style, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
            plot, &IncrementalPlot::setStyleText);

    //TODO: calculateRegression returns bool, slots are expected to return void, this makes
    // converting to new style way too hard.
    connect(ui->regressionButton, SIGNAL(clicked()), this, SLOT(calculateRegression()));

    // Allow style changes to propagate through this widget
    connect(qgcApp(), &QGCApplication::styleChanged, plot, &IncrementalPlot::styleChanged);
}

void QGCDataPlot2D::reloadFile()
{
    if (QFileInfo(fileName).isReadable()) {
        if (ui->inputFileType->currentText().contains("pxIMU") || ui->inputFileType->currentText().contains("RAW")) {
            loadRawLog(fileName, ui->xAxis->currentText(), ui->yAxis->text());
        } else if (ui->inputFileType->currentText().contains("CSV")) {
            loadCsvLog(fileName, ui->xAxis->currentText(), ui->yAxis->text());
        }
    }
}

void QGCDataPlot2D::loadFile()
{
    qDebug() << "DATA PLOT: Loading file:" << fileName;
    if (QFileInfo(fileName).isReadable()) {
        if (ui->inputFileType->currentText().contains("pxIMU") || ui->inputFileType->currentText().contains("RAW")) {
            loadRawLog(fileName);
        } else if (ui->inputFileType->currentText().contains("CSV")) {
            loadCsvLog(fileName);
        }
    }
}

void QGCDataPlot2D::loadFile(QString file)
{
    // TODO This "filename" is a private/protected member variable. It should be named in such way
    // it indicates so. This same name is used in several places within this file in local scopes.
    fileName = file;
    QFileInfo fi(fileName);
    if (fi.isReadable()) {
        if (fi.suffix() == QString("raw") || fi.suffix() == QString("imu")) {
            loadRawLog(fileName);
        } else if (fi.suffix() == QString("txt") || fi.suffix() == QString("csv")) {
            loadCsvLog(fileName);
        }
        // TODO Else, tell the user it doesn't know what to do with the file...
    }
}

/**
 * This function brings up a file name dialog and asks the user to enter a file to save to
 */
QString QGCDataPlot2D::getSavePlotFilename()
{
    QString fileName = QGCFileDialog::getSaveFileName(
        this, "Save Plot File", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "PDF Documents (*.pdf);;SVG Images (*.svg)",
        "pdf");
    return fileName;
}

/**
 * This function aks the user for a filename and exports to either PDF or SVG, depending on the filename
 */
void QGCDataPlot2D::savePlot()
{
    QString fileName = getSavePlotFilename();
    if (fileName.isEmpty())
        return;

    while(!(fileName.endsWith(".svg") || fileName.endsWith(".pdf"))) {
        QMessageBox::StandardButton button = QGCMessageBox::warning(
            tr("Unsuitable file extension for Plot document type."),
            tr("Please choose .pdf or .svg as file extension. Click OK to change the file extension, cancel to not save the file."),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Ok);
        // Abort if cancelled
        if (button == QMessageBox::Cancel) {
            return;
        }

        fileName = getSavePlotFilename();
        if (fileName.isEmpty())
            return; //Abort if cancelled
    }

    if (fileName.endsWith(".pdf")) {
        exportPDF(fileName);
    } else if (fileName.endsWith(".svg")) {
        exportSVG(fileName);
    }
}


void QGCDataPlot2D::print()
{
#ifndef __mobile__
    QPrinter printer(QPrinter::HighResolution);
    //    printer.setOutputFormat(QPrinter::PdfFormat);
    //    //QPrinter printer(QPrinter::HighResolution);
    //    printer.setOutputFileName(fileName);

    QString docName = plot->title().text();
    if ( !docName.isEmpty() ) {
        docName.replace (QRegExp (QString::fromLatin1 ("\n")), tr (" -- "));
        printer.setDocName (docName);
    }

    printer.setCreator("QGroundControl");
    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog dialog(&printer);
    if ( dialog.exec() ) {
        plot->setStyleSheet("QWidget { background-color: #FFFFFF; color: #000000; background-clip: border; font-size: 10pt;}");
        plot->setCanvasBackground(Qt::white);
        // FIXME: QwtPlotPrintFilter no longer exists in Qwt 6.1
        //QwtPlotPrintFilter filter;
        //filter.color(Qt::white, QwtPlotPrintFilter::CanvasBackground);
        //filter.color(Qt::black, QwtPlotPrintFilter::AxisScale);
        //filter.color(Qt::black, QwtPlotPrintFilter::AxisTitle);
        //filter.color(Qt::black, QwtPlotPrintFilter::MajorGrid);
        //filter.color(Qt::black, QwtPlotPrintFilter::MinorGrid);
        //if ( printer.colorMode() == QPrinter::GrayScale ) {
        //    int options = QwtPlotPrintFilter::PrintAll;
        //    options &= ~QwtPlotPrintFilter::PrintBackground;
        //    options |= QwtPlotPrintFilter::PrintFrameWithScales;
        //    filter.setOptions(options);
        //}
        //plot->print(printer);
        plot->setStyleSheet("QWidget { background-color: #050508; color: #DDDDDF; background-clip: border; font-size: 11pt;}");
        //plot->setCanvasBackground(QColor(5, 5, 8));
    }
#endif
}

void QGCDataPlot2D::exportPDF(QString fileName)
{
#ifdef __mobile__
    Q_UNUSED(fileName)
#else
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    //printer.setFullPage(true);
    printer.setPageMargins(10.0, 10.0, 10.0, 10.0, QPrinter::Millimeter);
    printer.setPageSize(QPrinter::A4);

    QString docName = plot->title().text();
    if ( !docName.isEmpty() ) {
        docName.replace (QRegExp (QString::fromLatin1 ("\n")), tr (" -- "));
        printer.setDocName (docName);
    }

    printer.setCreator("QGroundControl");
    printer.setOrientation(QPrinter::Landscape);

    plot->setStyleSheet("QWidget { background-color: #FFFFFF; color: #000000; background-clip: border; font-size: 10pt;}");
    //        plot->setCanvasBackground(Qt::white);
    // FIXME: QwtPlotPrintFilter no longer exists in Qwt 6.1
    //        QwtPlotPrintFilter filter;
    //        filter.color(Qt::white, QwtPlotPrintFilter::CanvasBackground);
    //        filter.color(Qt::black, QwtPlotPrintFilter::AxisScale);
    //        filter.color(Qt::black, QwtPlotPrintFilter::AxisTitle);
    //        filter.color(Qt::black, QwtPlotPrintFilter::MajorGrid);
    //        filter.color(Qt::black, QwtPlotPrintFilter::MinorGrid);
    //        if ( printer.colorMode() == QPrinter::GrayScale )
    //        {
    //            int options = QwtPlotPrintFilter::PrintAll;
    //            options &= ~QwtPlotPrintFilter::PrintBackground;
    //            options |= QwtPlotPrintFilter::PrintFrameWithScales;
    //            filter.setOptions(options);
    //        }
    //plot->print(printer);
    plot->setStyleSheet("QWidget { background-color: #050508; color: #DDDDDF; background-clip: border; font-size: 11pt;}");
    //plot->setCanvasBackground(QColor(5, 5, 8));
#endif
}

void QGCDataPlot2D::exportSVG(QString fileName)
{
#ifdef __mobile__
    Q_UNUSED(fileName)
#else
    if ( !fileName.isEmpty() ) {
        plot->setStyleSheet("QWidget { background-color: #FFFFFF; color: #000000; background-clip: border; font-size: 10pt;}");
        //plot->setCanvasBackground(Qt::white);
        QSvgGenerator generator;
        generator.setFileName(fileName);
        generator.setSize(QSize(800, 600));

        // FIXME: QwtPlotPrintFilter no longer exists in Qwt 6.1
        //QwtPlotPrintFilter filter;
        //filter.color(Qt::white, QwtPlotPrintFilter::CanvasBackground);
        //filter.color(Qt::black, QwtPlotPrintFilter::AxisScale);
        //filter.color(Qt::black, QwtPlotPrintFilter::AxisTitle);
        //filter.color(Qt::black, QwtPlotPrintFilter::MajorGrid);
        //filter.color(Qt::black, QwtPlotPrintFilter::MinorGrid);

        //plot->print(generator);
        plot->setStyleSheet("QWidget { background-color: #050508; color: #DDDDDF; background-clip: border; font-size: 11pt;}");
    }
#endif
}

/**
 * Selects a filename and attempts immediately to load it.
 */
void QGCDataPlot2D::selectFile()
{
    // Open a file dialog prompting the user for the file to load.
    // Note the special case for the Pixhawk.
    if (ui->inputFileType->currentText().contains("pxIMU") || ui->inputFileType->currentText().contains("RAW")) {
        fileName = QGCFileDialog::getOpenFileName(this, tr("Load Log File"), QString(), "Log Files (*.imu *.raw)");
    }
    else
    {
        fileName = QGCFileDialog::getOpenFileName(this, tr("Load Log File"), QString(), "Log Files (*.csv);;All Files (*)");
    }

    // Check if the user hit cancel, which results in an empty string.
    // If this is the case, we just stop.
    if (fileName.isEmpty())
    {
        return;
    }

    // Now attempt to open the file
    QFileInfo fileInfo(fileName);
    if (!fileInfo.isReadable())
    {
        // TODO This needs some TLC. File used by another program sounds like a Windows only issue.
        QGCMessageBox::critical(
            tr("Could not open file"),
            tr("The file is owned by user %1. Is the file currently used by another program?").arg(fileInfo.owner()));
        ui->filenameLabel->setText(tr("Could not open %1").arg(fileInfo.fileName()));
    }
    else
    {
        ui->filenameLabel->setText(tr("Opened %1").arg(fileInfo.completeBaseName()+"."+fileInfo.completeSuffix()));
        // Open and import the file
        loadFile();
    }

}

void QGCDataPlot2D::loadRawLog(QString file, QString xAxisName, QString yAxisFilter)
{
    Q_UNUSED(xAxisName);
    Q_UNUSED(yAxisFilter);

    if (logFile != NULL) {
        logFile->close();
        delete logFile;
    }
    // Postprocess log file
    logFile = new QTemporaryFile("qt_qgc_temp_log.XXXXXX.csv");
    compressor = new LogCompressor(file, logFile->fileName());
    connect(compressor, &LogCompressor::finishedFile, this, static_cast<void (QGCDataPlot2D::*)(QString)>(&QGCDataPlot2D::loadFile));
    compressor->startCompression();
}

/**
 * This function loads a CSV file into the plot. It tries to assign the dimension names
 * based on the first data row and tries to guess the separator char.
 *
 * @param file Name of the file to open
 * @param xAxisName Optional paramater. If given, the x axis dimension will be selected to match this string
 * @param yAxisFilter Optional parameter. If given, only data dimension names present in the filter string will be
 *        plotted
 *
 * @code
 *
 * QString file = "/home/user/datalog.txt"; // With header: x<tab>y<tab>z
 * QString xAxis = "x";
 * QString yAxis = "z";
 *
 * // Plotted result will be x vs z with y ignored.
 * @endcode
 */
void QGCDataPlot2D::loadCsvLog(QString file, QString xAxisName, QString yAxisFilter)
{
    if (logFile != NULL) {
        logFile->close();
        delete logFile;
        curveNames.clear();
    }
    logFile = new QFile(file);

    // Load CSV data
    if (!logFile->open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // Set plot title
    if (ui->plotTitle->text() != "") plot->setTitle(ui->plotTitle->text());
    if (ui->plotXAxisLabel->text() != "") plot->setAxisTitle(QwtPlot::xBottom, ui->plotXAxisLabel->text());
    if (ui->plotYAxisLabel->text() != "") plot->setAxisTitle(QwtPlot::yLeft, ui->plotYAxisLabel->text());

    // Extract header

    // Read in values
    // Find all keys
    QTextStream in(logFile);

    // First line is header
    QString header = in.readLine();

    bool charRead = false;
    QString separator = "";
    QList<QChar> sepCandidates;
    sepCandidates << '\t';
    sepCandidates << ',';
    sepCandidates << ';';
    sepCandidates << ' ';
    sepCandidates << '~';
    sepCandidates << '|';

    // Iterate until separator is found
    // or full header is parsed
    for (int i = 0; i < header.length(); i++) {
        if (sepCandidates.contains(header.at(i))) {
            // Separator found
            if (charRead) {
                separator += header[i];
            }
        } else {
            // Char found
            charRead = true;
            // If the separator is not empty, this char
            // has been read after a separator, so detection
            // is now complete
            if (separator != "") break;
        }
    }

    QString out = separator;
    out.replace("\t", "<tab>");
    ui->filenameLabel->setText(file.split("/").last().split("\\").last()+" Separator: \""+out+"\"");
    //qDebug() << "READING CSV:" << header;

    // Clear plot
    plot->removeData();

    QMap<QString, QVector<double>* > xValues;
    QMap<QString, QVector<double>* > yValues;

    curveNames.append(header.split(separator, QString::SkipEmptyParts));

    // Eliminate any non-string curve names
    for (int i = 0; i < curveNames.count(); ++i)
    {
        if (curveNames.at(i).length() == 0 ||
            curveNames.at(i) == " " ||
            curveNames.at(i) == "\n" ||
            curveNames.at(i) == "\t" ||
            curveNames.at(i) == "\r")
        {
            // Remove bogus curve name
            curveNames.removeAt(i);
        }
    }

    QString curveName;

    // Clear UI elements
    ui->xAxis->clear();
    ui->yAxis->clear();
    ui->xRegressionComboBox->clear();
    ui->yRegressionComboBox->clear();
    ui->regressionOutput->clear();

    int curveNameIndex = 0;

    QString xAxisFilter;
    if (xAxisName == "") {
        xAxisFilter = curveNames.first();
    } else {
        xAxisFilter = xAxisName;
    }

    // Fill y-axis renaming lookup table
    // Allow the user to rename data dimensions in the plot
    QMap<QString, QString> renaming;

    QStringList yCurves = yAxisFilter.split("|", QString::SkipEmptyParts);

    // Figure out the correct renaming
    for (int i = 0; i < yCurves.count(); ++i)
    {
        if (yCurves.at(i).contains(":"))
        {
            QStringList parts = yCurves.at(i).split(":", QString::SkipEmptyParts);
            if (parts.count() > 1)
            {
                // Insert renaming map
                renaming.insert(parts.first(), parts.last());
                // Replace curve value with first part only
                yCurves.replace(i, parts.first());
            }
        }
//        else
//        {
//            // Insert same value, not renaming anything
//            renaming.insert(yCurves.at(i), yCurves.at(i));
//        }
    }


    foreach(curveName, curveNames) {
        // Add to plot x axis selection
        ui->xAxis->addItem(curveName);
        // Add to regression selection
        ui->xRegressionComboBox->addItem(curveName);
        ui->yRegressionComboBox->addItem(curveName);
        if (curveName != xAxisFilter) {
            if ((yAxisFilter == "") || yCurves.contains(curveName)) {
                yValues.insert(curveName, new QVector<double>());
                xValues.insert(curveName, new QVector<double>());
                // Add separator starting with second item
                if (curveNameIndex > 0 && curveNameIndex < curveNames.count()) {
                    ui->yAxis->setText(ui->yAxis->text()+"|");
                }
                // If this curve was renamed, re-add the renaming to the text field
                QString renamingText = "";
                if (renaming.contains(curveName)) renamingText = QString(":%1").arg(renaming.value(curveName));
                ui->yAxis->setText(ui->yAxis->text()+curveName+renamingText);
                // Insert same value, not renaming anything
                if (!renaming.contains(curveName)) renaming.insert(curveName, curveName);
                curveNameIndex++;
            }
        }
    }

    // Select current axis in UI
    ui->xAxis->setCurrentIndex(curveNames.indexOf(xAxisFilter));

    // Read data

    double x = 0;
    double y = 0;

    while (!in.atEnd())
    {
        QString line = in.readLine();

        // Keep empty parts here - we still have to act on them
        QStringList values = line.split(separator, QString::KeepEmptyParts);

        bool headerfound = false;

        // First get header - ORDER MATTERS HERE!
        foreach(curveName, curveNames)
        {
            if (curveName == xAxisFilter)
            {
                // X  AXIS HANDLING

                // Take this value as x if it is selected
                QString text = values.at(curveNames.indexOf(curveName));
                text = text.trimmed();
                if (text.length() > 0 && text != " " && text != "\n" && text != "\r" && text != "\t")
                {
                    bool okx = true;
                    x = text.toDouble(&okx);
                    if (okx && !qIsNaN(x) && !qIsInf(x))
                    {
                        headerfound = true;
                    }
                }
            }
        }

        if (headerfound)
        {
            // Search again from start for values - ORDER MATTERS HERE!
            foreach(curveName, curveNames)
            {
                // Y  AXIS HANDLING
                // Only plot non-x curver and those selected in the yAxisFilter (or all if the filter is not set)
                if(curveName != xAxisFilter && (yAxisFilter == "" || yCurves.contains(curveName)))
                {
                    bool oky;
                    int curveNameIndex = curveNames.indexOf(curveName);
                    if (values.count() > curveNameIndex)
                    {
                        QString text(values.at(curveNameIndex));
                        text = text.trimmed();
                        y = text.toDouble(&oky);
                        // Only INF is really an issue for the plot
                        // NaN is fine
                        if (oky && !qIsNaN(y) && !qIsInf(y) && text.length() > 0 && text != " " && text != "\n" && text != "\r" && text != "\t")
                        {
                            // Only append definitely valid values
                            xValues.value(curveName)->append(x);
                            yValues.value(curveName)->append(y);
                        }
                    }
                }
            }
        }
    }

    // Add data array of each curve to the plot at once (fast)
    // Iterates through all x-y curve combinations
    for (int i = 0; i < yValues.count(); i++) {
        if (renaming.contains(yValues.keys().at(i)))
        {
            plot->appendData(renaming.value(yValues.keys().at(i)), xValues.values().at(i)->data(), yValues.values().at(i)->data(), xValues.values().at(i)->count());
        }
        else
        {
            plot->appendData(yValues.keys().at(i), xValues.values().at(i)->data(), yValues.values().at(i)->data(), xValues.values().at(i)->count());
        }
    }
    plot->updateScale();
    plot->setStyleText(ui->style->currentText());
}

bool QGCDataPlot2D::calculateRegression()
{
    // TODO: Add support for quadratic / cubic curve fitting
    return calculateRegression(ui->xRegressionComboBox->currentText(), ui->yRegressionComboBox->currentText(), "linear");
}

/**
 * @param xName Name of the x dimension
 * @param yName Name of the y dimension
 * @param method Regression method, either "linear", "quadratic" or "cubic". Only linear is supported at this point
 */
bool QGCDataPlot2D::calculateRegression(QString xName, QString yName, QString method)
{
    bool result = false;
    QString function;
    if (xName != yName) {
        if (QFileInfo(fileName).isReadable()) {
            loadCsvLog(fileName, xName, yName);
            ui->xRegressionComboBox->setCurrentIndex(curveNames.indexOf(xName));
            ui->yRegressionComboBox->setCurrentIndex(curveNames.indexOf(yName));
        }

        // Create a couple of arrays for us to use to temporarily store some of the data from the plot.
        // These arrays are allocated on the heap as they are far too big to go in the stack and will
        // cause an overflow.
        // TODO: Look into if this would be better done by having a getter return const double pointers instead
        // of using memcpy().
        const int size = 100000;
        double *x = new double[size];
        double *y = new double[size];
        int copied = plot->data(yName, x, y, size);

        if (method == "linear") {
            double a;  // Y-axis crossing
            double b;  // Slope
            double r;  // Regression coefficient
            if (linearRegression(x, y, copied, &a, &b, &r)) {
                function = tr("%1 = %2 * %3 + %4 | R-coefficient: %5").arg(yName, QString::number(b), xName, QString::number(a), QString::number(r));

                // Plot curve
                // y-axis crossing (x = 0)
                // Set plotting to lines only
                plot->appendData(tr("regression %1-%2").arg(xName, yName), 0.0, a);
                plot->setStyleText("lines");
                // x-value of the current rightmost x position in the plot
                plot->appendData(tr("regression %1-%2").arg(xName, yName), plot->invTransform(QwtPlot::xBottom, plot->width() - plot->width()*0.08f), (a + b*plot->invTransform(QwtPlot::xBottom, plot->width() - plot->width() * 0.08f)));

                result = true;
            } else {
                function = tr("Linear regression failed. (Limit: %1 data points. Try with less)").arg(size);
            }
        } else {
            function = tr("Regression method %1 not found").arg(method);
        }

        delete[] x;
        delete[] y;
    } else {
        // xName == yName
        function = tr("Please select different X and Y dimensions, not %1 = %2").arg(xName, yName);
    }
    ui->regressionOutput->setText(function);
    return result;
}

/**
 * Linear regression (least squares) for n data points.
 * Computes:
 *
 * y = a * x + b
 *
 * @param x values on x axis
 * @param y corresponding values on y axis
 * @param n Number of values
 * @param a returned slope of line
 * @param b y-axis intersection
 * @param r regression coefficient. The larger the coefficient is, the better is
 *          the match of the regression.
 * @return 1 on success, 0 on failure (e.g. because of infinite slope)
 */
bool QGCDataPlot2D::linearRegression(double *x, double *y, int n, double *a, double *b, double *r)
{
    int i;
    double sumx=0,sumy=0,sumx2=0,sumy2=0,sumxy=0;
    double sxx,syy,sxy;

    *a = 0;
    *b = 0;
    *r = 0;
    if (n < 2)
        return true;

    /* Conpute some things we need */
    for (i=0; i<n; i++) {
        sumx += x[i];
        sumy += y[i];
        sumx2 += (x[i] * x[i]);
        sumy2 += (y[i] * y[i]);
        sumxy += (x[i] * y[i]);
    }
    sxx = sumx2 - sumx * sumx / n;
    syy = sumy2 - sumy * sumy / n;
    sxy = sumxy - sumx * sumy / n;

    /* Infinite slope (b), non existant intercept (a) */
    if (fabs(sxx) == 0)
        return false;

    /* Calculate the slope (b) and intercept (a) */
    *b = sxy / sxx;
    *a = sumy / n - (*b) * sumx / n;

    /* Compute the regression coefficient */
    if (fabs(syy) == 0)
        *r = 1;
    else
        *r = sxy / sqrt(sxx * syy);

    return false;
}

void QGCDataPlot2D::saveCsvLog()
{
    QString fileName = QGCFileDialog::getSaveFileName(
        this, "Save CSV Log File", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "CSV Files (*.csv)",
        "csv",
        true);

    if (fileName.isEmpty()) {
        return; //User cancelled
    }

    bool success = logFile->copy(fileName);

    qDebug() << "Saved CSV log (" << fileName << "). Success: " << success;

    //qDebug() << "READE TO SAVE CSV LOG TO " << fileName;
}

QGCDataPlot2D::~QGCDataPlot2D()
{
    delete ui;
}

void QGCDataPlot2D::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
