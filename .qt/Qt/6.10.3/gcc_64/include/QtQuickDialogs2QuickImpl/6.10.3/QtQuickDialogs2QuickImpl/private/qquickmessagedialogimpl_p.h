// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMESSAGEDIALOGIMPL_P_H
#define QQUICKMESSAGEDIALOGIMPL_P_H

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

#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickMessageDialogImplAttached;
class QQuickMessageDialogImplAttachedPrivate;
class QQuickMessageDialogImplPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickMessageDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY optionsChanged)
    Q_PROPERTY(QString informativeText READ informativeText NOTIFY optionsChanged)
    Q_PROPERTY(QString detailedText READ detailedText NOTIFY optionsChanged)
    Q_PROPERTY(bool showDetailedText READ showDetailedText NOTIFY showDetailedTextChanged)
    QML_NAMED_ELEMENT(MessageDialogImpl)
    QML_ATTACHED(QQuickMessageDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 3)
public:
    explicit QQuickMessageDialogImpl(QObject *parent = nullptr);

    static QQuickMessageDialogImplAttached *qmlAttachedProperties(QObject *object);

    QSharedPointer<QMessageDialogOptions> options() const;
    void setOptions(const QSharedPointer<QMessageDialogOptions> &options);

    bool showDetailedText() const;
    QString text() const;
    QString informativeText() const;
    QString detailedText() const;

Q_SIGNALS:
    void buttonClicked(QPlatformDialogHelper::StandardButton button,
                       QPlatformDialogHelper::ButtonRole role);
    void showDetailedTextChanged();
    void optionsChanged();

public Q_SLOTS:
    void toggleShowDetailedText();

private:
    Q_DISABLE_COPY(QQuickMessageDialogImpl)
    Q_DECLARE_PRIVATE(QQuickMessageDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickMessageDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialogButtonBox *buttonBox READ buttonBox WRITE setButtonBox NOTIFY
                       buttonBoxChanged)
    Q_PROPERTY(QQuickButton *detailedTextButton READ detailedTextButton WRITE setDetailedTextButton
                       NOTIFY detailedTextButtonChanged)
public:
    explicit QQuickMessageDialogImplAttached(QObject *parent = nullptr);

    QQuickDialogButtonBox *buttonBox() const;
    void setButtonBox(QQuickDialogButtonBox *buttons);

    QQuickButton *detailedTextButton() const;
    void setDetailedTextButton(QQuickButton *detailedTextButton);

Q_SIGNALS:
    void buttonBoxChanged();
    void detailedTextButtonChanged();

private:
    Q_DISABLE_COPY(QQuickMessageDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickMessageDialogImplAttached)
};

QT_END_NAMESPACE

#endif // QQUICKMESSAGEDIALOGIMPL_P_H
