/**
 ******************************************************************************
 *
 * @file       consoleprocess_win.cpp
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
#include "winutils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QAbstractEventDispatcher>
#include "qwineventnotifier_p.h"

#include <QtNetwork/QLocalSocket>

#include <stdlib.h>

using namespace Utils;

ConsoleProcess::ConsoleProcess(QObject *parent) :
    QObject(parent),
    m_mode(Run),
    m_appPid(0),
    m_stubSocket(0),
    m_tempFile(0),
    m_pid(0),
    m_hInferior(NULL),
    inferiorFinishedNotifier(0),
    processFinishedNotifier(0)
{
    connect(&m_stubServer, SIGNAL(newConnection()), SLOT(stubConnectionAvailable()));
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
        QTextStream out(m_tempFile);
        out.setCodec("UTF-16LE");
        out.setGenerateByteOrderMark(false);
        foreach (const QString &var, fixWinEnvironment(environment()))
            out << var << QChar(0);
        out << QChar(0);
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    m_pid = new PROCESS_INFORMATION;
    ZeroMemory(m_pid, sizeof(PROCESS_INFORMATION));

    QString workDir = QDir::toNativeSeparators(workingDirectory());
    if (!workDir.isEmpty() && !workDir.endsWith('\\'))
        workDir.append('\\');

    QStringList stubArgs;
    stubArgs << modeOption(m_mode)
             << m_stubServer.fullServerName()
             << workDir
             << (m_tempFile ? m_tempFile->fileName() : 0)
             << createWinCommandline(program, args)
             << msgPromptToClose();

    const QString cmdLine = createWinCommandline(
            QCoreApplication::applicationDirPath() + QLatin1String("/qtcreator_process_stub.exe"), stubArgs);

    bool success = CreateProcessW(0, (WCHAR*)cmdLine.utf16(),
                                  0, 0, FALSE, CREATE_NEW_CONSOLE,
                                  0, 0,
                                  &si, m_pid);

    if (!success) {
        delete m_pid;
        m_pid = 0;
        delete m_tempFile;
        m_tempFile = 0;
        stubServerShutdown();
        emit processError(tr("The process '%1' could not be started: %2").arg(cmdLine, winErrorMessage(GetLastError())));
        return false;
    }

    processFinishedNotifier = new QWinEventNotifier(m_pid->hProcess, this);
    connect(processFinishedNotifier, SIGNAL(activated(HANDLE)), SLOT(stubExited()));
    emit wrapperStarted();
    return true;
}

void ConsoleProcess::stop()
{
    if (m_hInferior != NULL) {
        TerminateProcess(m_hInferior, (unsigned)-1);
        cleanupInferior();
    }
    if (m_pid) {
        TerminateProcess(m_pid->hProcess, (unsigned)-1);
        WaitForSingleObject(m_pid->hProcess, INFINITE);
        cleanupStub();
    }
}

bool ConsoleProcess::isRunning() const
{
    return m_pid != 0;
}

QString ConsoleProcess::stubServerListen()
{
    if (m_stubServer.listen(QString::fromLatin1("creator-%1-%2")
                            .arg(QCoreApplication::applicationPid())
                            .arg(rand())))
        return QString();
    return m_stubServer.errorString();
}

void ConsoleProcess::stubServerShutdown()
{
    delete m_stubSocket;
    m_stubSocket = 0;
    if (m_stubServer.isListening())
        m_stubServer.close();
}

void ConsoleProcess::stubConnectionAvailable()
{
    m_stubSocket = m_stubServer.nextPendingConnection();
    connect(m_stubSocket, SIGNAL(readyRead()), SLOT(readStubOutput()));
}

void ConsoleProcess::readStubOutput()
{
    while (m_stubSocket->canReadLine()) {
        QByteArray out = m_stubSocket->readLine();
        out.chop(2); // \r\n
        if (out.startsWith("err:chdir ")) {
            emit processError(msgCannotChangeToWorkDir(workingDirectory(), winErrorMessage(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emit processError(msgCannotExecute(m_executable, winErrorMessage(out.mid(9).toInt())));
        } else if (out.startsWith("pid ")) {
            // Wil not need it any more
            delete m_tempFile;
            m_tempFile = 0;

            m_appPid = out.mid(4).toInt();
            m_hInferior = OpenProcess(
                    SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,
                    FALSE, m_appPid);
            if (m_hInferior == NULL) {
                emit processError(tr("Cannot obtain a handle to the inferior: %1")
                                  .arg(winErrorMessage(GetLastError())));
                // Uhm, and now what?
                continue;
            }
            inferiorFinishedNotifier = new QWinEventNotifier(m_hInferior, this);
            connect(inferiorFinishedNotifier, SIGNAL(activated(HANDLE)), SLOT(inferiorExited()));
            emit processStarted();
        } else {
            emit processError(msgUnexpectedOutput());
            TerminateProcess(m_pid->hProcess, (unsigned)-1);
            break;
        }
    }
}

void ConsoleProcess::cleanupInferior()
{
    delete inferiorFinishedNotifier;
    inferiorFinishedNotifier = 0;
    CloseHandle(m_hInferior);
    m_hInferior = NULL;
    m_appPid = 0;
}

void ConsoleProcess::inferiorExited()
{
    DWORD chldStatus;

    if (!GetExitCodeProcess(m_hInferior, &chldStatus))
        emit processError(tr("Cannot obtain exit status from inferior: %1")
                          .arg(winErrorMessage(GetLastError())));
    cleanupInferior();
    m_appStatus = QProcess::NormalExit;
    m_appCode = chldStatus;
    emit processStopped();
}

void ConsoleProcess::cleanupStub()
{
    stubServerShutdown();
    delete processFinishedNotifier;
    processFinishedNotifier = 0;
    CloseHandle(m_pid->hThread);
    CloseHandle(m_pid->hProcess);
    delete m_pid;
    m_pid = 0;
    delete m_tempFile;
    m_tempFile = 0;
}

void ConsoleProcess::stubExited()
{
    // The stub exit might get noticed before we read the pid for the kill.
    if (m_stubSocket && m_stubSocket->state() == QLocalSocket::ConnectedState)
        m_stubSocket->waitForDisconnected();
    cleanupStub();
    if (m_hInferior != NULL) {
        TerminateProcess(m_hInferior, (unsigned)-1);
        cleanupInferior();
        m_appStatus = QProcess::CrashExit;
        m_appCode = -1;
        emit processStopped();
    }
    emit wrapperStopped();
}

