// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDIALOGBUTTONBOX_P_P_H
#define QQUICKDIALOGBUTTONBOX_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_EXPORT QQuickDialogButtonBoxPrivate : public QQuickContainerPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickDialogButtonBox)

    static QQuickDialogButtonBoxPrivate *get(QQuickDialogButtonBox *box)
    {
        return box->d_func();
    }

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    void resizeContent() override;

    void updateLayout();

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    void handleClick();

    static QString buttonText(QPlatformDialogHelper::StandardButton standardButton);

    QQuickAbstractButton *createStandardButton(QPlatformDialogHelper::StandardButton button);
    void removeStandardButtons();

    void updateLanguage();

    QPlatformDialogHelper::StandardButton standardButton(QQuickAbstractButton *button) const;

    Qt::Alignment alignment;
    QQuickDialogButtonBox::Position position = QQuickDialogButtonBox::Footer;
    QPlatformDialogHelper::StandardButtons standardButtons = QPlatformDialogHelper::NoButton;
    QPlatformDialogHelper::ButtonLayout buttonLayout = QPlatformDialogHelper::UnknownLayout;
    QQmlComponent *delegate = nullptr;
};

class QQuickDialogButtonBoxAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickDialogButtonBoxAttached)

public:
    static QQuickDialogButtonBoxAttachedPrivate *get(QQuickDialogButtonBoxAttached *q)
    {
        return q->d_func();
    }

    void setButtonBox(QQuickDialogButtonBox *box);

    QQuickDialogButtonBox *buttonBox = nullptr;
    QPlatformDialogHelper::ButtonRole buttonRole = QPlatformDialogHelper::InvalidRole;
    QPlatformDialogHelper::StandardButton standardButton = QPlatformDialogHelper::NoButton;
};

QT_END_NAMESPACE

#endif // QQUICKDIALOGBUTTONBOX_P_P_H
