#ifndef LOGCOMPRESSOR_H
#define LOGCOMPRESSOR_H

#include <QThread>

class LogCompressor : public QThread
{
public:
    LogCompressor(QString logFileName, int uasid = 0);
protected:
    void run();
    QString logFileName;
    int uasid;
};

#endif // LOGCOMPRESSOR_H
