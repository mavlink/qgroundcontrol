// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPCOLLECTIONHANDLER_H
#define QHELPCOLLECTIONHANDLER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the help generator tools. This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//

#include "qhelpdbreader_p.h"
#include "qhelplink.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QHelpFilterData;
class QSqlQuery;
class QVariant;
class QVersionNumber;

class QHelpCollectionHandler : public QObject
{
    Q_OBJECT

public:
    struct FileInfo
    {
        QString fileName;
        QString folderName;
        QString namespaceName;
    };
    typedef QList<FileInfo> FileInfoList;

    struct TimeStamp
    {
        int namespaceId = -1;
        int folderId = -1;
        QString fileName;
        int size = 0;
        QDateTime timeStamp;
    };

    struct ContentsData
    {
        QString namespaceName;
        QString folderName;
        QList<QByteArray> contentsList;
    };

    explicit QHelpCollectionHandler(const QString &collectionFile, QObject *parent = nullptr);
    ~QHelpCollectionHandler();

    QString collectionFile() const { return m_collectionFile; }

    bool openCollectionFile();
    bool copyCollectionFile(const QString &fileName);

    // *** Legacy block start ***
    //  legacy API since Qt 5.13

    // use filters() instead
    QStringList customFilters() const;

    // use QHelpFilterEngine::removeFilter() instead
    bool removeCustomFilter(const QString &filterName);

    // use QHelpFilterEngine::setFilterData() instead
    bool addCustomFilter(const QString &filterName, const QStringList &attributes);

    // use files(const QString &, const QString &, const QString &) instead
    QStringList files(const QString &namespaceName,
                      const QStringList &filterAttributes,
                      const QString &extensionFilter) const;

    // use namespaceForFile(const QUrl &, const QString &) instead
    QString namespaceForFile(const QUrl &url, const QStringList &filterAttributes) const;

    // use findFile(const QUrl &, const QString &) instead
    QUrl findFile(const QUrl &url, const QStringList &filterAttributes) const;

    // use indicesForFilter(const QString &) instead
    QStringList indicesForFilter(const QStringList &filterAttributes) const;

    // use contentsForFilter(const QString &) instead
    QList<ContentsData> contentsForFilter(const QStringList &filterAttributes) const;

    // use QHelpFilterEngine::activeFilter() and filterData(const QString &) instead;
    QStringList filterAttributes() const;

    // use filterData(const QString &) instead
    QStringList filterAttributes(const QString &filterName) const;

    // use filterData(const QString &) instead
    QList<QStringList> filterAttributeSets(const QString &namespaceName) const;

    // *** Legacy block end ***

    QStringList filters() const;

    QStringList availableComponents() const;
    QList<QVersionNumber> availableVersions() const;
    QMap<QString, QString> namespaceToComponent() const;
    QMap<QString, QVersionNumber> namespaceToVersion() const;
    QHelpFilterData filterData(const QString &filterName) const;
    bool setFilterData(const QString &filterName, const QHelpFilterData &filterData);
    bool removeFilter(const QString &filterName);

    FileInfo registeredDocumentation(const QString &namespaceName) const;
    FileInfoList registeredDocumentations() const;
    bool registerDocumentation(const QString &fileName);
    bool unregisterDocumentation(const QString &namespaceName);

    bool fileExists(const QUrl &url) const;
    QStringList files(const QString &namespaceName,
                      const QString &filterName,
                      const QString &extensionFilter) const;
    QString namespaceForFile(const QUrl &url, const QString &filterName) const;
    QUrl findFile(const QUrl &url, const QString &filterName) const;
    QByteArray fileData(const QUrl &url) const;

    QStringList indicesForFilter(const QString &filterName) const;
    QList<ContentsData> contentsForFilter(const QString &filterName) const;

    bool removeCustomValue(const QString &key);
    QVariant customValue(const QString &key, const QVariant &defaultValue) const;
    bool setCustomValue(const QString &key, const QVariant &value);

    int registerNamespace(const QString &nspace, const QString &fileName);
    int registerVirtualFolder(const QString &folderName, int namespaceId);
    int registerComponent(const QString &componentName, int namespaceId);
    bool registerVersion(const QString &version, int namespaceId);

    QList<QHelpLink> documentsForIdentifier(const QString &id, const QString &filterName) const;
    QList<QHelpLink> documentsForKeyword(const QString &keyword, const QString &filterName) const;
    QList<QHelpLink> documentsForIdentifier(const QString &id,
                                            const QStringList &filterAttributes) const;
    QList<QHelpLink> documentsForKeyword(const QString &keyword,
                                         const QStringList &filterAttributes) const;

    QStringList namespacesForFilter(const QString &filterName) const;

    void setReadOnly(bool readOnly) { m_readOnly = readOnly; }

    static QUrl buildQUrl(const QString &ns, const QString &folder,
                          const QString &relFileName, const QString &anchor);

signals:
    void error(const QString &msg);

private:
    // legacy stuff
    QList<QHelpLink> documentsForField(const QString &fieldName,
                                       const QString &fieldValue,
                                       const QStringList &filterAttributes) const;

    QString namespaceVersion(const QString &namespaceName) const;
    QMultiMap<QString, QUrl> linksForField(const QString &fieldName, const QString &fieldValue,
                                           const QString &filterName) const;
    QList<QHelpLink> documentsForField(const QString &fieldName,
                                       const QString &fieldValue,
                                       const QString &filterName) const;

    bool isDBOpened() const;
    bool createTables(QSqlQuery *query);
    void closeDB();
    bool recreateIndexAndNamespaceFilterTables(QSqlQuery *query);
    bool registerIndexAndNamespaceFilterTables(const QString &nameSpace,
                                               bool createDefaultVersionFilter = false);
    void createVersionFilter(const QString &version);
    bool registerFilterAttributes(const QList<QStringList> &attributeSets, int nsId);
    bool registerFileAttributeSets(const QList<QStringList> &attributeSets, int nsId);
    bool registerIndexTable(const QHelpDBReader::IndexTable &indexTable,
                            int nsId, int vfId, const QString &fileName);
    bool unregisterIndexTable(int nsId, int vfId);
    QString absoluteDocPath(const QString &fileName) const;
    bool isTimeStampCorrect(const TimeStamp &timeStamp) const;
    bool hasTimeStampInfo(const QString &nameSpace) const;
    void scheduleVacuum();
    void execVacuum();

    QString m_collectionFile;
    QString m_connectionName;
    std::unique_ptr<QSqlQuery> m_query;
    bool m_vacuumScheduled = false;
    bool m_readOnly = true;
};

QT_END_NAMESPACE

#endif // QHELPCOLLECTIONHANDLER_H
