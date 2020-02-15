/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
