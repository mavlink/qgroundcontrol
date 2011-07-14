/**
 ******************************************************************************
 *
 * @file       synchronousprocess.cpp
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

#include "synchronousprocess.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QTextCodec>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <QtGui/QApplication>

#include <limits.h>

enum { debug = 0 };

enum { defaultMaxHangTimerCount = 10 };

namespace Utils {

// ----------- SynchronousProcessResponse
SynchronousProcessResponse::SynchronousProcessResponse() :
   result(StartFailed),
   exitCode(-1)
{
}

void SynchronousProcessResponse::clear()
{
    result = StartFailed;
    exitCode = -1;
    stdOut.clear();
    stdErr.clear();
}

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const SynchronousProcessResponse& r)
{
    QDebug nsp = str.nospace();
    nsp << "SynchronousProcessResponse: result=" << r.result << " ex=" << r.exitCode << '\n'
        << r.stdOut.size() << " bytes stdout, stderr=" << r.stdErr << '\n';
    return str;
}

// Data for one channel buffer (stderr/stdout)
struct ChannelBuffer {
    ChannelBuffer();
    void clearForRun();
    QByteArray linesRead();

    QByteArray data;
    bool firstData;
    bool bufferedSignalsEnabled;
    bool firstBuffer;
    int bufferPos;
};

ChannelBuffer::ChannelBuffer() :
    firstData(true),
    bufferedSignalsEnabled(false),
    firstBuffer(true),
    bufferPos(0)
{
}

void ChannelBuffer::clearForRun()
{
    firstData = true;
    firstBuffer = true;
    bufferPos = 0;
}

/* Check for complete lines read from the device and return them, moving the
 * buffer position. This is based on the assumption that '\n' is the new line
 * marker in any sane codec. */
QByteArray ChannelBuffer::linesRead()
{
    // Any new lines?
    const int lastLineIndex = data.lastIndexOf('\n');
    if (lastLineIndex == -1 || lastLineIndex <= bufferPos)
        return QByteArray();
    const int nextBufferPos = lastLineIndex + 1;
    const QByteArray lines = data.mid(bufferPos, nextBufferPos - bufferPos);
    bufferPos = nextBufferPos;
    return lines;
}

// ----------- SynchronousProcessPrivate
struct SynchronousProcessPrivate {
    SynchronousProcessPrivate();
    void clearForRun();

    QTextCodec *m_stdOutCodec;
    QProcess m_process;
    QTimer m_timer;
    QEventLoop m_eventLoop;
    SynchronousProcessResponse m_result;
    int m_hangTimerCount;
    int m_maxHangTimerCount;
    bool m_startFailure;

    ChannelBuffer m_stdOut;
    ChannelBuffer m_stdErr;
};

SynchronousProcessPrivate::SynchronousProcessPrivate() :
    m_stdOutCodec(0),
    m_hangTimerCount(0),
    m_maxHangTimerCount(defaultMaxHangTimerCount),
    m_startFailure(false)
{
}

void SynchronousProcessPrivate::clearForRun()
{
    m_hangTimerCount = 0;
    m_stdOut.clearForRun();
    m_stdErr.clearForRun();
    m_result.clear();
    m_startFailure = false;
}

