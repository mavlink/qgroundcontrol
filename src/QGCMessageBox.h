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

#ifndef QGCMESSAGEBOX_H
#define QGCMESSAGEBOX_H

#include <QMessageBox>

#include "MainWindow.h"

/// @file
///     @brief Subclass of QMessageBox which re-implements the static public functions. There are two reasons for this:
///             1) The QMessageBox implementation on OSX does now show the title string. This leads to message
///             boxes which don't make much sense. So on OSX we set title to text and text to informative text.
///             2) If parent is NULL, we set parent to MainWindow::instance
///     @author Don Gagne <don@thegagnes.com>

class QGCMessageBox : public QMessageBox {
    
public:
    static StandardButton critical(const QString& title, const QString& text, StandardButtons buttons = Ok, StandardButton defaultButton = NoButton, QWidget* parent = NULL)
        { return _messageBox(QMessageBox::Critical, title, text, buttons, defaultButton, parent); }
    
    static StandardButton information(const QString & title, const QString& text, StandardButtons buttons = Ok, StandardButton defaultButton = NoButton, QWidget* parent = NULL)
        { return _messageBox(QMessageBox::Information, title, text, buttons, defaultButton, parent); }
    
    static StandardButton question(const QString& title, const QString& text, StandardButtons buttons = Ok, StandardButton defaultButton = NoButton, QWidget* parent = NULL)
        { return _messageBox(QMessageBox::Question, title, text, buttons, defaultButton, parent); }
    
    static StandardButton warning(const QString& title, const QString& text, StandardButtons buttons = Ok, StandardButton defaultButton = NoButton, QWidget* parent = NULL)
        { return _messageBox(QMessageBox::Warning, title, text, buttons, defaultButton, parent); }
    
private:
#ifdef Q_OS_MAC
    static StandardButton _messageBox(Icon icon, const QString& title, const QString& text, StandardButtons buttons, StandardButton defaultButton, QWidget* parent)
    {
        if (parent == NULL) {
            parent = qgcApp()->singletonMainWindow();
        }
        QString emptyTitle;
        QMessageBox box(icon, emptyTitle, title, buttons, parent);
        box.setDefaultButton(defaultButton);
        box.setInformativeText(text);
        return static_cast<QMessageBox::StandardButton>(box.exec());
    }
#else
    static StandardButton _messageBox(Icon icon, const QString& title, const QString& text, StandardButtons buttons, StandardButton defaultButton, QWidget* parent)
    {
        if (parent == NULL) {
            parent = qgcApp()->singletonMainWindow();
        }
        QMessageBox box(icon, title, text, buttons, parent);
        box.setDefaultButton(defaultButton);
        return static_cast<QMessageBox::StandardButton>(box.exec());
    }
#endif
};

#endif
