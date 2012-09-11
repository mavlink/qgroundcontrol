/**
 ******************************************************************************
 *
 * @file       consoleprocess_unix.cpp
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

#include "consoleprocess.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>

#include <QtNetwork/QLocalSocket>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

using namespace Utils;

ConsoleProcess::ConsoleProcess(QObject *parent)  :
    QObject(parent),
    m_mode(Run),
    m_appPid(0),
    m_stubSocket(0),
    m_settings(0)
{
    connect(&m_stubServer, SIGNAL(newConnection()), SLOT(stubConnectionAvailable()));

    m_process.setProcessChannelMode(QProcess::ForwardedChannels);
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            SLOT(stubExited()));
}

ConsoleProcess::~ConsoleProcess()
{
    stop();
}

bool ConsoleProcess::start(const QString &program, const QStringList &args)
{
    if (isRunning())
        return false;

    const QString err = stubServerListen();
    if (!err.isEmpty()) {
        emit processError(msgCommChannelFailed(err));
        return false;
    }

    if (!environment().isEmpty()) {
        m_tempFile = new QTemporaryFile();
        if (!m_tempFile->open()) {
            stubServerShutdown();
            emit processError(msgCannotCreateTempFile(m_tempFile->errorString()));
            delete m_tempFile;
            m_tempFile = 0;
            return false;
        }
        foreach (const QString &var, environment()) {
            m_tempFile->write(var.toLocal8Bit());
            m_tempFile->write("", 1);
        }
        m_tempFile->flush();
    }

    QStringList xtermArgs = terminalEmulator(m_settings).split(QLatin1Char(' ')); // FIXME: quoting
    xtermArgs
#ifdef Q_OS_MAC
              << (QCoreApplication::applicationDirPath() + QLatin1String("/../Resources/qtcreator_process_stub"))
#else
              << (QCoreApplication::applicationDirPath() + QLatin1String("/qtcreator_process_stub"))
#endif
              << modeOption(m_mode)
              << m_stubServer.fullServerName()
              << msgPromptToClose()
              << workingDirectory()
              << (m_tempFile ? m_tempFile->fileName() : 0)
              << program << args;

    QString xterm = xtermArgs.takeFirst();
    m_process.start(xterm, xtermArgs);
    if (!m_process.waitForStarted()) {
        stubServerShutdown();
        emit processError(tr("Cannot start the terminal emulator '%1'.").arg(xterm));
        delete m_tempFile;
        m_tempFile = 0;
        return false;
    }
    m_executable = program;
    emit wrapperStarted();
    return true;
}

void ConsoleProcess::stop()
{
    if (!isRunning())
        return;
    stubServerShutdown();
    m_appPid = 0;
    m_process.terminate();
    if (!m_process.waitForFinished(1000))
        m_process.kill();
    m_process.waitForFinished();
}

bool ConsoleProcess::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

QString ConsoleProcess::stubServerListen()
{
    // We need to put the socket in a private directory, as some systems simply do not
    // check the file permissions of sockets.
    QString stubFifoDir;
    forever {
        {
            QTemporaryFile tf;
            if (!tf.open())
                return msgCannotCreateTempFile(tf.errorString());
            stubFifoDir = QFile::encodeName(tf.fileName());
        }
        // By now the temp file was deleted again
        m_stubServerDir = QFile::encodeName(stubFifoDir);
        if (!::mkdir(m_stubServerDir.constData(), 0700))
            break;
        if (errno != EEXIST)
            return msgCannotCreateTempDir(stubFifoDir, QString::fromLocal8Bit(strerror(errno)));
    }
    const QString stubServer  = stubFifoDir + "/stub-socket";
    if (!m_stubServer.listen(stubServer)) {
        ::rmdir(m_stubServerDir.constData());
        return tr("Cannot create socket '%1': %2").arg(stubServer, m_stubServer.errorString());
    }
    return QString();
}

void ConsoleProcess::stubServerShutdown()
{
    delete m_stubSocket;
    m_stubSocket = 0;
    if (m_stubServer.isListening()) {
        m_stubServer.close();
        ::rmdir(m_stubServerDir.constData());
    }
}

void ConsoleProcess::stubConnectionAvailable()
{
    m_stubSocket = m_stubServer.nextPendingConnection();
    connect(m_stubSocket, SIGNAL(readyRead()), SLOT(readStubOutput()));
}

static QString errorMsg(int code)
{
    return QString::fromLocal8Bit(strerror(code));
}

void ConsoleProcess::readStubOutput()
{
    while (m_stubSocket->canReadLine()) {
        QByteArray out = m_stubSocket->readLine();
        out.chop(1); // \n
        if (out.startsWith("err:chdir ")) {
            emit processError(msgCannotChangeToWorkDir(workingDirectory(), errorMsg(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emit processError(msgCannotExecute(m_executable, errorMsg(out.mid(9).toInt())));
        } else if (out.startsWith("pid ")) {
            // Will not need it any more
            delete m_tempFile;
            m_tempFile = 0;

            m_appPid = out.mid(4).toInt();
            emit processStarted();
        } else if (out.startsWith("exit ")) {
            m_appStatus = QProcess::NormalExit;
            m_appCode = out.mid(5).toInt();
            m_appPid = 0;
            emit processStopped();
        } else if (out.startsWith("crash ")) {
            m_appStatus = QProcess::CrashExit;
            m_appCode = out.mid(6).toInt();
            m_appPid = 0;
            emit processStopped();
        } else {
            emit processError(msgUnexpectedOutput());
            m_process.terminate();
            break;
        }
    }
}

void ConsoleProcess::stubExited()
{
    // The stub exit might get noticed before we read the error status.
    if (m_stubSocket && m_stubSocket->state() == QLocalSocket::ConnectedState)
        m_stubSocket->waitForDisconnected();
    stubServerShutdown();
    delete m_tempFile;
    m_tempFile = 0;
    if (m_appPid) {
        m_appStatus = QProcess::CrashExit;
        m_appCode = -1;
        m_appPid = 0;
        emit processStopped(); // Maybe it actually did not, but keep state consistent
    }
    emit wrapperStopped();
}

QString ConsoleProcess::defaultTerminalEmulator()
{
// FIXME: enable this once runInTerminal works nicely
#if 0 //def Q_OS_MAC
    return QDir::cleanPath(QCoreApplication::applicationDirPath()
                           + QLatin1String("/../Resources/runInTerminal.command"));
#else
    return QLatin1String("xterm");
#endif
}

QString ConsoleProcess::terminalEmulator(const QSettings *settings)
{
    const QString dflt = defaultTerminalEmulator() + QLatin1String(" -e");
    if (!settings)
        return dflt;
    return settings->value(QLatin1String("General/TerminalEmulator"), dflt).toString();
}

void ConsoleProcess::setTerminalEmulator(QSettings *settings, const QString &term)
{
    return settings->setValue(QLatin1String("General/TerminalEmulator"), term);
}
