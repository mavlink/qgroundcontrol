// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef ABSTRACTDIALOGGUI_H
#define ABSTRACTDIALOGGUI_H

#include <QtDesigner/sdk_global.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qfiledialog.h>

QT_BEGIN_NAMESPACE

class QWidget;

class QDESIGNER_SDK_EXPORT QDesignerDialogGuiInterface
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerDialogGuiInterface)

    QDesignerDialogGuiInterface();
    virtual ~QDesignerDialogGuiInterface();

    enum Message { FormLoadFailureMessage, UiVersionMismatchMessage, ResourceLoadFailureMessage,
                   TopLevelSpacerMessage, PropertyEditorMessage, SignalSlotEditorMessage, FormEditorMessage,
                   PreviewFailureMessage, PromotionErrorMessage, ResourceEditorMessage,
                   ScriptDialogMessage, SignalSlotDialogMessage, OtherMessage, FileChangedMessage };

    virtual QMessageBox::StandardButton
        message(QWidget *parent, Message context, QMessageBox::Icon icon,
                const QString &title, const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) = 0;

    virtual QMessageBox::StandardButton
        message(QWidget *parent, Message context, QMessageBox::Icon icon,
                const QString &title, const QString &text, const QString &informativeText,
                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) = 0;

    virtual QMessageBox::StandardButton
        message(QWidget *parent, Message context, QMessageBox::Icon icon,
                const QString &title, const QString &text, const QString &informativeText, const QString &detailedText,
                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) = 0;

    virtual QString getExistingDirectory(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), QFileDialog::Options options = QFileDialog::ShowDirsOnly)= 0;
    virtual QString getOpenFileName(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {})= 0;
    virtual QString getOpenImageFileName(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {});
    virtual QStringList getOpenFileNames(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {})= 0;
    virtual QStringList getOpenImageFileNames(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {});
    virtual QString getSaveFileName(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {})= 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTDIALOGGUI_H
