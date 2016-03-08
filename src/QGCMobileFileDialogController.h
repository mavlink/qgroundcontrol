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

#ifndef QGCMobileFileDialogController_H
#define QGCMobileFileDialogController_H

#include <QObject>

class QGCMobileFileDialogController : public QObject
{
    Q_OBJECT

public:
    /// Return all file in Documents location which match the specified extension
    Q_INVOKABLE QStringList getFiles(const QString& fileExtension);

    /// Return the full path for specified file in the Documents location
    ///     @param filename File name, not fully qualified, may not have extension
    ///     @param fileExtension Expected file extension, added if needed
    Q_INVOKABLE QString fullPath(const QString& filename, const QString& fileExtension);

    /// Check for file existance
    ///     @param filename File name, not fully qualified, may not have extension
    ///     @param fileExtension Expected file extension, added if needed
    /// @return true: File exists at Documents location
    Q_INVOKABLE bool fileExists(const QString& filename, const QString& fileExtension);
};

#endif
