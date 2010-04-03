#include "Logfile.h"
#include <QDebug>

LogFile::LogFile(UASInterface* uas, QString filename, QString formatString)
{
    this->uas = uas;
    connect(this->uas, SIGNAL(valueChanged(int, QString, double, quint64)), this, SLOT(addValue(int, QString, double, quint64)));
    file = new QFile(filename);
    separator = ",";
    this->formatString = formatString;
    if (file->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        out = new QTextStream(file);
    }
}

LogFile::~LogFile()
{
    out->flush();
    file->close();
    delete out;
    delete file;
}

void LogFile::addValue(int uas, QString id, double value, quint64 timestamp)
{
    //out.atEnd()->append() << separator << value;

    if (formatString == id)
    {

        out->operator <<(timestamp);
        out->operator <<(separator);
        out->operator <<(value);
        out->operator <<("\n");
        out->flush();
    }
}

//std::ofstream markerlog("mavserial_markerlog.txt");
//std::ofstream attitudelog("mavserial_attitudelog.txt");


void LogFile::addValue(QString id, double value)
{
    //out.atEnd()->append() << separator << value;
    //qDebug() << id << value;
}
