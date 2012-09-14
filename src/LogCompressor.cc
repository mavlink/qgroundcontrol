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
 *   @brief Implementation of class LogCompressor. This class reads in a file containing messages and translates it into a tab-delimited CSV file.
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>
#include <QList>
#include "LogCompressor.h"

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
}

void LogCompressor::run()
{
	// Verify that the input file is useable
	QFile infile(logFileName);
	if (!infile.exists() || !infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		emit logProcessingStatusChanged(tr("Log Compressor: Cannot start/compress log file, since input file %1 is not readable").arg(QFileInfo(infile.fileName()).absoluteFilePath()));
		return;
	}

//    outFileName = logFileName;

    QString outFileName;

    QStringList parts =  QFileInfo(infile.fileName()).absoluteFilePath().split(".", QString::SkipEmptyParts);

    parts.replace(parts.size()-2, "compressed." + parts.last());
    outFileName = parts.join(".");

	// Verify that the output file is useable
    QFile outTmpFile(outFileName);
    if (!outTmpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		emit logProcessingStatusChanged(tr("Log Compressor: Cannot start/compress log file, since output file %1 is not writable").arg(QFileInfo(outTmpFile.fileName()).absoluteFilePath()));
		return;
	}


	// First we search the input file through keySearchLimit number of lines
	// looking for variables. This is neccessary before CSV files require
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
	outTmpFile.write(headerLine.toLocal8Bit());

	emit logProcessingStatusChanged(tr("Log compressor: Dataset contains dimension: ") + headerLine);

	// Reset our position in the input file before we start the main processing loop.
	infile.reset();
	in.reset();
	in.resetStatus();

	// Template list stores a list for populating with data as it's parsed from messages.
	QStringList templateList;
	for (int i = 0; i < headerList.size() + 1; ++i) {
		templateList << (holeFillingEnabled?"NaN":"");
	}
	QStringList filledList(templateList);
	QStringList currentLine = in.readLine().split(delimiter);
	currentDataLine = 1;
	while (!in.atEnd()) {
		// We only overwrite data from the last time set if we aren't doing a zero-order hold
		if (!holeFillingEnabled) {
			filledList = templateList;
		}
		// Populate this time set with the data from this first message
		filledList.replace(0, currentLine.at(0));
		filledList.replace(messageMap.value(currentLine.at(2)), currentLine.at(3));

		// Continue searching for messages in the same time set and adding that data
		// to the current time set if appropriate.
		while (!in.atEnd()) {
			QStringList newLine = in.readLine().split(delimiter);
			++currentDataLine;

			if (newLine.at(0) == currentLine.at(0)) {
				QString currentDataName = newLine.at(2);
				QString currentDataValue = newLine.at(3);
				filledList.replace(messageMap.value(currentDataName), currentDataValue);
			} else {
				currentLine = newLine;
				break;
			}
		}

		// Write this current time set out to the file
		QString output = filledList.join(delimiter) + "\n";
		outTmpFile.write(output.toLocal8Bit());
	}

	// We're now done with the source file
	infile.close();

	// Make sure we remove the source file before replacing it.
//	QFile::remove(outFileName);
//	outTmpFile.copy(outFileName);
//	outTmpFile.close();
    emit logProcessingStatusChanged(tr("Log Compressor: Writing output to file %1").arg(QFileInfo(outFileName).absoluteFilePath()));

	// Clean up and update the status before we return.
	currentDataLine = 0;
    emit logProcessingStatusChanged(tr("Log compressor: Finished processing file: %1").arg(outFileName));
	emit finishedFile(outFileName);
	qDebug() << "Done with logfile processing";
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

bool LogCompressor::isFinished()
{
	return !running;
}

int LogCompressor::getCurrentLine()
{
	return currentDataLine;
}
