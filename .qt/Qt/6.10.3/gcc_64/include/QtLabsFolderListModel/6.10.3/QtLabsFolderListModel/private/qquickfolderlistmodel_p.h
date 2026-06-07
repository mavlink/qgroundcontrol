// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKFOLDERLISTMODEL_P_H
#define QQUICKFOLDERLISTMODEL_P_H

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

#include "qquickfolderlistmodelglobal_p.h"

#include <QtQml/qqml.h>
#include <QStringList>
#include <QUrl>
#include <QAbstractListModel>

QT_BEGIN_NAMESPACE


class QQmlContext;
class QModelIndex;

class QQuickFolderListModelPrivate;

//![class begin]
class Q_LABSFOLDERLISTMODEL_EXPORT QQuickFolderListModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
//![class begin]

//![class props]
    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged FINAL)
    Q_PROPERTY(QUrl rootFolder READ rootFolder WRITE setRootFolder NOTIFY rootFolderChanged FINAL)
    Q_PROPERTY(QUrl parentFolder READ parentFolder NOTIFY folderChanged FINAL)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters NOTIFY nameFilterChanged FINAL)
    Q_PROPERTY(SortField sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged FINAL)
    Q_PROPERTY(bool sortReversed READ sortReversed WRITE setSortReversed NOTIFY sortReversedChanged FINAL)
    Q_PROPERTY(bool showFiles READ showFiles WRITE setShowFiles NOTIFY showFilesChanged REVISION(2, 1) FINAL)
    Q_PROPERTY(bool showDirs READ showDirs WRITE setShowDirs NOTIFY showDirsChanged FINAL)
    Q_PROPERTY(bool showDirsFirst READ showDirsFirst WRITE setShowDirsFirst NOTIFY showDirsFirstChanged FINAL)
    Q_PROPERTY(bool showDotAndDotDot READ showDotAndDotDot WRITE setShowDotAndDotDot NOTIFY showDotAndDotDotChanged FINAL)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged REVISION(2, 1) FINAL)
    Q_PROPERTY(bool showOnlyReadable READ showOnlyReadable WRITE setShowOnlyReadable NOTIFY  showOnlyReadableChanged FINAL)
    Q_PROPERTY(bool caseSensitive READ caseSensitive WRITE setCaseSensitive NOTIFY  caseSensitiveChanged REVISION(2, 2) FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged REVISION(2, 11) FINAL)
    Q_PROPERTY(bool sortCaseSensitive READ sortCaseSensitive WRITE setSortCaseSensitive NOTIFY sortCaseSensitiveChanged REVISION(2, 12) FINAL)
//![class props]

    QML_NAMED_ELEMENT(FolderListModel)
    QML_ADDED_IN_VERSION(1, 0)
//![abslistmodel]
public:
    QQuickFolderListModel(QObject *parent = nullptr);
    ~QQuickFolderListModel();

    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole = Qt::UserRole + 2,
        FileBaseNameRole = Qt::UserRole + 3,
        FileSuffixRole = Qt::UserRole + 4,
        FileSizeRole = Qt::UserRole + 5,
        FileLastModifiedRole = Qt::UserRole + 6,
        FileLastReadRole = Qt::UserRole +7,
        FileIsDirRole = Qt::UserRole + 8,
        FileUrlRole = Qt::UserRole + 9,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
//![abslistmodel]

//![count]
    int count() const { return rowCount(QModelIndex()); }
//![count]

//![prop funcs]
    QUrl folder() const;
    void setFolder(const QUrl &folder);
    QUrl rootFolder() const;
    void setRootFolder(const QUrl &path);

    QUrl parentFolder() const;

    QStringList nameFilters() const;
    void setNameFilters(const QStringList &filters);

    enum SortField { Unsorted, Name, Time, Size, Type };
    Q_ENUM(SortField)
    SortField sortField() const;
    void setSortField(SortField field);

    bool sortReversed() const;
    void setSortReversed(bool rev);

    bool showFiles() const;
    void setShowFiles(bool showFiles);
    bool showDirs() const;
    void setShowDirs(bool showDirs);
    bool showDirsFirst() const;
    void setShowDirsFirst(bool showDirsFirst);
    bool showDotAndDotDot() const;
    void setShowDotAndDotDot(bool on);
    bool showHidden() const;
    void setShowHidden(bool on);
    bool showOnlyReadable() const;
    void setShowOnlyReadable(bool on);
    bool caseSensitive() const;
    void setCaseSensitive(bool on);

    enum Status { Null, Ready, Loading };
    Q_ENUM(Status)
    Status status() const;
    bool sortCaseSensitive() const;
    void setSortCaseSensitive(bool on);
//![prop funcs]

    Q_INVOKABLE bool isFolder(int index) const;
    Q_INVOKABLE QVariant get(int idx, const QString &property) const;
    Q_INVOKABLE int indexOf(const QUrl &file) const;

//![parserstatus]
    void classBegin() override;
    void componentComplete() override;
//![parserstatus]

    int roleFromString(const QString &roleName) const;

//![notifier]
Q_SIGNALS:
    void folderChanged();
    void rowCountChanged() const;
    void rootFolderChanged();
    void nameFilterChanged();
    void sortFieldChanged();
    void sortReversedChanged();
    void showFilesChanged();
    void showDirsChanged();
    void showDirsFirstChanged();
    void showDotAndDotDotChanged();
    void showHiddenChanged();
    void showOnlyReadableChanged();
    void caseSensitiveChanged();
    void sortCaseSensitiveChanged();

    Q_REVISION(2, 1) void countChanged() const;
    Q_REVISION(2, 11) void statusChanged();

//![notifier]

//![class end]


private:
    Q_DISABLE_COPY(QQuickFolderListModel)
    Q_DECLARE_PRIVATE(QQuickFolderListModel)
    QScopedPointer<QQuickFolderListModelPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString &directory, const QList<FileProperty> &list))
    Q_PRIVATE_SLOT(d_func(), void _q_directoryUpdated(const QString &directory, const QList<FileProperty> &list, int fromIndex, int toIndex))
    Q_PRIVATE_SLOT(d_func(), void _q_sortFinished(const QList<FileProperty> &list))
    Q_PRIVATE_SLOT(d_func(), void _q_statusChanged(QQuickFolderListModel::Status s))
};
//![class end]

QT_END_NAMESPACE

#endif // QQUICKFOLDERLISTMODEL_P_H
