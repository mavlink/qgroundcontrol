// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QFILESYSTEMMODEL_P_H
#define QFILESYSTEMMODEL_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "qfilesystemmodel.h"

#include <private/qabstractitemmodel_p.h>
#include <qabstractitemmodel.h>
#include "qfileinfogatherer_p.h"
#include <qdir.h>
#include <qicon.h>
#include <qfileinfo.h>
#include <qtimer.h>
#include <qhash.h>

#include <vector>

QT_REQUIRE_CONFIG(filesystemmodel);

QT_BEGIN_NAMESPACE

class ExtendedInformation;
class QFileSystemModelPrivate;
class QFileIconProvider;

#if defined(Q_OS_WIN)
class QFileSystemModelNodePathKey : public QString
{
public:
    QFileSystemModelNodePathKey() {}
    QFileSystemModelNodePathKey(const QString &other) : QString(other) {}
    QFileSystemModelNodePathKey(const QFileSystemModelNodePathKey &other) : QString(other) {}
    bool operator==(const QFileSystemModelNodePathKey &other) const { return !compare(other, Qt::CaseInsensitive); }
};

Q_DECLARE_TYPEINFO(QFileSystemModelNodePathKey, Q_RELOCATABLE_TYPE);

inline size_t qHash(const QFileSystemModelNodePathKey &key, size_t seed = 0)
{
    return qHash(key.toCaseFolded(), seed);
}
#else // Q_OS_WIN
typedef QString QFileSystemModelNodePathKey;
#endif

class Q_GUI_EXPORT QFileSystemModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QFileSystemModel)

