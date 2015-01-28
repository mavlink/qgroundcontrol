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

#ifndef QGCFILEDIALOG_H
#define QGCFILEDIALOG_H

#include <QFileDialog>

/// @file
///     @brief Subclass of QFileDialog which re-implements the static public functions. The reason for this
///             is that the QFileDialog implementations of these use the native os dialogs. On OSX these
///             these can intermittently hang. So instead here we use the native dialogs. It also allows
///             use to catch these dialogs for unit testing.
///     @author Don Gagne <don@thegagnes.com>

class QGCFileDialog : public QFileDialog {
    
public:
    static QString getExistingDirectory(QWidget* parent = 0,
                                        const QString& caption = QString(),
                                        const QString& dir = QString(),
                                        Options options = ShowDirsOnly);
    
    static QString getOpenFileName(QWidget* parent = 0,
                                   const QString& caption = QString(),
                                   const QString& dir = QString(),
                                   const QString& filter = QString(),
                                   QString* selectedFilter = 0,
                                   Options options = 0);
    
    static QStringList getOpenFileNames(QWidget* parent = 0,
                                        const QString& caption = QString(),
                                        const QString& dir = QString(),
                                        const QString& filter = QString(),
                                        QString* selectedFilter = 0,
                                        Options options = 0);
    
    /// @brief getSaveFileName with an extra (optional) argument to define the extension (suffix) to
    ///        be added if none is given by the user. If set, don't add the preceding period (i.e.
    ///        use "mavlink" instead of ".mavlink")
    static QString getSaveFileName(QWidget* parent = 0,
                                   const QString& caption = QString(),
                                   const QString& dir = QString(),
                                   const QString& filter = QString(),
                                   QString* selectedFilter = 0,
                                   Options options = 0,
                                   QString* defaultSuffix = 0);
    
private slots:
    /// @brief The exec slot is private becasue when only want QGCFileDialog users to use the static methods. Otherwise it will break
    ///         unit testing.
    int exec(void) { return QGCFileDialog::exec(); }
    
private:
    static void _validate(QString* selectedFilter, Options& options);
};


#endif
