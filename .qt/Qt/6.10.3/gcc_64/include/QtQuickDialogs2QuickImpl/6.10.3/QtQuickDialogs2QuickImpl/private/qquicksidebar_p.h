// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSIDEBAR_P_H
#define QQUICKSIDEBAR_P_H

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
#include <QtCore/qstandardpaths.h>

#include "qquickfiledialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickSideBarPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickSideBar : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialog *dialog READ dialog WRITE setDialog NOTIFY dialogChanged FINAL)
    Q_PROPERTY(QList<QStandardPaths::StandardLocation> folderPaths READ folderPaths WRITE setFolderPaths NOTIFY folderPathsChanged FINAL)
    Q_PROPERTY(QList<QStandardPaths::StandardLocation> effectiveFolderPaths READ effectiveFolderPaths NOTIFY effectiveFolderPathsChanged FINAL)
    Q_PROPERTY(QList<QUrl> favoritePaths READ favoritePaths NOTIFY favoritePathsChanged FINAL)
    Q_PROPERTY(QQmlComponent *buttonDelegate READ buttonDelegate WRITE setButtonDelegate NOTIFY buttonDelegateChanged FINAL)
    Q_PROPERTY(QQmlComponent *separatorDelegate READ separatorDelegate WRITE setSeparatorDelegate NOTIFY separatorDelegateChanged FINAL)
    Q_PROPERTY(QQmlComponent *addFavoriteDelegate READ addFavoriteDelegate WRITE setAddFavoriteDelegate NOTIFY addFavoriteDelegateChanged FINAL)
    QML_NAMED_ELEMENT(SideBar)
    QML_ADDED_IN_VERSION(6, 9)

public:
    explicit QQuickSideBar(QQuickItem *parent = nullptr);
    ~QQuickSideBar();

    QQuickDialog *dialog() const;
    void setDialog(QQuickDialog *dialog);

    QList<QStandardPaths::StandardLocation> folderPaths() const;
    void setFolderPaths(const QList<QStandardPaths::StandardLocation>& folderPaths);

    QList<QStandardPaths::StandardLocation> effectiveFolderPaths() const;

    QList<QUrl> favoritePaths() const;

    QQmlComponent *buttonDelegate() const;
    void setButtonDelegate(QQmlComponent *delegate);

    QQmlComponent *separatorDelegate() const;
    void setSeparatorDelegate(QQmlComponent *delegate);

    QQmlComponent *addFavoriteDelegate() const;
    void setAddFavoriteDelegate(QQmlComponent *delegate);


Q_SIGNALS:
    void dialogChanged();
    void folderPathsChanged();
    void effectiveFolderPathsChanged();
    void favoritePathsChanged();
    void buttonDelegateChanged();
    void separatorDelegateChanged();
    void addFavoriteDelegateChanged();

protected:
    void componentComplete() override;

private:
    void setFavoritePaths(const QList<QUrl>& favoritePaths);

    Q_DISABLE_COPY(QQuickSideBar)
    Q_DECLARE_PRIVATE(QQuickSideBar)
};

QT_END_NAMESPACE

#endif // QQUICKSIDEBAR_P_H
