/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QGCMESSAGEBOX_H
#define QGCMESSAGEBOX_H

#ifdef __mobile__
#error Should not be included in mobile builds
#endif

#include <QMessageBox>

#include "MainWindow.h"
#include "QGCApplication.h"

#ifdef UNITTEST_BUILD
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
    /// @brief The exec slot is private because when only want QGCMessageBox users to use the static methods. Otherwise it will break
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
            if (parent == NULL) {
                parent = MainWindow::instance();
            }
        }

        qDebug() << "QGCMessageBox (unit testing)" << title << text;

#ifdef UNITTEST_BUILD
        if (qgcApp()->runningUnitTests()) {
            return UnitTest::_messageBox(icon, title, text, buttons, defaultButton);
        } else
#endif
        {
#ifdef Q_OS_MAC
            QString emptyTitle;
            QMessageBox box(icon, emptyTitle, title, buttons, parent);
            box.setDefaultButton(defaultButton);
            box.setInformativeText(text);
            // Get this thing off a proper size
            QSpacerItem* horizontalSpacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            QGridLayout* layout = (QGridLayout*)box.layout();
            layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
#else
            QMessageBox box(icon, title, text, buttons, parent);
            box.setDefaultButton(defaultButton);
#endif
            return static_cast<QMessageBox::StandardButton>(box.exec());
        }
    }
};

#endif
