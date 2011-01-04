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
 *   @brief Implementation of class LogCompressor
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>
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
        uasid(uasid)
{
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
    quint64 lastTimeIndex = 0;
    while (!data.atEnd())
    {
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

        // Constraining the search area might result in not finding a key,
        // but it significantly reduces the time needed for the search
        // setting a window of 1000 entries means that a 1 Hz data point
        // can still be located
        int offsetLimit = 200;
        quint64 offset;
        quint64 index = -1;
        // FIXME Lorenz (index is an unsigend int)
        while (index == -1)
        {
            if (lastTimeIndex > offsetLimit)
            {
                offset = lastTimeIndex - offsetLimit;
            }
            else
            {
                offset = 0;
            }
            quint64 index = times->indexOf(time, offset);
            // FIXME Lorenz (index is an unsigend int)
            if (index == -1)
            {
                qDebug() << "INDEX NOT FOUND DURING LOGFILE PROCESSING, RESTARTING SEARCH";
                // FIXME Reset and start without offset heuristic again
                offsetLimit+=1000;
            }
        }
        lastTimeIndex = index;
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
    emit finishedFile(outfile.fileName());
    running = false;
}

void LogCompressor::startCompression()
{
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
