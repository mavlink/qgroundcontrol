// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLIMPORT_P_H
#define QQMLIMPORT_P_H

#include <QtCore/qurl.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qset.h>
#include <QtCore/qstringlist.h>

#include <QtQml/qqmlabstracturlinterceptor.h>
#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlfile.h>

#include <private/qfieldlist_p.h>
#include <private/qqmldirparser_p.h>
#include <private/qqmltype_p.h>
#include <private/qstringhash_p.h>

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

QT_BEGIN_NAMESPACE

class QQmlTypeNameCache;
class QDir;
class QQmlImportNamespace;
class QQmlTypeLoader;
class QQmlTypeLoaderQmldirContent;
class QTypeRevision;
class QQmlTypeLoader;

const QLoggingCategory &lcQmlImport();

namespace QQmlImport {
    enum RecursionRestriction { PreventRecursion, AllowRecursion };
}

struct QQmlImportInstance
{
    enum Precedence {
        Lowest = std::numeric_limits<quint8>::max(),
        Implicit = Lowest / 2,
        Highest = 0,
    };

    QString uri; // e.g. QtQuick
    QString url; // the base path of the import
    QTypeRevision version; // the version imported

    bool isLibrary; // true means that this is not a file import

    // not covered by precedence. You can set a component as implicitly imported after the fact.
    bool implicitlyImported = false;
    bool isInlineComponent = false;

    quint8 precedence = 0;

    QQmlDirComponents qmlDirComponents; // a copy of the components listed in the qmldir
    QQmlDirScripts qmlDirScripts; // a copy of the scripts in the qmldir

    bool setQmldirContent(const QString &resolvedUrl, const QQmlTypeLoaderQmldirContent &qmldir,
                          QQmlImportNamespace *nameSpace, QList<QQmlError> *errors);

    static QQmlDirScripts getVersionedScripts(const QQmlDirScripts &qmldirscripts,
                                              QTypeRevision version);

    bool resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef &type,
                     QTypeRevision *version_return, QQmlType* type_return,
                     const QString *base = nullptr, bool *typeRecursionDetected = nullptr,
                     QQmlType::RegistrationType = QQmlType::AnyRegistrationType,
                     QQmlImport::RecursionRestriction recursionRestriction = QQmlImport::PreventRecursion,
                     QList<QQmlError> *errors = nullptr) const;
};

class QQmlImportNamespace
{
public:
    QQmlImportNamespace() : nextNamespace(nullptr) {}
    ~QQmlImportNamespace() { qDeleteAll(imports); }

    QList<QQmlImportInstance *> imports;

    QQmlImportInstance *findImportByModuleUri(
            const QString &moduleUri, QTypeRevision version) const;
    QQmlImportInstance *findImportByLocation(
            const QString &location, QTypeRevision version) const;

    bool resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef& type,
                     QTypeRevision *version_return, QQmlType* type_return,
                     const QString *base = nullptr, QList<QQmlError> *errors = nullptr,
                     QQmlType::RegistrationType registrationType = QQmlType::AnyRegistrationType,
                     bool *typeRecursionDeteced = nullptr);

    // Prefix when used as a qualified import.  Otherwise empty.
    QHashedString prefix;

    // Used by QQmlImports::m_qualifiedSets
    // set to this in unqualifiedSet to indicate that the lists of imports needs
    // to be sorted when an inline component import was added
    // We can't use flag pointer, as that does not work with QFieldList
    QQmlImportNamespace *nextNamespace = nullptr;
    bool needsSorting() const { return nextNamespace == this; }
    void setNeedsSorting(bool needsSorting)
    {
        Q_ASSERT(nextNamespace == this || nextNamespace == nullptr);
        nextNamespace = needsSorting ? this : nullptr;
    }
};

class Q_QML_EXPORT QQmlImports final : public QQmlRefCounted<QQmlImports>
{
    Q_DISABLE_COPY_MOVE(QQmlImports)
    Q_DECLARE_TR_FUNCTIONS(QQmlImports)
public:
    enum ImportVersion { FullyVersioned, PartiallyVersioned, Unversioned };

    enum ImportFlag : quint8 {
        ImportNoFlag        = 0x0,
        ImportIncomplete    = 0x1,
    };
    Q_DECLARE_FLAGS(ImportFlags, ImportFlag)

    QQmlImports() = default;
    ~QQmlImports()
    {
        while (QQmlImportNamespace *ns = m_qualifiedSets.takeFirst())
            delete ns;
    }

    void setBaseUrl(const QUrl &url, const QString &urlString = QString());
    QUrl baseUrl() const { return m_baseUrl; }

