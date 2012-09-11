/**
 ******************************************************************************
 *
 * @file       abstractprocess_win.cpp
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

#include "abstractprocess.h"

#include <windows.h>

namespace Utils {  

QStringList AbstractProcess::fixWinEnvironment(const QStringList &env)
{
    QStringList envStrings = env;
    // add PATH if necessary (for DLL loading)
    if (envStrings.filter(QRegExp(QLatin1String("^PATH="),Qt::CaseInsensitive)).isEmpty()) {
        QByteArray path = qgetenv("PATH");
        if (!path.isEmpty())
            envStrings.prepend(QString(QLatin1String("PATH=%1")).arg(QString::fromLocal8Bit(path)));
    }
    // add systemroot if needed
    if (envStrings.filter(QRegExp(QLatin1String("^SystemRoot="),Qt::CaseInsensitive)).isEmpty()) {
        QByteArray systemRoot = qgetenv("SystemRoot");
        if (!systemRoot.isEmpty())
            envStrings.prepend(QString(QLatin1String("SystemRoot=%1")).arg(QString::fromLocal8Bit(systemRoot)));
    }
    return envStrings;
}

QString AbstractProcess::createWinCommandline(const QString &program, const QStringList &args)
{
    const QChar doubleQuote = QLatin1Char('"');
    const QChar blank = QLatin1Char(' ');
    const QChar backSlash = QLatin1Char('\\');

    QString programName = program;
    if (!programName.startsWith(doubleQuote) && !programName.endsWith(doubleQuote) && programName.contains(blank)) {
        programName.insert(0, doubleQuote);
        programName.append(doubleQuote);
    }
    // add the prgram as the first arrg ... it works better
    programName.replace(QLatin1Char('/'), backSlash);
    QString cmdLine = programName;
    if (args.empty())
        return cmdLine;

    cmdLine += blank;
    for (int i = 0; i < args.size(); ++i) {
        QString tmp = args.at(i);
        // in the case of \" already being in the string the \ must also be escaped
        tmp.replace(QLatin1String("\\\""), QLatin1String("\\\\\""));
        // escape a single " because the arguments will be parsed
        tmp.replace(QString(doubleQuote), QLatin1String("\\\""));
        if (tmp.isEmpty() || tmp.contains(blank) || tmp.contains('\t')) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            QString endQuote(doubleQuote);
            int i = tmp.length();
            while (i > 0 && tmp.at(i - 1) == backSlash) {
                --i;
                endQuote += backSlash;
            }
            cmdLine += QLatin1String(" \"");
            cmdLine += tmp.left(i);
            cmdLine += endQuote;
        } else {
            cmdLine += blank;
            cmdLine += tmp;
        }
    }
    return cmdLine;
}

QByteArray AbstractProcess::createWinEnvironment(const QStringList &env)
{
    QByteArray envlist;
    int pos = 0;
    foreach (const QString &tmp, env) {
        const uint tmpSize = sizeof(TCHAR) * (tmp.length() + 1);
        envlist.resize(envlist.size() + tmpSize);
        memcpy(envlist.data() + pos, tmp.utf16(), tmpSize);
        pos += tmpSize;
    }
    envlist.resize(envlist.size() + 2);
    envlist[pos++] = 0;
    envlist[pos++] = 0;
    return envlist;
}

} //namespace Utils
