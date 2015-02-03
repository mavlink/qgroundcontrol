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
#include "UnitTest.h"
#endif

QString QGCFileDialog::getExistingDirectory(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    Options options)
{
    _validate(options);
    
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
    Options options)
{
    _validate(options);
    
#ifdef QT_DEBUG
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileName(parent, caption, dir, filter, options);
    } else
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
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileNames(parent, caption, dir, filter, options);
    } else
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
    Options options)
{
    _validate(options);

#ifdef QT_DEBUG
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getSaveFileName(parent, caption, dir, filter, defaultSuffix, options);
    } else
#endif
    {
        QFileDialog dlg(parent, caption, dir, filter);
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        if (options) {
            dlg.setOptions(options);
        }
        if (!defaultSuffix.isEmpty()) {
            QString suffixCopy(defaultSuffix);
            //-- Make sure dot is not present
            if (suffixCopy.startsWith(".")) {
                suffixCopy.remove(0,1);
            }
            dlg.setDefaultSuffix(suffixCopy);
        }
        if (dlg.exec()) {
            if (dlg.selectedFiles().count()) {
                return dlg.selectedFiles().first();
            }
        }
        return QString("");
    }
}

/// @brief Validates and updates the parameters for the file dialog calls
void QGCFileDialog::_validate(Options& options)
{
    // You can't use QGCFileDialog if QGCApplication is not created yet.
    Q_ASSERT(qgcApp());
    
    Q_ASSERT_X(QThread::currentThread() == qgcApp()->thread(), "Threading issue", "QGCFileDialog can only be called from main thread");
    
    // On OSX native dialog can hang so we always use Qt dialogs
    options |= DontUseNativeDialog;
    
    if (MainWindow::instance()) {
        MainWindow::instance()->hideSplashScreen();
    }
}
