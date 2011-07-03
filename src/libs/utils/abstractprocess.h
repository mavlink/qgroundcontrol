/**
 ******************************************************************************
 *
 * @file       abstractprocess.h
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

#ifndef ABSTRACTPROCESS_H
#define ABSTRACTPROCESS_H

#include "utils_global.h"

#include <QtCore/QStringList>

namespace Utils {

class QTCREATOR_UTILS_EXPORT AbstractProcess
{
public:
    AbstractProcess() {}
    virtual ~AbstractProcess() {}

    QString workingDirectory() const { return m_workingDir; }
    void setWorkingDirectory(const QString &dir) { m_workingDir = dir; }

    QStringList environment() const { return m_environment; }
    void setEnvironment(const QStringList &env) { m_environment = env; }

    virtual bool start(const QString &program, const QStringList &args) = 0;
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;
    virtual qint64 applicationPID() const = 0;
    virtual int exitCode() const = 0;

//signals:
    virtual void processError(const QString &error) = 0;

#ifdef Q_OS_WIN
    // Add PATH and SystemRoot environment variables in case they are missing
    static QStringList fixWinEnvironment(const QStringList &env);
    // Quote a Windows command line correctly for the "CreateProcess" API
    static QString createWinCommandline(const QString &program, const QStringList &args);
    // Create a bytearray suitable to be passed on as environment
    // to the "CreateProcess" API (0-terminated UTF 16 strings).
    static QByteArray createWinEnvironment(const QStringList &env);
#endif

private:
    QString m_workingDir;
    QStringList m_environment;
};

} //namespace Utils

#endif // ABSTRACTPROCESS_H

