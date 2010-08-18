#ifndef LOGCOMPRESSOR_H
#define LOGCOMPRESSOR_H

#include <QThread>

class LogCompressor : public QThread
{
public:
    LogCompressor(QString logFileName, QString outFileName="", int uasid = 0);
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
};

#endif // LOGCOMPRESSOR_H
