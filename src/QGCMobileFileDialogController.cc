/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "QGCMobileFileDialogController.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>

QStringList QGCMobileFileDialogController::getFiles(const QString& fileExtension)
{
    QStringList files;

    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qWarning() << "No Documents location";
        return QStringList();
    }
    QDir fileDir = docDirs.at(0);

    QFileInfoList fileInfoList = fileDir.entryInfoList(QStringList(QString("*.%1").arg(fileExtension)),  QDir::Files, QDir::Name);

    foreach (const QFileInfo& fileInfo, fileInfoList) {
        files << fileInfo.baseName() + QStringLiteral(".") + fileExtension;
    }

    return files;
}

QString QGCMobileFileDialogController::fullPath(const QString& filename, const QString& fileExtension)
{
    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qWarning() << "No Documents location";
        return filename;
    }

    QString fixedFilename(filename);
    QString correctExtension = QString(".%1").arg(fileExtension);
    if (!filename.endsWith(correctExtension)) {
        fixedFilename += correctExtension;
    }

    QString fullPath = docDirs.at(0) + QDir::separator() + fixedFilename;
    qDebug() << fullPath;
    return fullPath;
}

bool QGCMobileFileDialogController::fileExists(const QString& filename, const QString& fileExtension)
{
    QFile file(fullPath(filename, fileExtension));
    return file.exists();
}
