// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMESSAGEDIALOG_P_H
#define QQUICKMESSAGEDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickabstractdialog_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2_EXPORT QQuickMessageDialog : public QQuickAbstractDialog
{
    Q_OBJECT

private:
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QString informativeText READ informativeText WRITE setInformativeText NOTIFY informativeTextChanged FINAL)
    Q_PROPERTY(QString detailedText READ detailedText WRITE setDetailedText NOTIFY detailedTextChanged FINAL)
    Q_PROPERTY(QPlatformDialogHelper::StandardButtons buttons READ buttons WRITE setButtons NOTIFY buttonsChanged FINAL)
    QML_EXTENDED_NAMESPACE(QPlatformDialogHelper)
    QML_NAMED_ELEMENT(MessageDialog)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickMessageDialog(QObject *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    QString informativeText() const;
    void setInformativeText(const QString &text);

    QString detailedText() const;
    void setDetailedText(const QString &text);

    QPlatformDialogHelper::StandardButtons buttons() const;
    void setButtons(QPlatformDialogHelper::StandardButtons buttons);

Q_SIGNALS:
    void textChanged();
    void informativeTextChanged();
    void detailedTextChanged();
    void buttonsChanged();

    void buttonClicked(QPlatformDialogHelper::StandardButton button,
                       QPlatformDialogHelper::ButtonRole role);

private Q_SLOTS:
    void handleClick(QPlatformDialogHelper::StandardButton button,
                     QPlatformDialogHelper::ButtonRole role);

protected:
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    int dialogCode() const override;

private:
    QSharedPointer<QMessageDialogOptions> m_options;
    QPlatformDialogHelper::ButtonRole m_roleOfLastButtonPressed = QPlatformDialogHelper::NoRole;
};

QT_END_NAMESPACE

#endif // QQUICKMESSAGEDIALOG_P_H