// ----------- SynchronousProcess
SynchronousProcess::SynchronousProcess() :
    m_d(new SynchronousProcessPrivate)
{
    m_d->m_timer.setInterval(1000);
    connect(&m_d->m_timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
    connect(&m_d->m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
    connect(&m_d->m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(&m_d->m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(stdOutReady()));
    connect(&m_d->m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(stdErrReady()));
}

SynchronousProcess::~SynchronousProcess()
{
    delete m_d;
}

void SynchronousProcess::setTimeout(int timeoutMS)
{
    if (timeoutMS >= 0) {
        m_d->m_maxHangTimerCount = qMax(2, timeoutMS / 1000);
    } else {
        m_d->m_maxHangTimerCount = INT_MAX;
    }
}

int SynchronousProcess::timeout() const
{
    return m_d->m_maxHangTimerCount == INT_MAX ? -1 : 1000 * m_d->m_maxHangTimerCount;
}

void SynchronousProcess::setStdOutCodec(QTextCodec *c)
{
    m_d->m_stdOutCodec = c;
}

QTextCodec *SynchronousProcess::stdOutCodec() const
{
    return m_d->m_stdOutCodec;
}

bool SynchronousProcess::stdOutBufferedSignalsEnabled() const
{
    return m_d->m_stdOut.bufferedSignalsEnabled;
}

void SynchronousProcess::setStdOutBufferedSignalsEnabled(bool v)
{
    m_d->m_stdOut.bufferedSignalsEnabled = v;
}

bool SynchronousProcess::stdErrBufferedSignalsEnabled() const
{
    return m_d->m_stdErr.bufferedSignalsEnabled;
}

void SynchronousProcess::setStdErrBufferedSignalsEnabled(bool v)
{
    m_d->m_stdErr.bufferedSignalsEnabled = v;
}

QStringList SynchronousProcess::environment() const
{
    return m_d->m_process.environment();
}

void SynchronousProcess::setEnvironment(const QStringList &e)
{
    m_d->m_process.setEnvironment(e);
}

void SynchronousProcess::setWorkingDirectory(const QString &workingDirectory)
{
    m_d->m_process.setWorkingDirectory(workingDirectory);
}

QString SynchronousProcess::workingDirectory() const
{
    return m_d->m_process.workingDirectory();
}

QProcess::ProcessChannelMode SynchronousProcess::processChannelMode () const
{
    return m_d->m_process.processChannelMode();
}

void SynchronousProcess::setProcessChannelMode(QProcess::ProcessChannelMode m)
{
    m_d->m_process.setProcessChannelMode(m);
}

SynchronousProcessResponse SynchronousProcess::run(const QString &binary,
                                                 const QStringList &args)
{
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << binary << args;

    m_d->clearForRun();

    // On Windows, start failure is triggered immediately if the
    // executable cannot be found in the path. Do not start the
    // event loop in that case.
    m_d->m_process.start(binary, args, QIODevice::ReadOnly);
    if (!m_d->m_startFailure) {
        m_d->m_timer.start();
        QApplication::setOverrideCursor(Qt::WaitCursor);
        m_d->m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
        if (m_d->m_result.result == SynchronousProcessResponse::Finished || m_d->m_result.result == SynchronousProcessResponse::FinishedError) {
            processStdOut(false);
            processStdErr(false);
        }

        m_d->m_result.stdOut = convertStdOut(m_d->m_stdOut.data);
        m_d->m_result.stdErr = convertStdErr(m_d->m_stdErr.data);

        m_d->m_timer.stop();
        QApplication::restoreOverrideCursor();
    }

    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << binary << m_d->m_result;
    return  m_d->m_result;
}

void SynchronousProcess::slotTimeout()
{
    if (++m_d->m_hangTimerCount > m_d->m_maxHangTimerCount) {
        if (debug)
            qDebug() << Q_FUNC_INFO << "HANG detected, killing";
        m_d->m_process.kill();
        m_d->m_result.result = SynchronousProcessResponse::Hang;
    } else {
        if (debug)
            qDebug() << Q_FUNC_INFO << m_d->m_hangTimerCount;
    }
}

void SynchronousProcess::finished(int exitCode, QProcess::ExitStatus e)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << exitCode << e;
    m_d->m_hangTimerCount = 0;
    switch (e) {
    case QProcess::NormalExit:
        m_d->m_result.result = exitCode ? SynchronousProcessResponse::FinishedError : SynchronousProcessResponse::Finished;
        m_d->m_result.exitCode = exitCode;
        break;
    case QProcess::CrashExit:
        // Was hang detected before and killed?
        if (m_d->m_result.result != SynchronousProcessResponse::Hang)
            m_d->m_result.result = SynchronousProcessResponse::TerminatedAbnormally;
        m_d->m_result.exitCode = -1;
        break;
    }
    m_d->m_eventLoop.quit();
}

void SynchronousProcess::error(QProcess::ProcessError e)
{
    m_d->m_hangTimerCount = 0;
    if (debug)
        qDebug() << Q_FUNC_INFO << e;
    // Was hang detected before and killed?
    if (m_d->m_result.result != SynchronousProcessResponse::Hang)
        m_d->m_result.result = SynchronousProcessResponse::StartFailed;
    m_d->m_startFailure = true;
    m_d->m_eventLoop.quit();
}

void SynchronousProcess::stdOutReady()
{
    m_d->m_hangTimerCount = 0;
    processStdOut(true);
}

void SynchronousProcess::stdErrReady()
{
    m_d->m_hangTimerCount = 0;
    processStdErr(true);
}

QString SynchronousProcess::convertStdErr(const QByteArray &ba)
{
    return QString::fromLocal8Bit(ba.constData(), ba.size()).remove(QLatin1Char('\r'));
}

QString SynchronousProcess::convertStdOut(const QByteArray &ba) const
{
    QString stdOut = m_d->m_stdOutCodec ? m_d->m_stdOutCodec->toUnicode(ba) : QString::fromLocal8Bit(ba.constData(), ba.size());
    return stdOut.remove(QLatin1Char('\r'));
}

void SynchronousProcess::processStdOut(bool emitSignals)
{
    // Handle binary data
    const QByteArray ba = m_d->m_process.readAllStandardOutput();
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << emitSignals << ba;
    if (!ba.isEmpty()) {
        m_d->m_stdOut.data += ba;
        if (emitSignals) {
            // Emit binary signals
            emit stdOut(ba, m_d->m_stdOut.firstData);
            m_d->m_stdOut.firstData = false;
            // Buffered. Emit complete lines?
            if (m_d->m_stdOut.bufferedSignalsEnabled) {
                const QByteArray lines = m_d->m_stdOut.linesRead();
                if (!lines.isEmpty()) {
                    emit stdOutBuffered(convertStdOut(lines), m_d->m_stdOut.firstBuffer);
                    m_d->m_stdOut.firstBuffer = false;
                }
            }
        }
    }
}

void SynchronousProcess::processStdErr(bool emitSignals)
{
    // Handle binary data
    const QByteArray ba = m_d->m_process.readAllStandardError();
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << emitSignals << ba;
    if (!ba.isEmpty()) {
        m_d->m_stdErr.data += ba;
        if (emitSignals) {
            // Emit binary signals
            emit stdErr(ba, m_d->m_stdErr.firstData);
            m_d->m_stdErr.firstData = false;
            if (m_d->m_stdErr.bufferedSignalsEnabled) {
                // Buffered. Emit complete lines?
                const QByteArray lines = m_d->m_stdErr.linesRead();
                if (!lines.isEmpty()) {
                    emit stdErrBuffered(convertStdErr(lines), m_d->m_stdErr.firstBuffer);
                    m_d->m_stdErr.firstBuffer = false;
                }
            }
        }
    }
}

// Path utilities

enum  OS_Type { OS_Mac, OS_Windows, OS_Unix };

#ifdef Q_OS_WIN
static const OS_Type pathOS = OS_Windows;
#else
#  ifdef Q_OS_MAC
static const OS_Type pathOS = OS_Mac;
#  else
static const OS_Type pathOS = OS_Unix;
#  endif
#endif

// Locate a binary in a directory, applying all kinds of
// extensions the operating system supports.
static QString checkBinary(const QDir &dir, const QString &binary)
{
    // naive UNIX approach
    const QFileInfo info(dir.filePath(binary));
    if (info.isFile() && info.isExecutable())
        return info.absoluteFilePath();

    // Does the OS have some weird extension concept or does the
    // binary have a 3 letter extension?
    if (pathOS == OS_Unix)
        return QString();
    const int dotIndex = binary.lastIndexOf(QLatin1Char('.'));
    if (dotIndex != -1 && dotIndex == binary.size() - 4)
        return  QString();

    switch (pathOS) {
    case OS_Unix:
        break;
    case OS_Windows: {
            static const char *windowsExtensions[] = {".cmd", ".bat", ".exe", ".com" };
            // Check the Windows extensions using the order
            const int windowsExtensionCount = sizeof(windowsExtensions)/sizeof(const char*);
            for (int e = 0; e < windowsExtensionCount; e ++) {
                const QFileInfo windowsBinary(dir.filePath(binary + QLatin1String(windowsExtensions[e])));
                if (windowsBinary.isFile() && windowsBinary.isExecutable())
                    return windowsBinary.absoluteFilePath();
            }
        }
        break;
    case OS_Mac: {
            // Check for Mac app folders
            const QFileInfo appFolder(dir.filePath(binary + QLatin1String(".app")));
            if (appFolder.isDir()) {
                QString macBinaryPath = appFolder.absoluteFilePath();
                macBinaryPath += QLatin1String("/Contents/MacOS/");
                macBinaryPath += binary;
                const QFileInfo macBinary(macBinaryPath);
                if (macBinary.isFile() && macBinary.isExecutable())
                    return macBinary.absoluteFilePath();
            }
        }
        break;
    }
    return QString();
}

QString SynchronousProcess::locateBinary(const QString &path, const QString &binary)
{
    // Absolute file?
    const QFileInfo absInfo(binary);
    if (absInfo.isAbsolute())
        return checkBinary(absInfo.dir(), absInfo.fileName());

    // Windows finds binaries  in the current directory
    if (pathOS == OS_Windows) {
        const QString currentDirBinary = checkBinary(QDir::current(), binary);
        if (!currentDirBinary.isEmpty())
            return currentDirBinary;
    }

    const QStringList paths = path.split(pathSeparator());
    if (paths.empty())
        return QString();
    const QStringList::const_iterator cend = paths.constEnd();
    for (QStringList::const_iterator it = paths.constBegin(); it != cend; ++it) {
        const QDir dir(*it);
        const QString rc = checkBinary(dir, binary);
        if (!rc.isEmpty())
            return rc;
    }
    return QString();
}

QString SynchronousProcess::locateBinary(const QString &binary)
{
    const QByteArray path = qgetenv("PATH");
    return locateBinary(QString::fromLocal8Bit(path), binary);
}

QChar SynchronousProcess::pathSeparator()
{
    if (pathOS == OS_Windows)
        return QLatin1Char(';');
    return QLatin1Char(':');
}

} // namespace Utils
