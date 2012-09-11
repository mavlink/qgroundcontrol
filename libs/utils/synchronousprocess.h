/**
 ******************************************************************************
 *
 * @file       synchronousprocess.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief      
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   
 * @{
 * 
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef SYNCHRONOUSPROCESS_H
#define SYNCHRONOUSPROCESS_H

#include "utils_global.h"

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QTextCodec;
class QDebug;
class QByteArray;
QT_END_NAMESPACE

namespace Utils {

struct SynchronousProcessPrivate;

/* Result of SynchronousProcess execution */
struct QTCREATOR_UTILS_EXPORT SynchronousProcessResponse
{
    enum Result {
        // Finished with return code 0
        Finished,
        // Finished with return code != 0
        FinishedError,
        // Process terminated abnormally (kill)
        TerminatedAbnormally,
        // Executable could not be started
        StartFailed,
        // Hang, no output after time out
        Hang };

    SynchronousProcessResponse();
    void clear();

    Result result;
    int exitCode;
    QString stdOut;
    QString stdErr;
};

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const SynchronousProcessResponse &);

/* SynchronousProcess: Runs a synchronous process in its own event loop
 * that blocks only user input events. Thus, it allows for the gui to
 * repaint and append output to log windows.
 *
 * The stdOut(), stdErr() signals are emitted unbuffered as the process
 * writes them.
 *
 * The stdOutBuffered(), stdErrBuffered() signals are emitted with complete
 * lines based on the '\n' marker if they are enabled using
 * stdOutBufferedSignalsEnabled()/setStdErrBufferedSignalsEnabled().
 * They would typically be used for log windows. */

class QTCREATOR_UTILS_EXPORT SynchronousProcess : public QObject
{
    Q_OBJECT
public:
    SynchronousProcess();
    virtual ~SynchronousProcess();

    /* Timeout for hanging processes (no reaction on stderr/stdout)*/
    void setTimeout(int timeoutMS);
    int timeout() const;

    void setStdOutCodec(QTextCodec *c);
    QTextCodec *stdOutCodec() const;

    QProcess::ProcessChannelMode processChannelMode () const;
    void setProcessChannelMode(QProcess::ProcessChannelMode m);

    bool stdOutBufferedSignalsEnabled() const;
    void setStdOutBufferedSignalsEnabled(bool);

    bool stdErrBufferedSignalsEnabled() const;
    void setStdErrBufferedSignalsEnabled(bool);

    QStringList environment() const;
    void setEnvironment(const QStringList &);

    void setWorkingDirectory(const QString &workingDirectory);
    QString workingDirectory() const;

    SynchronousProcessResponse run(const QString &binary, const QStringList &args);

    // Helpers to find binaries. Do not use it for other path variables
    // and file types.
    static QString locateBinary(const QString &binary);
    static QString locateBinary(const QString &path, const QString &binary);
    static QChar pathSeparator();

signals:
    void stdOut(const QByteArray &data, bool firstTime);
    void stdErr(const QByteArray &data, bool firstTime);

    void stdOutBuffered(const QString &data, bool firstTime);
    void stdErrBuffered(const QString &data, bool firstTime);

private slots:
    void slotTimeout();
    void finished(int exitCode, QProcess::ExitStatus e);
    void error(QProcess::ProcessError);
    void stdOutReady();
    void stdErrReady();

private:
    void processStdOut(bool emitSignals);
    void processStdErr(bool emitSignals);
    static QString convertStdErr(const QByteArray &);
    QString convertStdOut(const QByteArray &) const;

    SynchronousProcessPrivate *m_d;
};

} // namespace Utils

#endif
