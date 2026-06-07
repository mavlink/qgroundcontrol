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

#ifndef QTRESOURCEEDITOR_H
#define QTRESOURCEEDITOR_H

#include <QtCore/qscopedpointer.h>
#include <QtWidgets/qdialog.h>

QT_BEGIN_NAMESPACE

class QtResourceModel;
class QDesignerDialogGuiInterface;
class QDesignerFormEditorInterface;

class QtResourceEditorDialog : public QDialog
{
    Q_OBJECT
public:
    QtResourceModel *model() const;
    void setResourceModel(QtResourceModel *model);

    QString selectedResource() const;

    static QString editResources(QDesignerFormEditorInterface *core, QtResourceModel *model,
                                 QDesignerDialogGuiInterface *dlgGui, QWidget *parent = nullptr);

    // Helper to display a message box with rcc logs in case of errors.
    static void displayResourceFailures(const QString &logOutput, QDesignerDialogGuiInterface *dlgGui, QWidget *parent = nullptr);

public slots:
    void accept() override;

private:
    QtResourceEditorDialog(QDesignerFormEditorInterface *core, QDesignerDialogGuiInterface *dlgGui, QWidget *parent = nullptr);
    ~QtResourceEditorDialog() override;

    QScopedPointer<class QtResourceEditorDialogPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QtResourceEditorDialog)
    Q_DISABLE_COPY_MOVE(QtResourceEditorDialog)
};

QT_END_NAMESPACE

#endif

