// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFOLDERBREADCRUMBBAR_P_H
#define QQUICKFOLDERBREADCRUMBBAR_P_H

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

#include <QtQml/qqmlcomponent.h>
#include <QtQuickTemplates2/private/qquickcontainer_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>

#include "qquickfiledialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickFolderBreadcrumbBarPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFolderBreadcrumbBar : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialog *dialog READ dialog WRITE setDialog NOTIFY dialogChanged)
    Q_PROPERTY(QQmlComponent *buttonDelegate READ buttonDelegate WRITE setButtonDelegate NOTIFY buttonDelegateChanged)
    Q_PROPERTY(QQmlComponent *separatorDelegate READ separatorDelegate WRITE setSeparatorDelegate NOTIFY separatorDelegateChanged)
    Q_PROPERTY(QQuickAbstractButton *upButton READ upButton WRITE setUpButton NOTIFY upButtonChanged)
    Q_PROPERTY(QQuickTextField *textField READ textField WRITE setTextField NOTIFY textFieldChanged)
    Q_PROPERTY(int upButtonSpacing READ upButtonSpacing WRITE setUpButtonSpacing NOTIFY upButtonSpacingChanged)
    QML_NAMED_ELEMENT(FolderBreadcrumbBar)
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickFolderBreadcrumbBar(QQuickItem *parent = nullptr);

    QQuickDialog *dialog() const;
    void setDialog(QQuickDialog *dialog);

    QQmlComponent *buttonDelegate();
    void setButtonDelegate(QQmlComponent *delegate);

    QQmlComponent *separatorDelegate();
    void setSeparatorDelegate(QQmlComponent *delegate);

    QQuickAbstractButton *upButton();
    void setUpButton(QQuickAbstractButton *upButton);

    int upButtonSpacing() const;
    void setUpButtonSpacing(int upButtonSpacing);

    QQuickTextField *textField();
    void setTextField(QQuickTextField *textField);

Q_SIGNALS:
    void dialogChanged();
    void buttonDelegateChanged();
    void separatorDelegateChanged();
    void upButtonChanged();
    void upButtonSpacingChanged();
    void textFieldChanged();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void componentComplete() override;

    void itemChange(ItemChange change, const ItemChangeData &data) override;

    bool isContent(QQuickItem *item) const override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickFolderBreadcrumbBar)
    Q_DECLARE_PRIVATE(QQuickFolderBreadcrumbBar)
};

QT_END_NAMESPACE

#endif // QQUICKFOLDERBREADCRUMBBAR_P_H
