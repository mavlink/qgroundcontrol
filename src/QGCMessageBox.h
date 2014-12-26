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
#include "QGCApplication.h"
#ifdef QT_DEBUG
#include "UnitTest.h"
#endif

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
    
private slots:
    /// @brief The exec slot is private becasue when only want QGCMessageBox users to use the static methods. Otherwise it will break
    ///         unit testing.
    int exec(void) { return QMessageBox::exec(); }
    
private:
    static QWidget* _validateParameters(StandardButtons buttons, StandardButton* defaultButton, QWidget* parent)
    {
        // This is an obsolete bit which unit tests use for signalling. It should not be used in regular code.
        Q_ASSERT(!(buttons & QMessageBox::Escape));
        
        // If there is more than one button displayed, make sure a default button is set. Without this unit test code
        // will not be able to respond to unexpected message boxes.
        
        unsigned int bits = static_cast<unsigned int>(buttons);
        int buttonCount = 0;
        for (size_t i=0; i<sizeof(bits)*8; i++) {
            if (bits & (1 << i)) {
                buttonCount++;
            }
        }
        Q_ASSERT(buttonCount != 0);
        
        if (buttonCount > 1) {
            Q_ASSERT(buttons & *defaultButton);
        } else {
            // Force default button to be set correctly for single button case to make unit test code simpler
            *defaultButton = static_cast<QMessageBox::StandardButton>(static_cast<int>(buttons));
        }
        
        return (parent == NULL) ? MainWindow::instance() : parent;
    }

    static StandardButton _messageBox(Icon icon, const QString& title, const QString& text, StandardButtons buttons, StandardButton defaultButton, QWidget* parent)
    {
        // You can't use QGCMessageBox if QGCApplication is not created yet.
        Q_ASSERT(qgcApp());
        
        Q_ASSERT_X(QThread::currentThread() == qgcApp()->thread(), "Threading issue", "QGCMessageBox can only be called from main thread");
        
        parent = _validateParameters(buttons, &defaultButton, parent);

        if (MainWindow::instance()) {
            MainWindow::instance()->hideSplashScreen();
            if (parent == NULL) {
                parent = MainWindow::instance();
            }
        }
        
#ifdef QT_DEBUG
        if (qgcApp()->runningUnitTests()) {
            qDebug() << "QGCMessageBox (unit testing)" << title << text;
            return UnitTest::_messageBox(icon, title, text, buttons, defaultButton);
        } else
#endif
        {
#ifdef Q_OS_MAC
            QString emptyTitle;
            QMessageBox box(icon, emptyTitle, title, buttons, parent);
            box.setDefaultButton(defaultButton);
            box.setInformativeText(text);
#else
            QMessageBox box(icon, title, text, buttons, parent);
            box.setDefaultButton(defaultButton);
#endif
            return static_cast<QMessageBox::StandardButton>(box.exec());
        }
    }
};

#endif
