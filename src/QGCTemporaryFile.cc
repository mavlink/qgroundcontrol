/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @brief This class mimics QTemporaryFile. We have our own implementation due to the fact that
///				QTemporaryFile implemenation differs cross platform making it unusable for our use-case.
///				Look for bug reports on QTemporaryFile keeping the file locked for details.
///
///     @author Don Gagne <don@thegagnes.com>

#include "QGCTemporaryFile.h"

#include <QDir>
#include <QStandardPaths>

QGCTemporaryFile::QGCTemporaryFile(const QString& fileTemplate, QObject* parent) :
    QFile(parent),
    _template(fileTemplate)
{

}

bool QGCTemporaryFile::open(QFile::OpenMode openMode)
{
    QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    
    // Generate unique, non-existing filename
    
    static const char rgDigits[] = "0123456789";
    
    QString tempFilename;
    
    do {
        QString uniqueStr;
        for (int i=0; i<6; i++) {
            uniqueStr += rgDigits[qrand() % 10];
        }
        
        if (_template.contains("XXXXXX")) {
            tempFilename = _template.replace("XXXXXX", uniqueStr, Qt::CaseSensitive);
        } else {
            tempFilename = _template + uniqueStr;
        }
    } while (tempDir.exists(tempFilename));

    setFileName(tempDir.filePath(tempFilename));
    
    return QFile::open(openMode);
}
