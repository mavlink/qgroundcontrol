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
#ifdef QT_DEBUG
#include "UnitTest.h"
#endif

QString QGCFileDialog::getExistingDirectory(QWidget* parent,
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

QString QGCFileDialog::getOpenFileName(QWidget* parent,
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

QStringList QGCFileDialog::getOpenFileNames(QWidget* parent,
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
                                  Options options)
{
    _validate(selectedFilter, options);
    
#ifdef QT_DEBUG
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
    } else
#endif
    {
        return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
    }
}

/// @brief Validates and updates the parameters for the file dialog calls
void QGCFileDialog::_validate(QString* selectedFilter, Options& options)
{
    // You can't use QGCFileDialog if QGCApplication is not created yet.
    Q_ASSERT(qgcApp());
    
    // Support for selectedFilter is not yet implemented through the unit test framework
    Q_ASSERT(selectedFilter == NULL);
    
    // On OSX native dialog can hang so we always use Qt dialogs
    options | DontUseNativeDialog;
}
