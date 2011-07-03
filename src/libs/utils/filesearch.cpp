/**
 ******************************************************************************
 *
 * @file       filesearch.cpp
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

#include "filesearch.h"
#include <cctype>

#include <QtCore/QIODevice>
#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QFutureInterface>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QRegExp>
#include <QtCore/QCoreApplication>

#include <qtconcurrent/runextensions.h>

using namespace Utils;

static inline QString msgCanceled(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: canceled. %n occurrences found in %2 files.",
                                       0, QCoreApplication::CodecForTr, numMatches).
                                       arg(searchTerm).arg(numFilesSearched);
}

static inline QString msgFound(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: %n occurrences found in %2 files.",
                                       0, QCoreApplication::CodecForTr, numMatches).
                                       arg(searchTerm).arg(numFilesSearched);
}

static inline QString msgFound(const QString &searchTerm, int numMatches, int numFilesSearched, int filesSize)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: %n occurrences found in %2 of %3 files.",
                                       0, QCoreApplication::CodecForTr, numMatches).
                                       arg(searchTerm).arg(numFilesSearched).arg(filesSize);
}

namespace {

void runFileSearch(QFutureInterface<FileSearchResult> &future,
                   QString searchTerm,
                   QStringList files,
                   QTextDocument::FindFlags flags,
                   QMap<QString, QString> fileToContentsMap)
{
    future.setProgressRange(0, files.size());
    int numFilesSearched = 0;
    int numMatches = 0;

    bool caseInsensitive = !(flags & QTextDocument::FindCaseSensitively);
    bool wholeWord = (flags & QTextDocument::FindWholeWords);

    QByteArray sa = searchTerm.toUtf8();
    int scMaxIndex = sa.length()-1;
    const char *sc = sa.constData();

    QByteArray sal = searchTerm.toLower().toUtf8();
    const char *scl = sal.constData();

    QByteArray sau = searchTerm.toUpper().toUtf8();
    const char *scu = sau.constData();

    int chunkSize = qMax(100000, sa.length());

    QFile file;
    QBuffer buffer;
    foreach (QString s, files) {
        if (future.isPaused())
            future.waitForResume();
        if (future.isCanceled()) {
            future.setProgressValueAndText(numFilesSearched, msgCanceled(searchTerm, numMatches, numFilesSearched));
            break;
        }
        QIODevice *device;
        if (fileToContentsMap.contains(s)) {
            buffer.setData(fileToContentsMap.value(s).toLocal8Bit());
            device = &buffer;
        } else {
            file.setFileName(s);
            device = &file;
        }
        if (!device->open(QIODevice::ReadOnly))
            continue;
        int lineNr = 1;
        const char *startOfLastLine = NULL;

        bool firstChunk = true;
        while (!device->atEnd()) {
            if (!firstChunk)
                device->seek(device->pos()-sa.length()+1);

            const QByteArray chunk = device->read(chunkSize);
            const char *chunkPtr = chunk.constData();
            startOfLastLine = chunkPtr;
            for (const char *regionPtr = chunkPtr; regionPtr < chunkPtr + chunk.length()-scMaxIndex; ++regionPtr) {
                const char *regionEnd = regionPtr + scMaxIndex;

                if (*regionPtr == '\n') {
                    startOfLastLine = regionPtr + 1;
                    ++lineNr;
                }
                else if (
                        // case sensitive
                        (!caseInsensitive && *regionPtr == sc[0] && *regionEnd == sc[scMaxIndex])
                        ||
                        // case insensitive
                        (caseInsensitive && (*regionPtr == scl[0] || *regionPtr == scu[0])
                        && (*regionEnd == scl[scMaxIndex] || *regionEnd == scu[scMaxIndex]))
                         ) {
                    const char *afterRegion = regionEnd + 1;
                    const char *beforeRegion = regionPtr - 1;
                    bool equal = true;
                    if (wholeWord &&
                            (  isalnum(*beforeRegion)
                            || (*beforeRegion == '_')
                            || isalnum(*afterRegion)
                            || (*afterRegion == '_'))) {
                        equal = false;
                    }

                    int regionIndex = 1;
                    for (const char *regionCursor = regionPtr + 1; regionCursor < regionEnd; ++regionCursor, ++regionIndex) {
                        if (  // case sensitive
                              (!caseInsensitive && equal && *regionCursor != sc[regionIndex])
                              ||
                              // case insensitive
                              (caseInsensitive && equal && *regionCursor != sc[regionIndex] && *regionCursor != scl[regionIndex] && *regionCursor != scu[regionIndex])
                               ) {
                         equal = false;
                        }
                    }
                    if (equal) {
                        int textLength = chunk.length() - (startOfLastLine - chunkPtr);
                        if (textLength > 0) {
                            QByteArray res;
                            res.reserve(256);
                            int i = 0;
                            int n = 0;
                            while (startOfLastLine[i] != '\n' && startOfLastLine[i] != '\r' && i < textLength && n++ < 256)
                                res.append(startOfLastLine[i++]);
                            future.reportResult(FileSearchResult(s, lineNr, QString(res),
                                                          regionPtr - startOfLastLine, sa.length()));
                            ++numMatches;
                        }
                    }
                }
            }
            firstChunk = false;
        }
        ++numFilesSearched;
        future.setProgressValueAndText(numFilesSearched, msgFound(searchTerm, numMatches, numFilesSearched, files.size()));
        device->close();
    }
    if (!future.isCanceled())
        future.setProgressValueAndText(numFilesSearched, msgFound(searchTerm, numMatches, numFilesSearched));
}

void runFileSearchRegExp(QFutureInterface<FileSearchResult> &future,
                   QString searchTerm,
                   QStringList files,
                   QTextDocument::FindFlags flags,
                   QMap<QString, QString> fileToContentsMap)
{
    future.setProgressRange(0, files.size());
    int numFilesSearched = 0;
    int numMatches = 0;
    if (flags & QTextDocument::FindWholeWords)
        searchTerm = QString::fromLatin1("\\b%1\\b").arg(searchTerm);
    const Qt::CaseSensitivity caseSensitivity = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const QRegExp expression(searchTerm, caseSensitivity);

    QFile file;
    QString str;
    QTextStream stream;
    foreach (const QString &s, files) {
        if (future.isPaused())
            future.waitForResume();
        if (future.isCanceled()) {
            future.setProgressValueAndText(numFilesSearched, msgCanceled(searchTerm, numMatches, numFilesSearched));
            break;
        }

        bool needsToCloseFile = false;
        if (fileToContentsMap.contains(s)) {
            str = fileToContentsMap.value(s);
            stream.setString(&str);
        } else {
            file.setFileName(s);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            needsToCloseFile = true;
            stream.setDevice(&file);
        }
        int lineNr = 1;
        QString line;
        while (!stream.atEnd()) {
            line = stream.readLine();
            int pos = 0;
            while ((pos = expression.indexIn(line, pos)) != -1) {
                future.reportResult(FileSearchResult(s, lineNr, line,
                                              pos, expression.matchedLength()));
                pos += expression.matchedLength();
            }
            ++lineNr;
        }
        ++numFilesSearched;
        future.setProgressValueAndText(numFilesSearched, msgFound(searchTerm, numMatches, numFilesSearched, files.size()));
        if (needsToCloseFile)
            file.close();
    }
    if (!future.isCanceled())
        future.setProgressValueAndText(numFilesSearched, msgFound(searchTerm, numMatches, numFilesSearched));
}

} // namespace


QFuture<FileSearchResult> Utils::findInFiles(const QString &searchTerm, const QStringList &files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return QtConcurrent::run<FileSearchResult, QString, QStringList, QTextDocument::FindFlags, QMap<QString, QString> >
            (runFileSearch, searchTerm, files, flags, fileToContentsMap);
}

QFuture<FileSearchResult> Utils::findInFilesRegExp(const QString &searchTerm, const QStringList &files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return QtConcurrent::run<FileSearchResult, QString, QStringList, QTextDocument::FindFlags, QMap<QString, QString> >
            (runFileSearchRegExp, searchTerm, files, flags, fileToContentsMap);
}
