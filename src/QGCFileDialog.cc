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
#include "MainWindow.h"

#ifdef QT_DEBUG
#ifndef __mobile__
#include "UnitTest.h"
#endif
#endif

#include <QRegularExpression>
#include <QMessageBox>
#include <QPushButton>

QString QGCFileDialog::getExistingDirectory(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    Options options)
{
    _validate(options);
    
#ifdef QT_DEBUG
#ifndef __mobile__
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getExistingDirectory(parent, caption, dir, options);
    } else
#endif
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
    Options options)
{
    _validate(options);
    
#ifdef QT_DEBUG
#ifndef __mobile__
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileName(parent, caption, dir, filter, options);
    } else
#endif
#endif
    {
        return QFileDialog::getOpenFileName(parent, caption, dir, filter, NULL, options);
    }
}

QStringList QGCFileDialog::getOpenFileNames(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    Options options)
{
    _validate(options);
    
#ifdef QT_DEBUG
#ifndef __mobile__
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileNames(parent, caption, dir, filter, options);
    } else
#endif
#endif
    {
        return QFileDialog::getOpenFileNames(parent, caption, dir, filter, NULL, options);
    }
}

QString QGCFileDialog::getSaveFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    const QString& defaultSuffix,
    bool strict,
    Options options)
{
    _validate(options);

#ifdef QT_DEBUG
#ifndef __mobile__
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getSaveFileName(parent, caption, dir, filter, defaultSuffix, options);
    } else
#endif
#endif
    {
        QString defaultSuffixCopy(defaultSuffix);
        QFileDialog dlg(parent, caption, dir, filter);
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        if (options) {
            dlg.setOptions(options);
        }
        if (!defaultSuffixCopy.isEmpty()) {
            //-- Make sure dot is not present
            if (defaultSuffixCopy.startsWith(".")) {
                defaultSuffixCopy.remove(0,1);
            }
            dlg.setDefaultSuffix(defaultSuffixCopy);
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
                        if (defaultSuffixCopy.isEmpty()) {
                            //-- We don't, so get the first one in the filter
                            defaultSuffixCopy = _getFirstExtensionInFilter(filter);
                        }
                        //-- If this is set to strict, we have to have a default extension
                        Q_ASSERT(defaultSuffixCopy.isEmpty() == false);
                        //-- Forcefully append our desired extension
                        result += ".";
                        result += defaultSuffixCopy;
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
            //-- Return "foo" from "*.foo"
            return match.captured(0).mid(2);
        }
    }
    return QString("");
}

/// @brief Validates and updates the parameters for the file dialog calls
void QGCFileDialog::_validate(Options& options)
{
    // You can't use QGCFileDialog if QGCApplication is not created yet.
    Q_ASSERT(qgcApp());
    
    Q_ASSERT_X(QThread::currentThread() == qgcApp()->thread(), "Threading issue", "QGCFileDialog can only be called from main thread");
#ifdef __mobile__
    Q_UNUSED(options)
#else
    // On OSX native dialog can hang so we always use Qt dialogs
    options |= DontUseNativeDialog;
#endif
    if (MainWindow::instance()) {
    }
}