public:
    enum {
        NameColumn,
        SizeColumn,
        TypeColumn,
        TimeColumn,
        NumColumns = 4
    };

    class QFileSystemNode
    {
    public:
        Q_DISABLE_COPY_MOVE(QFileSystemNode)

        explicit QFileSystemNode(const QString &filename = QString(), QFileSystemNode *p = nullptr)
            : fileName(filename), parent(p) {}
        ~QFileSystemNode() {
            qDeleteAll(children);
            delete info;
        }

        QString fileName;
#if defined(Q_OS_WIN)
        QString volumeName;
#endif

        inline qint64 size() const { if (info && !info->isDir()) return info->size(); return 0; }
        inline QString type() const { if (info) return info->displayType; return QLatin1StringView(""); }
        inline QDateTime lastModified(const QTimeZone &tz) const { return info ? info->lastModified(tz) : QDateTime(); }
        inline QFile::Permissions permissions() const { if (info) return info->permissions(); return { }; }
        inline bool isReadable() const { return ((permissions() & QFile::ReadUser) != 0); }
        inline bool isWritable() const { return ((permissions() & QFile::WriteUser) != 0); }
        inline bool isExecutable() const { return ((permissions() & QFile::ExeUser) != 0); }
        inline bool isDir() const {
            if (info)
                return info->isDir();
            if (children.size() > 0)
                return true;
            return false;
        }
        inline QFileInfo fileInfo() const { if (info) return info->fileInfo(); return QFileInfo(); }
        inline bool isFile() const { if (info) return info->isFile(); return true; }
        inline bool isSystem() const { if (info) return info->isSystem(); return true; }
        inline bool isHidden() const { if (info) return info->isHidden(); return false; }
        inline bool isSymLink(bool ignoreNtfsSymLinks = false) const { return info && info->isSymLink(ignoreNtfsSymLinks); }
        inline bool caseSensitive() const { if (info) return info->isCaseSensitive(); return false; }
        inline QIcon icon() const { if (info) return info->icon; return QIcon(); }

        inline bool operator <(const QFileSystemNode &node) const {
            if (caseSensitive() || node.caseSensitive())
                return fileName < node.fileName;
            return QString::compare(fileName, node.fileName, Qt::CaseInsensitive) < 0;
        }
        inline bool operator >(const QString &name) const {
            if (caseSensitive())
                return fileName > name;
            return QString::compare(fileName, name, Qt::CaseInsensitive) > 0;
        }
        inline bool operator <(const QString &name) const {
            if (caseSensitive())
                return fileName < name;
            return QString::compare(fileName, name, Qt::CaseInsensitive) < 0;
        }
        inline bool operator !=(const QExtendedInformation &fileInfo) const {
            return !operator==(fileInfo);
        }
        bool operator ==(const QString &name) const {
            if (caseSensitive())
                return fileName == name;
            return QString::compare(fileName, name, Qt::CaseInsensitive) == 0;
        }
        bool operator ==(const QExtendedInformation &fileInfo) const {
            return info && (*info == fileInfo);
        }

        inline bool hasInformation() const { return info != nullptr; }

        void populate(const QExtendedInformation &fileInfo) {
            if (!info)
                info = new QExtendedInformation(fileInfo.fileInfo());
            (*info) = fileInfo;
        }

        // children shouldn't normally be accessed directly, use node()
        inline int visibleLocation(const QString &childName) {
            return visibleChildren.indexOf(childName);
        }
        void updateIcon(QAbstractFileIconProvider *iconProvider, const QString &path) {
            if (!iconProvider)
                return;

            if (info)
                info->icon = iconProvider->icon(QFileInfo(path));

            for (QFileSystemNode *child : std::as_const(children)) {
                //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)
                if (!path.isEmpty()) {
                    if (path.endsWith(u'/'))
                        child->updateIcon(iconProvider, path + child->fileName);
                    else
                        child->updateIcon(iconProvider, path + u'/' + child->fileName);
                } else
                    child->updateIcon(iconProvider, child->fileName);
            }
        }

        void retranslateStrings(QAbstractFileIconProvider *iconProvider, const QString &path) {
            if (!iconProvider)
                return;

            if (info)
                info->displayType = iconProvider->type(QFileInfo(path));
            for (QFileSystemNode *child : std::as_const(children)) {
                //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)
                if (!path.isEmpty()) {
                    if (path.endsWith(u'/'))
                        child->retranslateStrings(iconProvider, path + child->fileName);
                    else
                        child->retranslateStrings(iconProvider, path + u'/' + child->fileName);
                } else
                    child->retranslateStrings(iconProvider, child->fileName);
            }
        }

        QHash<QFileSystemModelNodePathKey, QFileSystemNode *> children;
        QList<QString> visibleChildren;
        QExtendedInformation *info = nullptr;
        QFileSystemNode *parent;
        int dirtyChildrenIndex = -1;
        bool populatedChildren = false;
        bool isVisible = false;
    };

    QFileSystemModelPrivate();
    ~QFileSystemModelPrivate();
    void init();
    /*
      \internal

      Return true if index which is owned by node is hidden by the filter.
    */
    inline bool isHiddenByFilter(QFileSystemNode *indexNode, const QModelIndex &index) const
    {
       return (indexNode != &root && !index.isValid());
    }
    QFileSystemNode *node(const QModelIndex &index) const;
    QFileSystemNode *node(const QString &path, bool fetch = true) const;
    inline QModelIndex index(const QString &path, int column = 0) { return index(node(path), column); }
    QModelIndex index(const QFileSystemNode *node, int column = 0) const;
    bool filtersAcceptsNode(const QFileSystemNode *node) const;
    bool passNameFilters(const QFileSystemNode *node) const;
    void removeNode(QFileSystemNode *parentNode, const QString &name);
    QFileSystemNode* addNode(QFileSystemNode *parentNode, const QString &fileName, const QFileInfo &info);
    void addVisibleFiles(QFileSystemNode *parentNode, const QStringList &newFiles);
    void removeVisibleFile(QFileSystemNode *parentNode, int visibleLocation);
    void sortChildren(int column, const QModelIndex &parent);

    inline int translateVisibleLocation(QFileSystemNode *parent, int row) const {
        if (sortOrder != Qt::AscendingOrder) {
            if (parent->dirtyChildrenIndex == -1)
                return parent->visibleChildren.size() - row - 1;

            if (row < parent->dirtyChildrenIndex)
                return parent->dirtyChildrenIndex - row - 1;
        }

        return row;
    }

    inline static QString myComputer() {
        // ### TODO We should query the system to find out what the string should be
        // XP == "My Computer",
        // Vista == "Computer",
        // OS X == "Computer" (sometime user generated) "Benjamin's PowerBook G4"
#ifdef Q_OS_WIN
        return QFileSystemModel::tr("My Computer");
#else
        return QFileSystemModel::tr("Computer");
#endif
    }

    inline void delayedSort() {
        if (!delayedSortTimer.isActive())
            delayedSortTimer.start(0);
    }

    QIcon icon(const QModelIndex &index) const;
    QString name(const QModelIndex &index) const;
    QString displayName(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;
    QString size(const QModelIndex &index) const;
    static QString size(qint64 bytes);
    QString type(const QModelIndex &index) const;
    QString time(const QModelIndex &index) const;

    void directoryChanged(const QString &directory, const QStringList &list);
    void performDelayedSort();
    void fileSystemChanged(const QString &path, const QList<std::pair<QString, QFileInfo>> &);
    void resolvedName(const QString &fileName, const QString &resolvedName);

    QDir rootDir;
#if QT_CONFIG(filesystemwatcher)
#  ifdef Q_OS_WIN
    QStringList unwatchPathsAt(const QModelIndex &);
    void watchPaths(const QStringList &paths) { fileInfoGatherer->watchPaths(paths); }
#  endif // Q_OS_WIN
    std::unique_ptr<QFileInfoGatherer> fileInfoGatherer;
#endif // filesystemwatcher
    QTimer delayedSortTimer;
    QHash<const QFileSystemNode*, bool> bypassFilters;
#if QT_CONFIG(regularexpression)
    QStringList nameFilters;
    std::vector<QRegularExpression> nameFiltersRegexps;
    void rebuildNameFilterRegexps();
#endif
    QHash<QString, QString> resolvedSymLinks;

    QFileSystemNode root;

    struct Fetching {
        QString dir;
        QString file;
        const QFileSystemNode *node;
    };
    QList<Fetching> toFetch;

    QBasicTimer fetchingTimer;

    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs;
    int sortColumn = 0;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    bool forceSort = true;
    bool readOnly = true;
    bool setRootPath = false;
    bool nameFilterDisables = true; // false on windows, true on mac and unix
    // This flag is an optimization for QFileDialog. It enables a sort which is
    // not recursive, meaning we sort only what we see.
    bool disableRecursiveSort = false;
};
Q_DECLARE_TYPEINFO(QFileSystemModelPrivate::Fetching, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
