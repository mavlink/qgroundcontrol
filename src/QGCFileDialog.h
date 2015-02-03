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
///     @brief Subclass of <a href="http://qt-project.org/doc/qt-5/qfiledialog.html">QFileDialog</a>
///     @author Don Gagne <don@thegagnes.com>

/*!
    Subclass of <a href="http://qt-project.org/doc/qt-5/qfiledialog.html">QFileDialog</a> which re-implements the static public functions. The reason for this
    is that the <a href="http://qt-project.org/doc/qt-5/qfiledialog.html">QFileDialog</a> implementations of these use the native os dialogs. On OSX these
    these can intermittently hang. So instead here we use the native dialogs. It also allows
    use to catch these dialogs for unit testing.
    @remark If you need to know what type of file was returned by these functions, you can use something like:
    @code{.cpp}
    QString filename = QGCFileDialog::getSaveFileName(this, tr("Save File"), "~/", "Foo files (*.foo);;All Files (*.*)", "foo");
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        QString fileExtension(fi.suffix());
        if (fileExtension == QString("foo")) {
            // do something
        }
    }
    @endcode
*/

class QGCFileDialog : public QFileDialog {
    
public:

    //! Static helper that will return an existing directory selected by the user.
    /*!
      @param[in] parent The parent QWidget.
      @param[in] caption The caption displayed at the top of the dialog.
      @param[in] dir The initial directory shown to the user.
      @param[in] options Set the various options that affect the look and feel of the dialog.
      @return The chosen path or \c QString("") if none.
      @sa <a href="http://qt-project.org/doc/qt-5/qfiledialog.html#getExistingDirectory">QFileDialog::getExistingDirectory()</a>
    */
    static QString getExistingDirectory(
        QWidget* parent = 0,
        const QString& caption = QString(),
        const QString& dir = QString(),
        Options options = ShowDirsOnly);
    
    //! Static helper that invokes a File Open dialog where the user can select a file to be opened.
    /*!
      @param[in] parent The parent QWidget.
      @param[in] caption The caption displayed at the top of the dialog.
      @param[in] dir The initial directory shown to the user.
      @param[in] filter The filter used for selecting the file type.
      @param[in] options Set the various options that affect the look and feel of the dialog.
      @return The full path and filename to be opened or \c QString("") if none.
      @sa <a href="http://qt-project.org/doc/qt-5/qfiledialog.html#getOpenFileName">QFileDialog::getOpenFileName()</a>
    */
    static QString getOpenFileName(
        QWidget* parent = 0,
        const QString& caption = QString(),
        const QString& dir = QString(),
        const QString& filter = QString(),
        Options options = 0);
    
    //! Static helper that invokes a File Open dialog where the user can select one or more files to be opened.
    /*!
      @param[in] parent The parent QWidget.
      @param[in] caption The caption displayed at the top of the dialog.
      @param[in] dir The initial directory shown to the user.
      @param[in] filter The filter used for selecting the file type.
      @param[in] options Set the various options that affect the look and feel of the dialog.
      @return A <a href="http://qt-project.org/doc/qt-5/qstringlist.html">QStringList</a> object containing zero or more files to be opened.
      @sa <a href="http://qt-project.org/doc/qt-5/qfiledialog.html#getOpenFileNames">QFileDialog::getOpenFileNames()</a>
    */
    static QStringList getOpenFileNames(
        QWidget* parent = 0,
        const QString& caption = QString(),
        const QString& dir = QString(),
        const QString& filter = QString(),
        Options options = 0);
    
    //! Static helper that invokes a File Save dialog where the user can select a directory and enter a filename to be saved.
    /*!
      @param[in] parent The parent QWidget.
      @param[in] caption The caption displayed at the top of the dialog.
      @param[in] dir The initial directory shown to the user.
      @param[in] filter The filter used for selecting the file type.
      @param[in] defaultSuffix Specifies a string that will be added to the filename if it has no suffix already. The suffix is typically used to indicate the file type (e.g. "txt" indicates a text file).
      @param[in] options Set the various options that affect the look and feel of the dialog.
      @return The full path and filename to be used to save the file or \c QString("") if none.
      @sa <a href="http://qt-project.org/doc/qt-5/qfiledialog.html#getSaveFileName">QFileDialog::getSaveFileName()</a>
      @remark If a default suffix is given, it will be appended to the filename if the user does not enter one themselves. That is, if the user simply enters \e foo and the default suffix is set to \e bar,
      the returned filename will be \e foo.bar. However, if the user specifies a suffix, none will be added. That is, if the user enters \e foo.txt, that's what you will receive in return.
    */
    static QString getSaveFileName(
        QWidget* parent = 0,
        const QString& caption = QString(),
        const QString& dir = QString(),
        const QString& filter = QString(),
        const QString& defaultSuffix = QString(),
        Options options = 0);

private slots:
    /// @brief The exec slot is private because we only want QGCFileDialog users to use the static methods. Otherwise it will break
    ///        unit testing.
    int exec(void) { return QGCFileDialog::exec(); }
    
private:
    static void _validate(Options& options);
};


#endif
