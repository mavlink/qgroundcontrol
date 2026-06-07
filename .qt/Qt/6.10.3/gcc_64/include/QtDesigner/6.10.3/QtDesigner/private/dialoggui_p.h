// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DIALOGGUI
#define DIALOGGUI

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

#include "shared_global_p.h"
#include <abstractdialoggui_p.h>

QT_BEGIN_NAMESPACE

class QFileIconProvider;

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT DialogGui : public QDesignerDialogGuiInterface
{
public:
    DialogGui();
    ~DialogGui() override;

    QMessageBox::StandardButton
        message(QWidget *parent, Message context, QMessageBox::Icon icon,
                const QString &title, const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) override;

    QMessageBox::StandardButton
        message(QWidget *parent, Message context, QMessageBox::Icon icon,
                const QString &title, const QString &text, const QString &informativeText,
                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) override;

    QMessageBox::StandardButton
        message(QWidget *parent, Message context, QMessageBox::Icon icon,
                const QString &title, const QString &text, const QString &informativeText, const QString &detailedText,
                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) override;

    QString getExistingDirectory(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), QFileDialog::Options options = QFileDialog::ShowDirsOnly) override;
    QString getOpenFileName(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {}) override;
    QStringList getOpenFileNames(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {}) override;
    QString getSaveFileName(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {}) override;

    QString getOpenImageFileName(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {}) override;
    QStringList getOpenImageFileNames(QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = {}) override;

private:
    QFileIconProvider *ensureIconProvider();
    void initializeImageFileDialog(QFileDialog &fd, QFileDialog::Options options, QFileDialog::FileMode);

    QFileIconProvider *m_iconProvider = nullptr;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // DIALOGGUI
