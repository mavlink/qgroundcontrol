/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
