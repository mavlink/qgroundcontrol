
#include <QFileDialog>
#include <QTemporaryFile>
#include <QMessageBox>
#include <QPrintDialog>
#include <QProgressDialog>
#include <QHBoxLayout>
#include <QSvgGenerator>
#include <QPrinter>
#include <QDesktopServices>
#include "QGCDataPlot2D.h"
#include "ui_QGCDataPlot2D.h"
#include "MG.h"
#include <cmath>

#include <QDebug>

QGCDataPlot2D::QGCDataPlot2D(QWidget *parent) :
        QWidget(parent),
        plot(new IncrementalPlot()),
        logFile(NULL),
        ui(new Ui::QGCDataPlot2D)
{
    ui->setupUi(this);

    // Add plot to ui
    QHBoxLayout* layout = new QHBoxLayout(ui->plotFrame);
    layout->addWidget(plot);
    ui->plotFrame->setLayout(layout);

    // Connect user actions
    connect(ui->selectFileButton, SIGNAL(clicked()), this, SLOT(selectFile()));
    connect(ui->saveCsvButton, SIGNAL(clicked()), this, SLOT(saveCsvLog()));
    connect(ui->reloadButton, SIGNAL(clicked()), this, SLOT(reloadFile()));
    connect(ui->savePlotButton, SIGNAL(clicked()), this, SLOT(savePlot()));
    connect(ui->printButton, SIGNAL(clicked()), this, SLOT(print()));
    connect(ui->legendCheckBox, SIGNAL(clicked(bool)), plot, SLOT(showLegend(bool)));
    connect(ui->style, SIGNAL(currentIndexChanged(QString)), plot, SLOT(setStyleText(QString)));
}

void QGCDataPlot2D::reloadFile()
{
    if (QFileInfo(fileName).isReadable())
    {
        if (ui->inputFileType->currentText().contains("pxIMU"))
        {
            loadRawLog(fileName, ui->xAxis->currentText(), ui->yAxis->text());
        }
        else if (ui->inputFileType->currentText().contains("CSV"))
        {
            loadCsvLog(fileName, ui->xAxis->currentText(), ui->yAxis->text());
        }
    }
}

void QGCDataPlot2D::loadFile()
{
    if (QFileInfo(fileName).isReadable())
    {
        if (ui->inputFileType->currentText().contains("pxIMU"))
        {
            loadRawLog(fileName);
        }
        else if (ui->inputFileType->currentText().contains("CSV"))
        {
            loadCsvLog(fileName);
        }
    }
}

void QGCDataPlot2D::savePlot()
{
    QString fileName = "plot.svg";
    fileName = QFileDialog::getSaveFileName(
            this, "Export File Name", QDesktopServices::storageLocation(QDesktopServices::DesktopLocation),
            "SVG Documents (*.svg);;");
    while(!fileName.endsWith("svg"))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Unsuitable file extension for SVG");
        msgBox.setInformativeText("Please choose .svg as file extension. Click OK to change the file extension, cancel to not save the file.");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        if(msgBox.exec() == QMessageBox::Cancel) break;
        fileName = QFileDialog::getSaveFileName(
                this, "Export File Name", QDesktopServices::storageLocation(QDesktopServices::DesktopLocation),
                "SVG Documents (*.svg);;");
    }
    exportSVG(fileName);

    //    else if (fileName.endsWith("pdf"))
    //    {
    //        print(fileName);
    //    }
}


void QGCDataPlot2D::print()
{
    QPrinter printer(QPrinter::HighResolution);
    //    printer.setOutputFormat(QPrinter::PdfFormat);
    //    //QPrinter printer(QPrinter::HighResolution);
    //    printer.setOutputFileName(fileName);

    QString docName = plot->title().text();
    if ( !docName.isEmpty() )
    {
        docName.replace (QRegExp (QString::fromLatin1 ("\n")), tr (" -- "));
        printer.setDocName (docName);
    }

    printer.setCreator("QGroundControl");
    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog dialog(&printer);
    if ( dialog.exec() )
    {
        plot->setStyleSheet("QWidget { background-color: #FFFFFF; color: #000000; background-clip: border; font-size: 10pt;}");
        plot->setCanvasBackground(Qt::white);
        QwtPlotPrintFilter filter;
        filter.color(Qt::white, QwtPlotPrintFilter::CanvasBackground);
        filter.color(Qt::black, QwtPlotPrintFilter::AxisScale);
        filter.color(Qt::black, QwtPlotPrintFilter::AxisTitle);
        filter.color(Qt::black, QwtPlotPrintFilter::MajorGrid);
        filter.color(Qt::black, QwtPlotPrintFilter::MinorGrid);
        if ( printer.colorMode() == QPrinter::GrayScale )
        {
            int options = QwtPlotPrintFilter::PrintAll;
            options &= ~QwtPlotPrintFilter::PrintBackground;
            options |= QwtPlotPrintFilter::PrintFrameWithScales;
            filter.setOptions(options);
        }
        plot->print(printer, filter);
        plot->setStyleSheet("QWidget { background-color: #050508; color: #DDDDDF; background-clip: border; font-size: 11pt;}");
        //plot->setCanvasBackground(QColor(5, 5, 8));
    }
}

