/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCQFileDialog.h"
#include "QGCApplication.h"
#include "MainWindow.h"

#ifdef UNITTEST_BUILD
    #include "UnitTest.h"
#endif

#include <QRegularExpression>
#include <QMessageBox>
#include <QPushButton>

QString QGCQFileDialog::getExistingDirectory(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    Options options)
{
    _validate(options);
    
#ifdef UNITTEST_BUILD
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getExistingDirectory(parent, caption, dir, options);
    } else
#endif
    {
        return QFileDialog::getExistingDirectory(parent, caption, dir, options);
    }
}

QString QGCQFileDialog::getOpenFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    Options options)
{
    _validate(options);
    
#ifdef UNITTEST_BUILD
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileName(parent, caption, dir, filter, options);
    } else
#endif
    {
        return QFileDialog::getOpenFileName(parent, caption, dir, filter, NULL, options);
    }
}

QStringList QGCQFileDialog::getOpenFileNames(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    Options options)
{
    _validate(options);
    
#ifdef UNITTEST_BUILD
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getOpenFileNames(parent, caption, dir, filter, options);
    } else
#endif
    {
        return QFileDialog::getOpenFileNames(parent, caption, dir, filter, NULL, options);
    }
}

QString QGCQFileDialog::getSaveFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    const QString& defaultSuffix,
    bool strict,
    Options options)
{
    _validate(options);

#ifdef UNITTEST_BUILD
    if (qgcApp()->runningUnitTests()) {
        return UnitTest::_getSaveFileName(parent, caption, dir, filter, defaultSuffix, options);
    } else
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
                        if (defaultSuffixCopy.isEmpty()) {
                            qWarning() << "Internal error";
                        }
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
        return {};
    }
}

/// @brief Make sure filename is using one of the valid extensions defined in the filter
bool QGCQFileDialog::_validateExtension(const QString& filter, const QString& extension) {
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
QString QGCQFileDialog::_getFirstExtensionInFilter(const QString& filter) {
    QRegularExpression re("(\\*\\.\\w+)");
    QRegularExpressionMatchIterator i = re.globalMatch(filter);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            //-- Return "foo" from "*.foo"
            return match.captured(0).mid(2);
        }
    }
    return {};
}

/// @brief Validates and updates the parameters for the file dialog calls
void QGCQFileDialog::_validate(Options& options)
{
    Q_UNUSED(options)

    // You can't use QGCQFileDialog if QGCApplication is not created yet.
    if (!qgcApp()) {
        qWarning() << "Internal error";
        return;
    }
    
    if (QThread::currentThread() != qgcApp()->thread()) {
        qWarning() << "Threading issue: QGCQFileDialog can only be called from main thread";
        return;
    }
}
