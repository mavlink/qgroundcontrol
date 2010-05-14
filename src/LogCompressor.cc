#include <QFile>
#include <QTextStream>
#include <QStringList>
#include "LogCompressor.h"

#include <QDebug>

LogCompressor::LogCompressor(QString logFileName, int uasid) :
        logFileName(logFileName),
        uasid(uasid)
{
    start();
}

void LogCompressor::run()
{
    QString separator = "\t";
    QString fileName = logFileName;
    QFile file(fileName);
    QStringList* keys = new QStringList();
    QStringList* times = new QStringList();

    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
        return;

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

    qDebug() << header;

    qDebug() << "NOW READING TIMES";

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
    while (!data.atEnd()) {
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
        quint64 index = times->indexOf(time);
        QString outLine = outLines->at(index);
        QStringList outParts = outLine.split(separator);
        // Replace measurement placeholder with current value
        outParts.replace(keys->indexOf(field)+1, value);
        outLine = outParts.join(separator);
        outLines->replace(index, outLine);
    }



    // Add header, write out file
    file.close();
    QFile::remove(file.fileName());
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
        return;
    file.write(QString(QString("unix_timestamp") + separator + header.replace(" ", "_") + QString("\n")).toLatin1());
    //QString fileHeader = QString("unix_timestamp") + header.replace(" ", "_") + QString("\n");

    // Debug output
    for (int i = 0; i < outLines->length(); i++)
    {
        //qDebug() << outLines->at(i);
        file.write(QString(outLines->at(i) + "\n").toLatin1());

    }

    delete keys;
    qDebug() << "Done with logfile processing";
}
