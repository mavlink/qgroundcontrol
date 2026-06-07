// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSIDEBAR_H
#define QSIDEBAR_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <qlist.h>
#include <qlistview.h>
#include <qstandarditemmodel.h>
#include <qstyleditemdelegate.h>
#include <qurl.h>

QT_REQUIRE_CONFIG(filedialog);

QT_BEGIN_NAMESPACE

class QFileSystemModel;

class QSideBarDelegate : public QStyledItemDelegate
{
 public:
     QSideBarDelegate(QWidget *parent = nullptr) : QStyledItemDelegate(parent) {}
     void initStyleOption(QStyleOptionViewItem *option,
                          const QModelIndex &index) const override;
};

class Q_AUTOTEST_EXPORT QUrlModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        EnabledRole = Qt::UserRole + 2
    };

    QUrlModel(QObject *parent = nullptr);
    ~QUrlModel();

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
#if QT_CONFIG(draganddrop)
    bool canDrop(QDragEnterEvent *event);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
#endif
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole) override;

    void setUrls(const QList<QUrl> &list);
    void addUrls(const QList<QUrl> &urls, int row = -1, bool move = true);
    QList<QUrl> urls() const;
    void setFileSystemModel(QFileSystemModel *model);
    bool showFullPath;

private Q_SLOTS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void layoutChanged();

private:
    void setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex);
    void changed(const QString &path);
    void addIndexToWatch(const QString &path, const QModelIndex &index);
    QFileSystemModel *fileSystemModel;
    struct WatchItem {
        QModelIndex index;
        QString path;
    };
    friend class QTypeInfo<WatchItem>;

    QList<WatchItem> watching;
    QList<QUrl> invalidUrls;
    std::array<QMetaObject::Connection, 3> modelConnections;
};
Q_DECLARE_TYPEINFO(QUrlModel::WatchItem, Q_RELOCATABLE_TYPE);

class Q_AUTOTEST_EXPORT QSidebar : public QListView
{
    Q_OBJECT

Q_SIGNALS:
    void goToUrl(const QUrl &url);

public:
    QSidebar(QWidget *parent = nullptr);
    void setModelAndUrls(QFileSystemModel *model, const QList<QUrl> &newUrls);
    ~QSidebar();

    QSize sizeHint() const override;

    void setUrls(const QList<QUrl> &list) { urlModel->setUrls(list); }
    void addUrls(const QList<QUrl> &list, int row) { urlModel->addUrls(list, row); }
    QList<QUrl> urls() const { return urlModel->urls(); }

    void selectUrl(const QUrl &url);

protected:
    bool event(QEvent * e) override;
    void focusInEvent(QFocusEvent *event) override;
#if QT_CONFIG(draganddrop)
    void dragEnterEvent(QDragEnterEvent *event) override;
#endif

private Q_SLOTS:
    void clicked(const QModelIndex &index);
#if QT_CONFIG(menu)
    void showContextMenu(const QPoint &position);
#endif
    void removeEntry();

private:
    QUrlModel *urlModel;
};

QT_END_NAMESPACE

#endif // QSIDEBAR_H

