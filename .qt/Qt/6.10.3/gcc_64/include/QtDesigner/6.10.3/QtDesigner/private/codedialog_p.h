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

#ifndef CODEPREVIEWDIALOG_H
#define CODEPREVIEWDIALOG_H

#include "shared_global_p.h"
#include <QtWidgets/qdialog.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;

namespace qdesigner_internal {

enum class UicLanguage;

// Dialog for viewing code.
class QDESIGNER_SHARED_EXPORT CodeDialog : public QDialog
{
    Q_OBJECT
    explicit CodeDialog(QWidget *parent = nullptr);
public:
    ~CodeDialog() override;

    static bool generateCode(const QDesignerFormWindowInterface *fw,
                             UicLanguage language,
                             QString *code,
                             QString *errorMessage);

    static bool showCodeDialog(const QDesignerFormWindowInterface *fw,
                               UicLanguage language,
                               QWidget *parent,
                               QString *errorMessage);

private slots:
    void slotSaveAs();
#if QT_CONFIG(clipboard)
    void copyAll();
#endif

private:
    void setCode(const QString &code);
    QString code() const;
    void setFormFileName(const QString &f);
    QString formFileName() const;
    void setMimeType(const QString &m);

    void warning(const QString &msg);

    struct CodeDialogPrivate;
    CodeDialogPrivate *m_impl;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // CODEPREVIEWDIALOG_H
