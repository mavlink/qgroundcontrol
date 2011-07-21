#ifndef LOGCOMPRESSOR_H
#define LOGCOMPRESSOR_H

#include <QThread>

class LogCompressor : public QThread
{
    Q_OBJECT
public:
    /** @brief Create the log compressor. It will only get active upon calling startCompression() */
    LogCompressor(QString logFileName, QString outFileName="", int uasid = 0);
    /** @brief Start the compression of a raw, line-based logfile into a CSV file */
    void startCompression(bool holeFilling=false);
    bool isFinished();
    int getDataLines();
    int getCurrentLine();

protected:
    void run();
    QString logFileName;
    QString outFileName;
    bool running;
    int currentDataLine;
    int dataLines;
    int uasid;
    bool holeFillingEnabled;       ///< Enables the filling of holes in the dataset with the previous value (or NaN if none exists)

signals:
    /** @brief This signal is emitted once a logfile has been finished writing
     * @param fileName The name out the output (CSV) file
     */
    void logProcessingStatusChanged(QString);
    void finishedFile(QString fileName);
};

#endif // LOGCOMPRESSOR_H
