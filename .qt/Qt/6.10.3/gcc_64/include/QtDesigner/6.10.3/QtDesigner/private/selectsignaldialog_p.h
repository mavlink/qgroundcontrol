// Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef SELECTSIGNALDIALOG_H
#define SELECTSIGNALDIALOG_H

#include <QtWidgets/qdialog.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QStandardItemModel;

namespace Ui { class SelectSignalDialog; }

namespace qdesigner_internal {

class SelectSignalDialog : public QDialog
{
    Q_OBJECT
public:
    struct Method
    {
        Method() = default;
        explicit Method(const QString &c, const QString &s, const QStringList &p = QStringList())
            : className(c), signature(s), parameterNames(p) {}
        bool isValid() const { return !signature.isEmpty(); }

        QString className;
        QString signature;
        QStringList parameterNames;
    };

    explicit SelectSignalDialog(QWidget *parent = nullptr);
    ~SelectSignalDialog();

    Method selectedMethod() const;

    void populate(QDesignerFormEditorInterface *core, QObject *object,
                  const QString &defaultSignal);

private slots:
    void currentChanged(const QModelIndex &, const QModelIndex &);
    void activated(const QModelIndex &);

private:
    Method methodFromIndex(const QModelIndex &) const;
    void populateModel(QDesignerFormEditorInterface *core, QObject *object);

    QT_PREPEND_NAMESPACE(Ui)::SelectSignalDialog *m_ui;
    QPushButton *m_okButton;
    QStandardItemModel *m_model;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

Q_DECLARE_METATYPE(qdesigner_internal::SelectSignalDialog::Method)

#endif // SELECTSIGNALDIALOG_H
