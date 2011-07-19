/*===================================================================
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
 *   @brief Implementation of class LogCompressor
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>
#include <QList>
#include "LogCompressor.h"

#include <QDebug>

/**
 * It will only get active upon calling startCompression()
 */
LogCompressor::LogCompressor(QString logFileName, QString outFileName, int uasid) :
    logFileName(logFileName),
    outFileName(outFileName),
    running(true),
    currentDataLine(0),
    dataLines(1),
    uasid(uasid),
    holeFillingEnabled(true)
{
}

void LogCompressor::run()
{
    QString separator = "\t";
    QString fileName = logFileName;
    QFile file(fileName);
    QFile outfile(outFileName);
    QStringList* keys = new QStringList();
    QList<quint64> times;// = new QList<quint64>();
    QList<quint64> finalTimes;

    //qDebug() << "LOG COMPRESSOR: Starting" << fileName;

    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //qDebug() << "LOG COMPRESSOR: INPUT FILE DOES NOT EXIST";
        emit logProcessingStatusChanged(tr("Log Compressor: Cannot start/compress log file, since input file %1 is not readable").arg(QFileInfo(fileName).absoluteFilePath()));
        return;
    }

    // Check if file is writeable
    if (outFileName == ""/* || !QFileInfo(outfile).isWritable()*/) {
        //qDebug() << "LOG COMPRESSOR: OUTPUT FILE DOES NOT EXIST" << outFileName;
        emit logProcessingStatusChanged(tr("Log Compressor: Cannot start/compress log file, since output file %1 is not writable").arg(QFileInfo(outFileName).absoluteFilePath()));
        return;
    }

    // Find all keys
    QTextStream in(&file);

    // Search only a certain region, assuming that not more
    // than N dimensions at H Hertz can be send
    const unsigned int keySearchLimit = 15000;
    // e.g. 500 Hz * 30 values or
    // e.g. 100 Hz * 150 values

    unsigned int keyCounter = 0;
    while (!in.atEnd() && keyCounter < keySearchLimit) {
        QString line = in.readLine();
        // Accumulate map of keys
        // Data field name is at position 2
        QString key = line.split(separator).at(2);
        if (!keys->contains(key))
        {
            keys->append(key);
        }
        keyCounter++;
    }
    keys->sort();

    QString header = "";
    QString spacer = "";
    for (int i = 0; i < keys->length(); i++) {
        header += keys->at(i) + separator;
        spacer += separator;
    }

    emit logProcessingStatusChanged(tr("Log compressor: Dataset contains dimension: ") + header);

    // Find all times
    //in.reset();
    file.reset();
    in.reset();
    in.resetStatus();
    bool ok;
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Accumulate map of keys
        // Data field name is at position 2b
        quint64 time = static_cast<QString>(line.split(separator).at(0)).toLongLong(&ok);
        if (ok) {
            times.append(time);
        }
    }

    qSort(times);

    qint64 lastTime = -1;

    // Create lines
    QStringList* outLines = new QStringList();
    for (int i = 0; i < times.length(); i++) {
        // Cast to signed on purpose, 64 bit timestamp still long enough
        if (static_cast<qint64>(times.at(i)) != lastTime) {
            outLines->append(QString("%1").arg(times.at(i)) + separator + spacer);
            lastTime = static_cast<qint64>(times.at(i));
            finalTimes.append(times.at(i));
            //qDebug() << "ADDED:" << outLines->last();
        }
    }

    dataLines = finalTimes.length();

    emit logProcessingStatusChanged(tr("Log compressor: Now processing %1 log lines").arg(finalTimes.length()));

    // Fill in the values for all keys
    file.reset();
    QTextStream data(&file);
    int linecounter = 0;
    quint64 lastTimeIndex = 0;
    bool failed = false;

    while (!data.atEnd()) {
        linecounter++;
        currentDataLine = linecounter;
        QString line = data.readLine();
        QStringList parts = line.split(separator);
        // Get time
        quint64 time = static_cast<QString>(parts.first()).toLongLong(&ok);
        QString field = parts.at(2);
        int fieldIndex = keys->indexOf(field);
        QString value = parts.at(3);
//        // Enforce NaN if no value is present
//        if (value.length() == 0 || value == "" || value == " " || value == "\t" || value == "\n") {
//            // Hole filling disabled, fill with NaN
//            value = "NaN";
//        }
        // Get matching output line

        // Constraining the search area might result in not finding a key,
        // but it significantly reduces the time needed for the search
        // setting a window of 100 entries means that a 1 Hz data point
        // can still be located
        quint64 offsetLimit = 100;
        quint64 offset;
        qint64 index = -1;
        failed = false;

        // Search the index until it is valid (!= -1)
        // or the start of the list has been reached (failed)
        while (index == -1 && !failed) {
            if (lastTimeIndex > offsetLimit) {
                offset = lastTimeIndex - offsetLimit;
            } else {
                offset = 0;
            }

            index = finalTimes.indexOf(time, offset);
            if (index == -1) {
                if (offset == 0) {
                    emit logProcessingStatusChanged(tr("Log compressor: Timestamp %1 not found in dataset, ignoring log line %2").arg(time).arg(linecounter));
                    qDebug() << "Completely failed finding value";
                    //continue;
                    failed = true;
                } else {
                    emit logProcessingStatusChanged(tr("Log compressor: Timestamp %1 not found in dataset, restarting search.").arg(time));
                    offsetLimit*=2;
                }
            }
        }

        if (dataLines > 100) if (index % (dataLines/100) == 0) emit logProcessingStatusChanged(tr("Log compressor: Processed %1% of %2 lines").arg(index/(float)dataLines*100, 0, 'f', 2).arg(dataLines));

        if (!failed) {
            // When the algorithm reaches here the correct index was found
            lastTimeIndex = index;
            QString outLine = outLines->at(index);
            QStringList outParts = outLine.split(separator);
            // Replace measurement placeholder with current value
            outParts.replace(fieldIndex+1, value);
            outLine = outParts.join(separator);
            outLines->replace(index, outLine);
        }
    }

    ///////////////////////////
    // HOLE FILLING

    // If hole filling is enabled, run again through the whole file and replace holes
    if (holeFillingEnabled)
    {
        // Build up the fill values - initialize to NaN
        QStringList fillValues;
        int fillCount = keys->count();
        for (int i = 0; i< fillCount; ++i)
        {
            fillValues.append("NaN");
        }

        // Run through all lines and replace with fill values
        for (int index = 0; index < outLines->count(); ++index)
        {
            QString line = outLines->at(index);
            //qDebug() << "LINE" << line;
            QStringList fields = line.split(separator, QString::SkipEmptyParts);
            // The fields line contains the timestamp
            // index of the data fields therefore runs from 1 to n-1
            int fieldCount = fields.count();
            for (int i = 1; i < fillCount+1; ++i)
            {
                if (fieldCount <= i) fields.append("");

                // Allow input data to be screwed up
                if (fields.at(i) == "\t" || fields.at(i) == " " || fields.at(i) == "\n")
                {
                    // Remove invalid data
                    if (fieldCount > fillCount+1)
                    {
                        // This field has a seperator value and is too much
                        //qDebug() << "REMOVED INVALID INPUT DATA";
                        fields.removeAt(i);
                    }
                    // Continue on invalid data
                    continue;
                }

                // Check if this is NaN
                if (fields.at(i) == 0 || fields.at(i) == "")
                {
                    // Value was empty, replace it
                    fields.replace(i, fillValues[i-1]);
                    //qDebug() << "FILL" << fillValues.at(i-1);
                }
                else
                {
                    // Value was not NaN, use it as
                    // new fill value
                    fillValues.replace(i-1, fields[i]);
                }
            }
            outLines->replace(index, fields.join(separator));
        }
    }

    // Add header, write out file
    file.close();

    if (outFileName == logFileName) {
        QFile::remove(file.fileName());
        outfile.setFileName(file.fileName());

    }
    if (!outfile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    outfile.write(QString(QString("timestamp_ms") + separator + header.replace(" ", "_") + QString("\n")).toLatin1());
    emit logProcessingStatusChanged(tr("Log Compressor: Writing output to file %1").arg(QFileInfo(outFileName).absoluteFilePath()));

    // File output
    for (int i = 0; i < outLines->length(); i++) {
        //qDebug() << outLines->at(i);
        outfile.write(QString(outLines->at(i) + "\n").toLatin1());

    }

    currentDataLine = 0;
    dataLines = 1;
    delete keys;
    emit logProcessingStatusChanged(tr("Log compressor: Finished processing file: %1").arg(outfile.fileName()));
    qDebug() << "Done with logfile processing";
    emit finishedFile(outfile.fileName());
    running = false;
}

/**
 * @param holeFilling If hole filling is enabled, the compressor tries to fill empty data fields with previous
 *                    values from the same variable (or NaN, if no previous value existed)
 */
void LogCompressor::startCompression(bool holeFilling)
{
    // Set hole filling
    holeFillingEnabled = holeFilling;
    start();
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
