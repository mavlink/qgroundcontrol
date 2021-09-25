/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Implementation of class LogCompressor.
 *          This class reads in a file containing messages and translates it into a tab-delimited CSV file.
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 */

#include "LogCompressor.h"
#include "QGCApplication.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>
#include <QList>
#include <QDebug>

/**
 * Initializes all the variables necessary for a compression run. This won't actually happen
 * until startCompression(...) is called.
 */
LogCompressor::LogCompressor(QString logFileName, QString outFileName, QString delimiter) :
	logFileName(logFileName),
	outFileName(outFileName),
	running(true),
	currentDataLine(0),
    delimiter(delimiter),
    holeFillingEnabled(true)
{
    connect(this, &LogCompressor::logProcessingCriticalError, qgcApp(), &QGCApplication::criticalMessageBoxOnMainThread);
}

void LogCompressor::run()
{
	// Verify that the input file is useable
	QFile infile(logFileName);
	if (!infile.exists() || !infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		_signalCriticalError(tr("Log Compressor: Cannot start/compress log file, since input file %1 is not readable").arg(QFileInfo(infile.fileName()).absoluteFilePath()));
		return;
	}

//    outFileName = logFileName;

    QString outFileName;

    QStringList parts = QFileInfo(infile.fileName()).absoluteFilePath().split(".", Qt::SkipEmptyParts);

    parts.replace(0, parts.first() + "_compressed");
    parts.replace(parts.size()-1, "txt");
    outFileName = parts.join(".");

	// Verify that the output file is useable
    QFile outTmpFile(outFileName);
    if (!outTmpFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		_signalCriticalError(tr("Log Compressor: Cannot start/compress log file, since output file %1 is not writable").arg(QFileInfo(outTmpFile.fileName()).absoluteFilePath()));
		return;
	}


	// First we search the input file through keySearchLimit number of lines
	// looking for variables. This is necessary before CSV files require
	// the same number of fields for every line.
	const unsigned int keySearchLimit = 15000;
	unsigned int keyCounter = 0;
	QTextStream in(&infile);
	QMap<QString, int> messageMap;

	while (!in.atEnd() && keyCounter < keySearchLimit) {
		QString messageName = in.readLine().split(delimiter).at(2);
		messageMap.insert(messageName, 0);
		++keyCounter;
	}

	// Now update each key with its index in the output string. These are
	// all offset by one to account for the first field: timestamp_ms.
    QMap<QString, int>::iterator i = messageMap.begin();
	int j;
	for (i = messageMap.begin(), j = 1; i != messageMap.end(); ++i, ++j) {
		i.value() = j;
	}

	// Open the output file and write the header line to it
	QStringList headerList(messageMap.keys());

	QString headerLine = "timestamp_ms" + delimiter + headerList.join(delimiter) + "\n";
    // Clean header names from symbols Matlab considers as Latex syntax
    headerLine = headerLine.replace("timestamp", "TIMESTAMP");
    headerLine = headerLine.replace(":", "");
    headerLine = headerLine.replace("_", "");
    headerLine = headerLine.replace(".", "");
	outTmpFile.write(headerLine.toLocal8Bit());

    _signalCriticalError(tr("Log compressor: Dataset contains dimensions: ") + headerLine);

    // Template list stores a list for populating with data as it's parsed from messages.
    QStringList templateList;
    for (int i = 0; i < headerList.size() + 1; ++i) {
        templateList << (holeFillingEnabled?"NaN":"");
    }


//	// Reset our position in the input file before we start the main processing loop.
//    in.seek(0);

//    // Search through all lines and build a list of unique timestamps
//    QMap<quint64, QStringList> timestampMap;
//    while (!in.atEnd()) {
//        quint64 timestamp = in.readLine().split(delimiter).at(0).toULongLong();
//        timestampMap.insert(timestamp, templateList);
//    }

    // Jump back to start of file
    in.seek(0);

    // Map of final output lines, key is time
    QMap<quint64, QStringList> timestampMap;

    // Run through the whole file and fill map of timestamps
    while (!in.atEnd()) {
        QStringList newLine = in.readLine().split(delimiter);
        quint64 timestamp = newLine.at(0).toULongLong();

        // Check if timestamp does exist - if not, add it
        if (!timestampMap.contains(timestamp)) {
            timestampMap.insert(timestamp, templateList);
        }

        QStringList list = timestampMap.value(timestamp);

        QString currentDataName = newLine.at(2);
        QString currentDataValue = newLine.at(3);
        list.replace(messageMap.value(currentDataName), currentDataValue);
        timestampMap.insert(timestamp, list);
    }

    int lineCounter = 0;

    QStringList lastList = timestampMap.values().at(1);

    foreach (QStringList list, timestampMap.values()) {
        // Write this current time set out to the file
        // only do so from the 2nd line on, since the first
        // line could be incomplete
        if (lineCounter > 1) {
            // Set the timestamp
            list.replace(0,QString("%1").arg(timestampMap.keys().at(lineCounter)));

            // Fill holes if necessary
            if (holeFillingEnabled) {
                int index = 0;
                foreach (const QString& str, list) {
                    if (str == "" || str == "NaN") {
                        list.replace(index, lastList.at(index));
                    }
                    index++;
                }
            }

            // Set last list
            lastList = list;

            // Write data columns
            QString output = list.join(delimiter) + "\n";
            outTmpFile.write(output.toLocal8Bit());
        }
        lineCounter++;
    }

	// We're now done with the source file
	infile.close();

	// Clean up and update the status before we return.
	currentDataLine = 0;
	emit finishedFile(outFileName);
	running = false;
}

/**
 * @param holeFilling If hole filling is enabled, the compressor tries to fill empty data fields with previous
 * values from the same variable (or NaN, if no previous value existed)
 */
void LogCompressor::startCompression(bool holeFilling)
{
	holeFillingEnabled = holeFilling;
	start();
}

bool LogCompressor::isFinished() const
{
	return !running;
}

int LogCompressor::getCurrentLine() const
{
	return currentDataLine;
}


void LogCompressor::_signalCriticalError(const QString& msg)
{
    emit logProcessingCriticalError(tr("Log Compressor"), msg);
}
