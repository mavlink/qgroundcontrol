/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCTemporaryFile.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QRandomGenerator>
#include <QtCore/QStandardPaths>

QGC_LOGGING_CATEGORY(QGCTemporaryFileLog, "qgc.utilities.qgctemporaryfile");

QGCTemporaryFile::QGCTemporaryFile(const QString &fileTemplate, QObject *parent)
    : QFile(parent)
    , _template(fileTemplate)
{
    // qCDebug(QGCTemporaryFileLog) << Q_FUNC_INFO << this;
}

QGCTemporaryFile::~QGCTemporaryFile()
{
    if (_autoRemove) {
        remove();
    }

    // qCDebug(QGCTemporaryFileLog) << Q_FUNC_INFO << this;
}

bool QGCTemporaryFile::open(QFile::OpenMode openMode)
{
    setFileName(_newTempFileFullyQualifiedName(_template));

    return QFile::open(openMode);
}

QString QGCTemporaryFile::_newTempFileFullyQualifiedName(const QString &fileTemplate)
{
    QString nameTemplate = fileTemplate;
    const QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    static constexpr const char rgDigits[] = "0123456789";

    QString tempFilename;
    do {
        QString uniqueStr;
        for (int i = 0; i < 6; i++) {
            uniqueStr += rgDigits[QRandomGenerator::global()->generate() % 10];
        }

        if (fileTemplate.contains("XXXXXX")) {
            tempFilename = nameTemplate.replace("XXXXXX", uniqueStr, Qt::CaseSensitive);
        } else {
            tempFilename = nameTemplate + uniqueStr;
        }
    } while (tempDir.exists(tempFilename));

    return tempDir.filePath(tempFilename);
}
