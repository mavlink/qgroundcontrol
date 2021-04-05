/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QThread>

/**
 * @file
 *   @brief Declaration of class LogCompressor.
 *          This class reads in a file containing messages and translates it into a tab-delimited CSV file.
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 */

class LogCompressor : public QThread
{
    Q_OBJECT
public:
    /** @brief Create the log compressor. It will only get active upon calling startCompression() */
    LogCompressor(QString logFileName, QString outFileName="", QString delimiter="\t");
    /** @brief Start the compression of a raw, line-based logfile into a CSV file */
    void startCompression(bool holeFilling=false);
    bool isFinished() const;
    int getCurrentLine() const;

protected:
    void run();                     ///< This function actually performs the compression. It's an overloaded function from QThread
    QString logFileName;            ///< The input file name.
    QString outFileName;            ///< The output file name. If blank defaults to logFileName
    bool running;                   ///< True when the startCompression() function is operating.
    int currentDataLine;            ///< The current line of data that is being processed. Only relevant when running==true
    QString delimiter;              ///< Delimiter between fields in the output file. Defaults to tab ('\t')
    bool holeFillingEnabled;        ///< Enables the filling of holes in the dataset with the previous value (or NaN if none exists)

signals:
    /** @brief This signal is emitted once a logfile has been finished writing
     * @param fileName The name of the output (CSV) file
     */
    void finishedFile(QString fileName);
    
    /// This signal is connected to QGCApplication::showCriticalMessage to show critical errors which come from the thread.
    /// There is no need for clients to connect to this signal.
    void logProcessingCriticalError(const QString& title, const QString& msg);
    
private:
    void _signalCriticalError(const QString& msg);
    
};