void QGCDataPlot2D::exportSVG(QString fileName)
{
    if ( !fileName.isEmpty() )
    {
        QSvgGenerator generator;
        generator.setFileName(fileName);
        generator.setSize(QSize(800, 600));

        QwtPlotPrintFilter filter;
        filter.color(Qt::white, QwtPlotPrintFilter::CanvasBackground);
        filter.color(Qt::black, QwtPlotPrintFilter::AxisScale);
        filter.color(Qt::black, QwtPlotPrintFilter::AxisTitle);
        filter.color(Qt::black, QwtPlotPrintFilter::MajorGrid);
        filter.color(Qt::black, QwtPlotPrintFilter::MinorGrid);

        plot->print(generator, filter);
    }
}

/**
 * Selects a filename and attempts immediately to load it.
 */
void QGCDataPlot2D::selectFile()
{
    // Let user select the log file name
    //QDate date(QDate::currentDate());
    // QString("./pixhawk-log-" + date.toString("yyyy-MM-dd") + "-" + QString::number(logindex) + ".log")
    fileName = QFileDialog::getOpenFileName(this, tr("Specify log file name"), tr("."), tr("Logfile (*.txt)"));
    // Store reference to file

    QFileInfo fileInfo(fileName);

    if (!fileInfo.isReadable())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Could not open file");
        msgBox.setInformativeText(tr("The file is owned by user %1. Is the file currently used by another program?").arg(fileInfo.owner()));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        ui->filenameLabel->setText(tr("Could not open %1").arg(fileInfo.baseName()+"."+fileInfo.completeSuffix()));
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
    if (logFile != NULL)
    {
        logFile->close();
        delete logFile;
    }
    // Postprocess log file
    logFile = new QTemporaryFile();
    compressor = new LogCompressor(file, logFile->fileName());

    // Block UI
    QProgressDialog progress("Transforming RAW log file to CSV", "Abort Transformation", 0, 1, this);
    progress.setWindowModality(Qt::WindowModal);

    while (!compressor->isFinished())
    {
        MG::SLEEP::usleep(100000);
        progress.setMaximum(compressor->getDataLines());
        progress.setValue(compressor->getCurrentLine());
    }
    // Enforce end
    progress.setMaximum(compressor->getDataLines());
    progress.setValue(compressor->getDataLines());

    // Done with preprocessing - now load csv log
    loadCsvLog(logFile->fileName(), xAxisName, yAxisFilter);
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
    if (logFile != NULL)
    {
        logFile->close();
        delete logFile;
    }
    logFile = new QFile(file);

    // Load CSV data
    if (!logFile->open(QIODevice::ReadOnly | QIODevice::Text))
        return;

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
    for (int i = 0; i < header.length(); i++)
    {
        if (sepCandidates.contains(header.at(i)))
        {
            // Separator found
            if (charRead)
            {
                separator += header[i];
            }
        }
        else
        {
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

    QVector<double> xValues;
    QMap<QString, QVector<double>* > yValues;

    QStringList curveNames = header.split(separator, QString::SkipEmptyParts);
    QString curveName;

    // Clear UI elements
    ui->xAxis->clear();
    ui->yAxis->clear();

    int curveNameIndex = 0;

    //int xValueIndex = curveNames.indexOf(xAxisName);

    QString xAxisFilter;
    if (xAxisName == "")
    {
        xAxisFilter = curveNames.first();
    }
    else
    {
        xAxisFilter = xAxisName;
    }

    foreach(curveName, curveNames)
    {
        ui->xAxis->addItem(curveName);
        if (curveName != xAxisFilter)
        {
            if ((yAxisFilter == "") || yAxisFilter.contains(curveName))
            {
                yValues.insert(curveName, new QVector<double>());
                // Add separator starting with second item
                if (curveNameIndex > 0 && curveNameIndex < curveNames.size())
                {
                    ui->yAxis->setText(ui->yAxis->text()+"|");
                }
                ui->yAxis->setText(ui->yAxis->text()+curveName);
                curveNameIndex++;
            }
        }
    }

    // Select current axis in UI
    ui->xAxis->setCurrentIndex(curveNames.indexOf(xAxisFilter));

    // Read data

    double x,y;

    while (!in.atEnd())
    {
        QString line = in.readLine();

        QStringList values = line.split(separator, QString::SkipEmptyParts);

        foreach(curveName, curveNames)
        {
            bool okx;
            if (curveName == xAxisFilter)
            {
                // X  AXIS HANDLING

                // Take this value as x if it is selected
                x = values.at(curveNames.indexOf(curveName)).toDouble(&okx);
                xValues.append(x - 1270125570000LL);
                qDebug() << "x" << x - 1270125570000LL;
            }
            else
            {
                // Y  AXIS HANDLING

                if(yAxisFilter == "" || yAxisFilter.contains(curveName))
                {
                    // Only append y values where a valid x value is present
                    if (yValues.value(curveName)->size() == xValues.size() - 1)
                    {
                        bool oky;
                        int curveNameIndex = curveNames.indexOf(curveName);
                        if (values.size() > curveNameIndex)
                        {
                            y = values.at(curveNameIndex).toDouble(&oky);
                            yValues.value(curveName)->append(y);
                        }
                    }
                }
            }
        }
    }

    // Add data array of each curve to the plot at once (fast)
    // Iterates through all x-y curve combinations
    for (int i = 0; i < yValues.size(); i++)
    {
        plot->appendData(yValues.keys().at(i), xValues.data(), yValues.values().at(i)->data(), xValues.size());
    }
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
int QGCDataPlot2D::linearRegression(double* x,double* y,int n,double* a,double* b,double* r)
{
    int i;
    double sumx=0,sumy=0,sumx2=0,sumy2=0,sumxy=0;
    double sxx,syy,sxy;

    *a = 0;
    *b = 0;
    *r = 0;
    if (n < 2)
        return(FALSE);

    /* Conpute some things we need */
    for (i=0;i<n;i++) {
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
        return(FALSE);

    /* Calculate the slope (b) and intercept (a) */
    *b = sxy / sxx;
    *a = sumy / n - (*b) * sumx / n;

    /* Compute the regression coefficient */
    if (fabs(syy) == 0)
        *r = 1;
    else
        *r = sxy / sqrt(sxx * syy);

    return(TRUE);
}

void QGCDataPlot2D::saveCsvLog()
{
    QString fileName = "export.csv";
    fileName = QFileDialog::getSaveFileName(
            this, "Export CSV File Name", QDesktopServices::storageLocation(QDesktopServices::DesktopLocation),
            "CSV file (*.csv);;Text file (*.txt)");
    //    QFileInfo fileInfo(fileName);
    //
    //    // Check if we could create a new file in this directory
    //    QDir dir(fileInfo.absoluteDir());
    //    QFileInfo dirInfo(dir);
    //
    //    while(!(dirInfo.isWritable()))
    //    {
    //        QMessageBox msgBox;
    //        msgBox.setIcon(QMessageBox::Critical);
    //        msgBox.setText("File cannot be written, Operating System denies permission");
    //        msgBox.setInformativeText("Please choose a different file name or directory. Click OK to change the file, cancel to not save the file.");
    //        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    //        msgBox.setDefaultButton(QMessageBox::Ok);
    //        if(msgBox.exec() == QMessageBox::Cancel) break;
    //        fileName = QFileDialog::getSaveFileName(
    //                this, "Export CSV File Name", QDesktopServices::storageLocation(QDesktopServices::DesktopLocation),
    //            "CSV file (*.csv);;Text file (*.txt)");
    //    }

    bool success = logFile->copy(fileName);

    qDebug() << "Saved CSV log. Success: " << success;

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
