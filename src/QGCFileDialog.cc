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

#include "QGCFileDialog.h"
#include "QGCApplication.h"
#include "QRegularExpression.h"
#include "MainWindow.h"
#ifdef QT_DEBUG
#include "UnitTest.h"
#endif

QString QGCFileDialog::getExistingDirectory(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    Options options)
{
    _validate(NULL, options);
    
#ifdef QT_DEBUG
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getExistingDirectory(parent, caption, dir, options);
    } else
#endif
    {
        return QFileDialog::getExistingDirectory(parent, caption, dir, options);
    }
}

QString QGCFileDialog::getOpenFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    QString* selectedFilter,
    Options options)
{
    _validate(selectedFilter, options);
    
#ifdef QT_DEBUG
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
    } else
#endif
    {
        return QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
    }
}

QStringList QGCFileDialog::getOpenFileNames(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    QString* selectedFilter,
    Options options)
{
    _validate(selectedFilter, options);
    
#ifdef QT_DEBUG
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileNames(parent, caption, dir, filter, selectedFilter, options);
    } else
#endif
    {
        return QFileDialog::getOpenFileNames(parent, caption, dir, filter, selectedFilter, options);
    }
}

QString QGCFileDialog::getSaveFileName(QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    QString* selectedFilter,
    Options options,
    QString* defaultSuffix,
    bool strict)
{
    _validate(selectedFilter, options);

#ifdef QT_DEBUG
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getSaveFileName(parent, caption, dir, filter, selectedFilter, options, defaultSuffix);
    } else
#endif
    {
        QFileDialog dlg(parent, caption, dir, filter);
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        if (options) {
            dlg.setOptions(options);
        }
        if (defaultSuffix) {
            //-- Make sure dot is not present
            if (defaultSuffix->startsWith(".")) {
                defaultSuffix->remove(0,1);
            }
            dlg.setDefaultSuffix(*defaultSuffix);
        }
        while (true) {
            if (dlg.exec()) {
                if (dlg.selectedFiles().count()) {
                    QString result = dlg.selectedFiles().first();
                    //-- If we don't care about the extension, just return it
                    if (!strict) {
                        return result;
                    } else {
                        //-- We must enforce the file extension
                        QFileInfo fi(result);
                        QString userSuffix(fi.suffix());
                        if (_validateExtension(filter, userSuffix)) {
                            return result;
                        }
                        //-- Do we have a default extension?
                        QString localDefaultSuffix;
                        if (!defaultSuffix) {
                            //-- We don't, so get the first one in the filter
                            localDefaultSuffix = _getFirstExtensionInFilter(filter);
                            defaultSuffix = &localDefaultSuffix;
                        }
                        Q_ASSERT(defaultSuffix->isEmpty() == false);
                        //-- Forcefully append our desired extension
                        result += ".";
                        result += *defaultSuffix;
                        //-- Check and see if this new file already exists
                        fi.setFile(result);
                        if (fi.exists()) {
                            //-- Ask user what to do
                            QMessageBox msgBox(
                                QMessageBox::Warning,
                                tr("File Exists"),
                                tr("%1 already exists.\nDo you want to replace it?").arg(fi.fileName()),
                                QMessageBox::Cancel,
                                parent);
                            msgBox.addButton(QMessageBox::Retry);
                            QPushButton *overwriteButton = msgBox.addButton(tr("Replace"), QMessageBox::ActionRole);
                            msgBox.setDefaultButton(QMessageBox::Retry);
                            msgBox.setWindowModality(Qt::ApplicationModal);
                            if (msgBox.exec() == QMessageBox::Retry) {
                                continue;
                            } else if (msgBox.clickedButton() == overwriteButton) {
                                return result;
                            }
                        } else {
                            return result;
                        }
                    }
                }
            }
            break;
        }
        return QString("");
    }
}

/// @brief Make sure filename is using one of the valid extensions defined in the filter
bool QGCFileDialog::_validateExtension(const QString& filter, const QString& extension) {
    QRegularExpression re("(\\*\\.\\w+)");
    QRegularExpressionMatchIterator i = re.globalMatch(filter);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            //-- Compare "foo" with "*.foo"
            if(extension == match.captured(0).mid(2)) {
                return true;
            }
        }
    }
    return false;
}

/// @brief Returns first extension found in filter
QString QGCFileDialog::_getFirstExtensionInFilter(const QString& filter) {
    QRegularExpression re("(\\*\\.\\w+)");
    QRegularExpressionMatchIterator i = re.globalMatch(filter);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
             return match.captured(0).mid(1);
        }
    }
    return QString("");
}

/// @brief Validates and updates the parameters for the file dialog calls
void QGCFileDialog::_validate(QString* selectedFilter, Options& options)
{
    // You can't use QGCFileDialog if QGCApplication is not created yet.
    Q_ASSERT(qgcApp());
    
    Q_ASSERT_X(QThread::currentThread() == qgcApp()->thread(), "Threading issue", "QGCFileDialog can only be called from main thread");
    
    // Support for selectedFilter is not yet implemented through the unit test framework
    Q_UNUSED(selectedFilter);
    Q_ASSERT(selectedFilter == NULL);
    
    // On OSX native dialog can hang so we always use Qt dialogs
    options |= DontUseNativeDialog;
    
    if (MainWindow::instance()) {
        MainWindow::instance()->hideSplashScreen();
    }
}
