// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMMESSAGEDIALOG_P_H
#define QQUICKLABSPLATFORMMESSAGEDIALOG_P_H

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

#include "qquicklabsplatformdialog_p.h"
#include <QtQml/qqml.h>

#if QT_DEPRECATED_SINCE(6, 9)

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformMessageDialog : public QQuickLabsPlatformDialog
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MessageDialog)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QString informativeText READ informativeText WRITE setInformativeText NOTIFY informativeTextChanged FINAL)
    Q_PROPERTY(QString detailedText READ detailedText WRITE setDetailedText NOTIFY detailedTextChanged FINAL)
    Q_PROPERTY(QPlatformDialogHelper::StandardButtons buttons READ buttons WRITE setButtons NOTIFY buttonsChanged FINAL)
    QML_EXTENDED_NAMESPACE(QPlatformDialogHelper)

public:
    explicit QQuickLabsPlatformMessageDialog(QObject *parent = nullptr);

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
    void clicked(QPlatformDialogHelper::StandardButton button);

    void okClicked();
    void saveClicked();
    void saveAllClicked();
    void openClicked();
    void yesClicked();
    void yesToAllClicked();
    void noClicked();
    void noToAllClicked();
    void abortClicked();
    void retryClicked();
    void ignoreClicked();
    void closeClicked();
    void cancelClicked();
    void discardClicked();
    void helpClicked();
    void applyClicked();
    void resetClicked();
    void restoreDefaultsClicked();

protected:
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;

private Q_SLOTS:
    void handleClick(QPlatformDialogHelper::StandardButton button);

private:
    QSharedPointer<QMessageDialogOptions> m_options;
};

QT_END_NAMESPACE

#endif // QT_DEPRECATED_SINCE(6, 9)

#endif // QQUICKLABSPLATFORMMESSAGEDIALOG_P_H
