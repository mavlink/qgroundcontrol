// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMIMEPROVIDER_P_H
#define QMIMEPROVIDER_P_H

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

#include "qmimedatabase_p.h"

QT_REQUIRE_CONFIG(mimetype);

#include "qmimeglobpattern_p.h"
#include <QtCore/qdatetime.h>
#include <QtCore/qset.h>

#include <map>

QT_BEGIN_NAMESPACE

class QMimeMagicRuleMatcher;
class QMimeTypeXMLData;
class QMimeProviderBase;

struct QMimeMagicResult
{
    bool isValid() const { return !candidate.isEmpty(); }

    QString candidate;
    int accuracy = 0;
};

class QMimeProviderBase
{
    Q_DISABLE_COPY(QMimeProviderBase)

public:
    QMimeProviderBase(QMimeDatabasePrivate *db, const QString &directory);
    virtual ~QMimeProviderBase() = default;

    virtual bool isValid() = 0;
    virtual bool isInternalDatabase() const = 0;
    virtual bool knowsMimeType(const QString &name) = 0;
    virtual void addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result) = 0;
    virtual void addParents(const QString &mime, QStringList &result) = 0;
    virtual QString resolveAlias(const QString &name) = 0;
    virtual void addAliases(const QString &name, QStringList &result) = 0;
    virtual void findByMagic(const QByteArray &data, QMimeMagicResult &result) = 0;
    virtual void addAllMimeTypes(QList<QMimeType> &result) = 0;
    virtual QMimeTypePrivate::LocaleHash localeComments(const QString &name) = 0;
    virtual bool hasGlobDeleteAll(const QString &name) = 0;
    virtual QStringList globPatterns(const QString &name) = 0;
    virtual QString icon(const QString &name) = 0;
    virtual QString genericIcon(const QString &name) = 0;
    virtual void ensureLoaded() { }

    QString directory() const { return m_directory; }

    QMimeProviderBase *overrideProvider() const;
    void setOverrideProvider(QMimeProviderBase *provider);
    bool isMimeTypeGlobsExcluded(const QString &name) const;

    QMimeDatabasePrivate *m_db;
    QString m_directory;
    QMimeProviderBase *m_overrideProvider = nullptr; // more "local" than this one
};

/*
   Parses the files 'mime.cache' and 'types' on demand
 */
class QMimeBinaryProvider final : public QMimeProviderBase
{
public:
    QMimeBinaryProvider(QMimeDatabasePrivate *db, const QString &directory);
    virtual ~QMimeBinaryProvider();

    bool isValid() override;
    bool isInternalDatabase() const override;
    bool knowsMimeType(const QString &name) override;
    void addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result) override;
    void addParents(const QString &mime, QStringList &result) override;
    QString resolveAlias(const QString &name) override;
    void addAliases(const QString &name, QStringList &result) override;
    void findByMagic(const QByteArray &data, QMimeMagicResult &result) override;
    void addAllMimeTypes(QList<QMimeType> &result) override;
    QMimeTypePrivate::LocaleHash localeComments(const QString &name) override;
    bool hasGlobDeleteAll(const QString &name) override;
    QStringList globPatterns(const QString &name) override;
    QString icon(const QString &name) override;
    QString genericIcon(const QString &name) override;
    void ensureLoaded() override;

private:
    struct CacheFile;

    int matchGlobList(QMimeGlobMatchResult &result, CacheFile *cacheFile, int offset,
                      const QString &fileName);
    bool matchSuffixTree(QMimeGlobMatchResult &result, CacheFile *cacheFile, int numEntries,
                         int firstOffset, const QString &fileName, qsizetype charPos,
                         bool caseSensitiveCheck);
    bool matchMagicRule(CacheFile *cacheFile, int numMatchlets, int firstOffset,
                        const QByteArray &data);
    QLatin1StringView iconForMime(CacheFile *cacheFile, int posListOffset, const QByteArray &inputMime);
    void loadMimeTypeList();
    bool checkCacheChanged();

    std::unique_ptr<CacheFile> m_cacheFile;
    QStringList m_cacheFileNames;
    QSet<QString> m_mimetypeNames;
    bool m_mimetypeListLoaded;
    struct MimeTypeExtra
    {
        QHash<QString, QString> localeComments;
        QStringList globPatterns;
        bool hasGlobDeleteAll = false;
    };
    using MimeTypeExtraMap = std::map<QString, MimeTypeExtra>;
    MimeTypeExtraMap m_mimetypeExtra;

    MimeTypeExtraMap::const_iterator loadMimeTypeExtra(const QString &mimeName);
};

/*
   Parses the raw XML files (slower)
 */
class QMimeXMLProvider final : public QMimeProviderBase
{
public:
    enum InternalDatabaseEnum { InternalDatabase };
#if QT_CONFIG(mimetype_database)
    enum : bool { InternalDatabaseAvailable = true };
#else
    enum : bool { InternalDatabaseAvailable = false };
#endif
    QMimeXMLProvider(QMimeDatabasePrivate *db, InternalDatabaseEnum);
    QMimeXMLProvider(QMimeDatabasePrivate *db, const QString &directory);
    ~QMimeXMLProvider();

    bool isValid() override;
    bool isInternalDatabase() const override;
    bool knowsMimeType(const QString &name) override;
    void addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result) override;
    void addParents(const QString &mime, QStringList &result) override;
    QString resolveAlias(const QString &name) override;
    void addAliases(const QString &name, QStringList &result) override;
    void findByMagic(const QByteArray &data, QMimeMagicResult &result) override;
    void addAllMimeTypes(QList<QMimeType> &result) override;
    void ensureLoaded() override;
    QMimeTypePrivate::LocaleHash localeComments(const QString &name) override;
    bool hasGlobDeleteAll(const QString &name) override;
    QStringList globPatterns(const QString &name) override;
    QString icon(const QString &name) override;
    QString genericIcon(const QString &name) override;

    bool load(const QString &fileName, QString *errorMessage);

    // Called by the mimetype xml parser
    void addMimeType(const QMimeTypeXMLData &mt);
    void addGlobPattern(const QMimeGlobPattern &glob);
    void addParent(const QString &child, const QString &parent);
    void addAlias(const QString &alias, const QString &name);
    void addMagicMatcher(const QMimeMagicRuleMatcher &matcher);

private:
    void load(const QString &fileName);
    void load(const char *data, qsizetype len);

    typedef QHash<QString, QMimeTypeXMLData> NameMimeTypeMap;
    NameMimeTypeMap m_nameMimeTypeMap;

    typedef QHash<QString, QString> AliasHash;
    AliasHash m_aliases;

    typedef QHash<QString, QStringList> ParentsHash;
    ParentsHash m_parents;
    QMimeAllGlobPatterns m_mimeTypeGlobs;

    QList<QMimeMagicRuleMatcher> m_magicMatchers;
    QStringList m_allFiles;
};

QT_END_NAMESPACE

#endif // QMIMEPROVIDER_P_H
