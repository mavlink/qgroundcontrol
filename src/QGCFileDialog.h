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
///     @brief Subclass of <a href="http://qt-project.org/doc/qt-4.8/qfiledialog.html">QFileDialog</a>
///     @author Don Gagne <don@thegagnes.com>

/*!
     Subclass of <a href="http://qt-project.org/doc/qt-4.8/qfiledialog.html">QFileDialog</a> which re-implements the static public functions. The reason for this
     is that the <a href="http://qt-project.org/doc/qt-4.8/qfiledialog.html">QFileDialog</a> implementations of these use the native os dialogs. On OSX these
     these can intermittently hang. So instead here we use the native dialogs. It also allows
     use to catch these dialogs for unit testing.
*/

class QGCFileDialog : public QFileDialog {
    
public:

    //! Static helper that will return an existing directory selected by the user.
    /*!
      \param parent The parent QWidget.
      \param caption The caption displayed at the top of the dialog.
      \param dir The initial directory shown to the user.
      \param options Set the various options that affect the look and feel of the dialog.
      \return The chosen path or QString("") if none.
      \sa <a href="http://qt-project.org/doc/qt-4.8/qfiledialog.html#getExistingDirectory">QFileDialog::getExistingDirectory()</a>
    */
    static QString getExistingDirectory(QWidget* parent = 0,
                                        const QString& caption = QString(),
                                        const QString& dir = QString(),
                                        Options options = ShowDirsOnly);
    
    //! Static helper that invokes a File Open dialog where the user can select a file to be opened.
    /*!
      \param parent The parent QWidget.
      \param caption The caption displayed at the top of the dialog.
      \param dir The initial directory shown to the user.
      \param filter The filter used for selecting the file type.
      \param selectedFilter **NOT IMPLEMENTED** Returns the filter that the user selected in the file dialog.
      \param options Set the various options that affect the look and feel of the dialog.
      \return The full path and filename to be opened or QString("") if none.
      \sa <a href="http://qt-project.org/doc/qt-4.8/qfiledialog.html#getOpenFileName">QFileDialog::getOpenFileName()</a>
    */
    static QString getOpenFileName(QWidget* parent = 0,
                                   const QString& caption = QString(),
                                   const QString& dir = QString(),
                                   const QString& filter = QString(),
                                   QString* selectedFilter = 0,
                                   Options options = 0);
    
    //! Static helper that invokes a File Open dialog where the user can select one or more files to be opened.
    /*!
      \param parent The parent QWidget.
      \param caption The caption displayed at the top of the dialog.
      \param dir The initial directory shown to the user.
      \param filter The filter used for selecting the file type.
      \param selectedFilter **NOT IMPLEMENTED** Returns the filter that the user selected in the file dialog.
      \param options Set the various options that affect the look and feel of the dialog.
      \return A QStringList object containing zero or more files to be opened.
      \sa <a href="http://qt-project.org/doc/qt-4.8/qfiledialog.html#getOpenFileNames">QFileDialog::getOpenFileNames()</a>
    */
    static QStringList getOpenFileNames(QWidget* parent = 0,
                                        const QString& caption = QString(),
                                        const QString& dir = QString(),
                                        const QString& filter = QString(),
                                        QString* selectedFilter = 0,
                                        Options options = 0);
    
    //! Static helper that invokes a File Save dialog where the user can select a directory and enter a filename to be saved.
    /*!
      \param parent The parent QWidget.
      \param caption The caption displayed at the top of the dialog.
      \param dir The initial directory shown to the user.
      \param filter The filter used for selecting the file type.
      \param selectedFilter **NOT IMPLEMENTED** Returns the filter that the user selected in the file dialog.
      \param options Set the various options that affect the look and feel of the dialog.
      \param defaultSuffix Specifies a string that will be added to the filename if it has no suffix already. The suffix is typically used to indicate the file type (e.g. "txt" indicates a text file).
      \return The full path and filename to be used to save the file or QString("") if none.
      \sa <a href="http://qt-project.org/doc/qt-4.8/qfiledialog.html#getSaveFileName">QFileDialog::getSaveFileName()</a>
    */
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
