#ifndef LOGFILE_H
#define LOGFILE_H

#include <QFile>
#include <QTextStream>
#include <UASInterface.h>

class LogFile : public QObject
{
    Q_OBJECT
public:
    LogFile(UASInterface* uas, QString filename, QString formatString="");
    ~LogFile();

public slots:
    void addValue(QString id, double value);
    void addValue(int uas, QString id, double value, quint64 timestamp);

protected:
    QFile* file;
    QTextStream* out;
    QString separator;
    QString formatString;
    UASInterface* uas;
};

#endif // LOGFILE_H
