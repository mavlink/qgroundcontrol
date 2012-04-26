#ifndef LOGCOMPRESSOR_H
#define LOGCOMPRESSOR_H

#include <QThread>

class LogCompressor : public QThread
{
    Q_OBJECT
public:
    /** @brief Create the log compressor. It will only get active upon calling startCompression() */
    LogCompressor(QString logFileName, QString outFileName="", QString delimiter="\t");
    /** @brief Start the compression of a raw, line-based logfile into a CSV file */
    void startCompression(bool holeFilling=false);
    bool isFinished();
    int getCurrentLine();

protected:
    void run();                     ///< This function actually performs the compression. It's an overloaded function from QThread
    QString logFileName;            ///< The input file name.
    QString outFileName;            ///< The output file name. If blank defaults to logFileName
    bool running;                   ///< True when the startCompression() function is operating.
    int currentDataLine;            ///< The current line of data that is being processed. Only relevant when running==true
    QString delimiter;              ///< Delimiter between fields in the output file. Defaults to tab ('\t')
    bool holeFillingEnabled;        ///< Enables the filling of holes in the dataset with the previous value (or NaN if none exists)

signals:
    /** @brief This signal is emitted when there is a change in the status of the parsing algorithm. For instance if an error is encountered.
     * @param status A status message
     */
    void logProcessingStatusChanged(QString status);

    /** @brief This signal is emitted once a logfile has been finished writing
     * @param fileName The name of the output (CSV) file
     */
    void finishedFile(QString fileName);
};

#endif // LOGCOMPRESSOR_H