    bool resolveType(
            QQmlTypeLoader *typeLoader, const QHashedStringRef &type, QQmlType *type_return,
            QTypeRevision *version_return, QQmlImportNamespace **ns_return,
            QList<QQmlError> *errors = nullptr,
            QQmlType::RegistrationType registrationType = QQmlType::AnyRegistrationType,
            bool *typeRecursionDetected = nullptr) const;

    QTypeRevision addImplicitImport(
            QQmlTypeLoader *typeLoader, QString *localQmldir, QList<QQmlError> *errors)
    {
        Q_ASSERT(errors);
        qCDebug(lcQmlImport) << "addImplicitImport:" << qPrintable(baseUrl().toString());

        const ImportFlags flags =
                ImportFlags(!isLocal(baseUrl()) ? ImportIncomplete : ImportNoFlag);
        return addFileImport(
                typeLoader, QLatin1String("."), QString(), QTypeRevision(), flags,
                QQmlImportInstance::Implicit, localQmldir, errors);
    }

    bool addInlineComponentImport(
            QQmlImportInstance  *const importInstance, const QString &name, const QUrl &importUrl);

    QTypeRevision addFileImport(
            QQmlTypeLoader *typeLoader, const QString &uri, const QString &prefix,
            QTypeRevision requestedVersion, ImportFlags flags, quint16 precedence,
            QString *localQmldir, QList<QQmlError> *errors);

    QTypeRevision addLibraryImport(
            QQmlTypeLoader *typeLoader, const QString &uri, const QString &prefix,
            QTypeRevision requestedVersion, const QString &qmldirIdentifier,
            const QString &qmldirUrl, ImportFlags flags, quint16 precedence,
            QList<QQmlError> *errors);

    QTypeRevision updateQmldirContent(
            QQmlTypeLoader *typeLoader, const QString &uri, QTypeRevision version,
            const QString &prefix, const QString &qmldirIdentifier, const QString &qmldirUrl,
            QList<QQmlError> *errors);

    void populateCache(QQmlTypeNameCache *cache) const;

    struct ScriptReference
    {
        QString nameSpace;
        QString qualifier;
        QUrl fileName;
        QUrl location;
    };

    QList<ScriptReference> resolvedScripts() const;

    struct CompositeSingletonReference
    {
        QString typeName;
        QString prefix;
        QTypeRevision version;
    };

    QList<CompositeSingletonReference> resolvedCompositeSingletons() const;

    static QStringList completeQmldirPaths(
            const QString &uri, const QStringList &basePaths, QTypeRevision version);

    static QString versionString(QTypeRevision version, ImportVersion importVersion);

    static bool isLocal(const QString &url)
    {
        return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
    }

    static bool isLocal(const QUrl &url)
    {
        return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
    }

    static QUrl urlFromLocalFileOrQrcOrUrl(const QString &);

    static void setDesignerSupportRequired(bool b);

    static QTypeRevision validVersion(QTypeRevision version = QTypeRevision());

    static QQmlError moduleNotFoundError(const QString &uri, QTypeRevision version);

private:
    QQmlImportNamespace *importNamespace(const QString &prefix);

    bool resolveType(
            QQmlTypeLoader *typeLoader, const QHashedStringRef &type, QTypeRevision *version_return,
            QQmlType *type_return, QList<QQmlError> *errors,
            QQmlType::RegistrationType registrationType,
            bool *typeRecursionDetected = nullptr) const;

    QQmlImportNamespace *findQualifiedNamespace(const QHashedStringRef &) const;

    QTypeRevision importExtension(
            QQmlTypeLoader *typeLoader, const QString &uri, QTypeRevision version,
            const QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors);

    void registerBuiltinModuleTypes(
            const QQmlTypeLoaderQmldirContent &qmldir, QTypeRevision version);

    QString redirectQmldirContent(
            QQmlTypeLoader *typeLoader, QQmlTypeLoaderQmldirContent *qmldir);

    bool getQmldirContent(
            QQmlTypeLoader *typeLoader,  const QString &qmldirIdentifier, const QString &uri,
            QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors);

    QString resolvedUri(const QString &dir_arg, QQmlTypeLoader *typeLoader);

    QUrl m_baseUrl;
    QString m_base;

    // storage of data related to imports without a namespace
    // TODO: This needs to be mutable because QQmlImportNamespace likes to sort itself on
    //       resolveType(). Therefore, QQmlImportNamespace::resolveType() is not const.
    //       There should be a better way to do this.
    mutable QQmlImportNamespace m_unqualifiedset;

    // storage of data related to imports with a namespace
    QFieldList<QQmlImportNamespace, &QQmlImportNamespace::nextNamespace> m_qualifiedSets;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlImports::ImportFlags)

void qmlClearEnginePlugins();// For internal use by qmlClearRegisteredProperties

QT_END_NAMESPACE

#endif // QQMLIMPORT_P_H

