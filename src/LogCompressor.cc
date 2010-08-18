#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>
#include "LogCompressor.h"

#include <QDebug>

LogCompressor::LogCompressor(QString logFileName, QString outFileName, int uasid) :
        logFileName(logFileName),
        outFileName(outFileName),
        running(true),
        currentDataLine(0),
        dataLines(1),
        uasid(uasid)
{
    start();
}

void LogCompressor::run()
{
    QString separator = "\t";
    QString fileName = logFileName;
    QFile file(fileName);
    QFile outfile(outFileName);
    QStringList* keys = new QStringList();
    QStringList* times = new QStringList();

    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    if (outFileName != "")
    {
        // Check if file is writeable
        if (!QFileInfo(outfile).isWritable())
        {
            return;
        }
    }

    // Find all keys
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Accumulate map of keys
        // Data field name is at position 2
        QString key = line.split(separator).at(2);
        if (!keys->contains(key)) keys->append(key);
    }
    keys->sort();

    QString header = "";
    QString spacer = "";
    for (int i = 0; i < keys->length(); i++)
    {
        header += keys->at(i) + separator;
        spacer += " " + separator;
    }

    //qDebug() << header;

    //qDebug() << "NOW READING TIMES";

    // Find all times
    //in.reset();
    file.reset();
    in.reset();
    in.resetStatus();
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Accumulate map of keys
        // Data field name is at position 2
        QString time = line.split(separator).at(0);
        if (!times->contains(time))
        {
            times->append(time);
        }
    }

    dataLines = times->length();

    times->sort();

    // Create lines
    QStringList* outLines = new QStringList();
    for (int i = 0; i < times->length(); i++)
    {
        outLines->append(times->at(i) + separator + spacer);
    }

    // Fill in the values for all keys
    file.reset();
    QTextStream data(&file);
    int linecounter = 0;
    while (!data.atEnd()) {
        linecounter++;
        currentDataLine = linecounter;
        QString line = data.readLine();
        QStringList parts = line.split(separator);
        // Get time
        QString time = parts.first();
        QString field = parts.at(2);
        QString value = parts.at(3);
        // Enforce NaN if no value is present
        if (value.length() == 0 || value == "" || value == " " || value == "\t" || value == "\n")
        {
            value = "NaN";
        }
        // Get matching output line
        quint64 index = times->indexOf(time);
        QString outLine = outLines->at(index);
        QStringList outParts = outLine.split(separator);
        // Replace measurement placeholder with current value
        outParts.replace(keys->indexOf(field)+1, value);
        outLine = outParts.join(separator);
        outLines->replace(index, outLine);
    }



    // Add header, write out file
    file.close();

    if (outFileName == "")
    {
        QFile::remove(file.fileName());
        outfile.setFileName(file.fileName());
    }
    if (!outfile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    outfile.write(QString(QString("unix_timestamp") + separator + header.replace(" ", "_") + QString("\n")).toLatin1());
    //QString fileHeader = QString("unix_timestamp") + header.replace(" ", "_") + QString("\n");

    // File output
    for (int i = 0; i < outLines->length(); i++)
    {
        //qDebug() << outLines->at(i);
        outfile.write(QString(outLines->at(i) + "\n").toLatin1());

    }

    currentDataLine = 0;
    dataLines = 1;
    delete keys;
    qDebug() << "Done with logfile processing";
    running = false;
}

bool LogCompressor::isFinished()
{
    return !running;
}

int LogCompressor::getCurrentLine()
{
    return currentDataLine;
}

int LogCompressor::getDataLines()
{
    return dataLines;
}
