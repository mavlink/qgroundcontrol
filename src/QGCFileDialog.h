/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QGCFILEDIALOG_H
#define QGCFILEDIALOG_H

#ifdef __mobile__
#error Should not be included in mobile builds
#endif

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
        @param[in] strict Makes the default suffix mandatory. Only files with those extensions will be allowed.
        @param[in] options Set the various options that affect the look and feel of the dialog.
        @return The full path and filename to be used to save the file or \c QString("") if none.
        @sa <a href="http://qt-project.org/doc/qt-5/qfiledialog.html#getSaveFileName">QFileDialog::getSaveFileName()</a>
        @remark If a default suffix is given, it will be appended to the filename if the user does not enter one themselves. That is, if the user simply enters \e foo and the default suffix is set to \e bar,
        the returned filename will be \e foo.bar. However, if the user specifies a suffix, the \e strict flag will determine what is done. If the user enters \e foo.txt and \e strict is false, the function
        returns \e foo.txt (no suffix enforced). If \e strict is true however, the default suffix is appended no matter what. In the case above, the function will return \e foo.txt.bar (suffix enforced).
        @remark If \e strict is set and the file name given by the user is renamed (the \e foo.txt.bar example above), the function will check and see if the file already exists. If that's the case, it will
        ask the user if they want to overwrite it.
    */
    static QString getSaveFileName(
        QWidget* parent = 0,
        const QString& caption = QString(),
        const QString& dir = QString(),
        const QString& filter = QString(),
        const QString& defaultSuffix = QString(),
        bool strict = false,
        Options options = 0);

private slots:
    /// @brief The exec slot is private because we only want QGCFileDialog users to use the static methods. Otherwise it will break
    ///        unit testing.
    int exec(void) { return QFileDialog::exec(); }
    
private:
    static void    _validate(Options& options);
    static bool    _validateExtension(const QString& filter, const QString& extension);
    static QString _getFirstExtensionInFilter(const QString& filter);
};


#endif
