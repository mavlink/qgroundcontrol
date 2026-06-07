// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFOLDERBREADCRUMBBAR_P_P_H
#define QQUICKFOLDERBREADCRUMBBAR_P_P_H

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
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractButton;
class QQuickTextField;

class QQuickFileDialogImpl;
class QQuickFolderDialogImpl;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFolderBreadcrumbBarPrivate : public QQuickContainerPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickFolderBreadcrumbBar)

    QQuickItem *createDelegateItem(QQmlComponent *component, const QVariantMap &initialProperties);
    static QString folderBaseName(const QString &folderPath);
    static QStringList crumbPathsForFolder(const QUrl &folder);
    void repopulate();
    void crumbClicked();
    void folderChanged();

    void cancelUpButton();
    void executeUpButton(bool complete = false);
    void goUp();

    void cancelTextField();
    void executeTextField(bool complete = false);
    void toggleTextFieldVisibility();
    void textFieldAccepted();

    void textFieldVisibleChanged();
    void textFieldActiveFocusChanged();
    void handleTextFieldShown();
    void handleTextFieldHidden();
    void ungrabEditPathShortcut();

    QQuickFileDialogImpl *asFileDialog() const;
    QQuickFolderDialogImpl *asFolderDialog() const;
    bool isFileDialog() const;
    QUrl dialogFolder() const;
    void setDialogFolder(const QUrl &folder);

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;
    void resizeContent() override;

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

private:
    QQuickDialog *dialog = nullptr;
    QList<QString> folderPaths;
    QQmlComponent *buttonDelegate = nullptr;
    QQmlComponent *separatorDelegate = nullptr;
    QQuickDeferredPointer<QQuickAbstractButton> upButton;
    QQuickDeferredPointer<QQuickTextField> textField;
    int editPathToggleShortcutId = 0;
#if QT_CONFIG(shortcut)
    int goUpShortcutId = 0;
#endif
    int upButtonSpacing = 0;
    bool repopulating = false;
};

QT_END_NAMESPACE

#endif // QQUICKFOLDERBREADCRUMBBAR_P_P_H
