/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Implementation of class MAVLinkXMLParserV10
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include <QFile>
#include <QDir>
#include <QPair>
#include <QList>
#include <QMap>
#include <QDateTime>
#include <QLocale>
#include <QApplication>

#include "MAVLinkXMLParserV10.h"

#include <QDebug>

MAVLinkXMLParserV10::MAVLinkXMLParserV10(QDomDocument* document, QString outputDirectory, QObject* parent) : QObject(parent),
doc(document),
outputDirName(outputDirectory),
fileName("")
{
}

MAVLinkXMLParserV10::MAVLinkXMLParserV10(QString document, QString outputDirectory, QObject* parent) : QObject(parent)
{
    doc = new QDomDocument();
    QFile file(document);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString instanceText(QString::fromUtf8(file.readAll()));
        doc->setContent(instanceText);
    }
    fileName = document;
    outputDirName = outputDirectory;
}

MAVLinkXMLParserV10::~MAVLinkXMLParserV10()
{
}

void MAVLinkXMLParserV10::processError(QProcess::ProcessError err)
{
    switch(err)
    {
    case QProcess::FailedToStart:
        emit parseState(tr("Generator failed to start. Please check if the path and command is correct."));
        break;
    case QProcess::Crashed:
        emit parseState("Generator crashed, This is a generator-related problem. Please upgrade MAVLink generator.");
        break;
    case QProcess::Timedout:
        emit parseState(tr("Generator start timed out, please check if the path and command are correct"));
        break;
    case QProcess::WriteError:
        emit parseState(tr("Could not communicate with generator. Please check if the path and command are correct"));
        break;
    case QProcess::ReadError:
        emit parseState(tr("Could not communicate with generator. Please check if the path and command are correct"));
        break;
    case QProcess::UnknownError:
    default:
        emit parseState(tr("Generator error. Please check if the path and command is correct."));
        break;
    }
}

/**
 * Generate C-code (C-89 compliant) out of the XML protocol specs.
 */
bool MAVLinkXMLParserV10::generate()
{
    emit parseState(tr("Generator ready."));
#ifdef Q_OS_WIN
    QString generatorCall("files/mavlink_generator/generator/mavgen.exe");
#endif
#if (defined Q_OS_MAC) || (defined Q_OS_LINUX)
    QString generatorCall("python");
#endif
    QString lang("C");
    QString version("1.0");

    QStringList arguments;
#if (defined Q_OS_MAC) || (defined Q_OS_LINUX)
    // Script is only needed as argument if Python is used, the Py2Exe implicitely knows the script
    arguments << QString("%1/files/mavlink_generator/generator/mavgen.py").arg(QApplication::applicationDirPath());
#endif
    arguments << QString("--lang=%1").arg(lang);
    arguments << QString("--output=%2").arg(outputDirName);
    arguments << QString("%3").arg(fileName);
    arguments << QString("--wire-protocol=%4").arg(version);

    qDebug() << "Attempted to start" << generatorCall << arguments;
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStdOut()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readStdErr()));
    process->start(generatorCall, arguments, QIODevice::ReadWrite);
    QString output = QString(process->readAll());
    emit parseState(output);
    // Print process status
    emit parseState(QString("<font color=\"red\">%1</font>").arg(QString(process->readAllStandardError())));
    emit parseState(QString(process->readAllStandardOutput()));

    process->waitForFinished(20000);

    process->terminate();
    process->kill();
    return true;//result;
}

void MAVLinkXMLParserV10::readStdOut()
{
    QString out(process->readAllStandardOutput());
    emit parseState(out);
}

void MAVLinkXMLParserV10::readStdErr()
{
    QString out = QString("<font color=\"red\">%1</font>").arg(QString(process->readAllStandardError()));
    emit parseState(out);
}
