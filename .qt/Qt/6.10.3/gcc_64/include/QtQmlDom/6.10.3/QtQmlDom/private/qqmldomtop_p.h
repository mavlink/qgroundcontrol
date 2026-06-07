// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef DOMTOP_H
#define DOMTOP_H

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

#include "qqmldomitem_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomexternalitems_p.h"

#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QDateTime>

#include <QtCore/QCborValue>
#include <QtCore/QCborMap>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT ExternalItemPairBase: public OwningItem { // all access should have the lock of the DomUniverse containing this
    Q_DECLARE_TR_FUNCTIONS(ExternalItemPairBase);
public:
    constexpr static DomType kindValue = DomType::ExternalItemPair;
    DomType kind() const final override { return kindValue; }
    ExternalItemPairBase(
            const QDateTime &validExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : OwningItem(derivedFrom, lastDataUpdateAt),
          validExposedAt(validExposedAt),
          currentExposedAt(currentExposedAt)
    {}
    ExternalItemPairBase(const ExternalItemPairBase &o):
        OwningItem(o), validExposedAt(o.validExposedAt), currentExposedAt(o.currentExposedAt)
    {}
    virtual std::shared_ptr<ExternalOwningItem> validItem() const = 0;
    virtual DomItem validItem(const DomItem &self) const = 0;
    virtual std::shared_ptr<ExternalOwningItem> currentItem() const = 0;
    virtual DomItem currentItem(const DomItem &self) const = 0;

    QString canonicalFilePath(const DomItem &) const final override;
    Path canonicalPath(const DomItem &self) const final override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const final override;
    DomItem field(const DomItem &self, QStringView name) const final override
    {
        return OwningItem::field(self, name);
    }

    bool currentIsValid() const;

    std::shared_ptr<ExternalItemPairBase> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemPairBase>(doCopy(self));
    }

    QDateTime lastDataUpdateAt() const final override
    {
        if (currentItem())
            return currentItem()->lastDataUpdateAt();
        return ExternalItemPairBase::lastDataUpdateAt();
    }

    void refreshedDataAt(QDateTime tNew) final override
    {
        if (currentItem())
            currentItem()->refreshedDataAt(tNew);
        return OwningItem::refreshedDataAt(tNew);
    }

    friend class DomUniverse;

    QDateTime validExposedAt;
    QDateTime currentExposedAt;
};

template<class T>
class QMLDOM_EXPORT ExternalItemPair final : public ExternalItemPairBase
{ // all access should have the lock of the DomUniverse containing this
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<ExternalItemPair>(*this);
    }

public:
    constexpr static DomType kindValue = DomType::ExternalItemPair;
    friend class DomUniverse;
    ExternalItemPair(
            const std::shared_ptr<T> &valid = {}, const std::shared_ptr<T> &current = {},
            const QDateTime &validExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : ExternalItemPairBase(validExposedAt, currentExposedAt, derivedFrom, lastDataUpdateAt),
          valid(valid),
          current(current)
    {}
    ExternalItemPair(const ExternalItemPair &o):
        ExternalItemPairBase(o), valid(o.valid), current(o.current)
    {
    }
    std::shared_ptr<ExternalOwningItem> validItem() const override { return valid; }
    DomItem validItem(const DomItem &self) const override { return self.copy(valid); }
    std::shared_ptr<ExternalOwningItem> currentItem() const override { return current; }
    DomItem currentItem(const DomItem &self) const override { return self.copy(current); }
    std::shared_ptr<ExternalItemPair> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemPair>(doCopy(self));
    }

    std::shared_ptr<T> valid;
    std::shared_ptr<T> current;
};

class QMLDOM_EXPORT DomTop: public OwningItem {
public:
    DomTop(QMap<QString, OwnerT> extraOwningItems = {}, int derivedFrom = 0)
        : OwningItem(derivedFrom), m_extraOwningItems(extraOwningItems)
    {}
    DomTop(const DomTop &o):
        OwningItem(o)
    {
        QMap<QString, OwnerT> items = o.extraOwningItems();
        {
            QMutexLocker l(mutex());
            m_extraOwningItems = items;
        }
    }
    using Callback = DomItem::Callback;

    virtual Path canonicalPath() const = 0;

    Path canonicalPath(const DomItem &) const override;
    DomItem containingObject(const DomItem &) const override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    template<typename T>
    void setExtraOwningItem(const QString &fieldName, const std::shared_ptr<T> &item)
    {
        QMutexLocker l(mutex());
        if (!item)
            m_extraOwningItems.remove(fieldName);
        else
            m_extraOwningItems.insert(fieldName, item);
    }

    void clearExtraOwningItems();
    QMap<QString, OwnerT> extraOwningItems() const;

private:
    QMap<QString, OwnerT> m_extraOwningItems;
};

class QMLDOM_EXPORT DomUniverse final : public DomTop,
                                        public std::enable_shared_from_this<DomUniverse>
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(DomUniverse);
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

public:
    constexpr static DomType kindValue = DomType::DomUniverse;
    DomType kind() const override {  return kindValue; }

    static ErrorGroups myErrors();

    DomUniverse(const QString &universeName);
    DomUniverse(const DomUniverse &) = delete;
    static std::shared_ptr<DomUniverse> guaranteeUniverse(const std::shared_ptr<DomUniverse> &univ);
    static DomItem create(const QString &universeName);

    Path canonicalPath() const override;
    using DomTop::canonicalPath;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    std::shared_ptr<DomUniverse> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<DomUniverse>(doCopy(self));
    }

    // Helper structure reflecting the change in the map once loading && parsing is completed
    // formerItem - DomItem representing value (ExternalItemPair) existing in the map before the
    // loading && parsing. Might be empty (if didn't exist / failure) or equal to currentItem
    // currentItem - DomItem representing current map value
    struct LoadResult
    {
        DomItem formerItem;
        DomItem currentItem;
    };

    LoadResult loadFile(const FileToLoad &file, DomType fileType,
                        DomCreationOption creationOption = {});

    void removePath(const QString &dir);

    std::shared_ptr<ExternalItemPair<GlobalScope>> globalScopeWithName(const QString &name) const
    {
        QMutexLocker l(mutex());
        return m_globalScopeWithName.value(name);
    }

    std::shared_ptr<ExternalItemPair<GlobalScope>> ensureGlobalScopeWithName(const QString &name)
    {
        if (auto current = globalScopeWithName(name))
            return current;
        auto newScope = std::make_shared<GlobalScope>(name);
        auto newValue = std::make_shared<ExternalItemPair<GlobalScope>>(
                newScope, newScope);
        QMutexLocker l(mutex());
        if (auto current = m_globalScopeWithName.value(name))
            return current;
        m_globalScopeWithName.insert(name, newValue);
        return newValue;
    }

    QSet<QString> globalScopeNames() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<GlobalScope>>> map;
        {
            QMutexLocker l(mutex());
            map = m_globalScopeWithName;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmlDirectory>> qmlDirectoryWithPath(const QString &path) const
    {
        QMutexLocker l(mutex());
        return m_qmlDirectoryWithPath.value(path);
    }
    QSet<QString> qmlDirectoryPaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmlDirectory>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmlDirectoryWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmldirFile>> qmldirFileWithPath(const QString &path) const
    {
        QMutexLocker l(mutex());
        return m_qmldirFileWithPath.value(path);
    }
    QSet<QString> qmldirFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmldirFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmldirFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmlFile>> qmlFileWithPath(const QString &path) const
    {
        QMutexLocker l(mutex());
        return m_qmlFileWithPath.value(path);
    }
    QSet<QString> qmlFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmlFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmlFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<JsFile>> jsFileWithPath(const QString &path) const
    {
        QMutexLocker l(mutex());
        return m_jsFileWithPath.value(path);
    }
    QSet<QString> jsFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<JsFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_jsFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmltypesFile>> qmltypesFileWithPath(const QString &path) const
    {
        QMutexLocker l(mutex());
        return m_qmltypesFileWithPath.value(path);
    }
    QSet<QString> qmltypesFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmltypesFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmltypesFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    QString name() const {
        return m_name;
    }

private:
    struct ContentWithDate
    {
        QString content;
        QDateTime date;
    };
    // contains either Content with the timestamp when it was read or an Error
    using ReadResult = std::variant<ContentWithDate, ErrorMessage>;
    ReadResult readFileContent(const QString &canonicalPath) const;

    LoadResult load(const ContentWithDate &codeWithDate, const FileToLoad &file, DomType fType,
                    DomCreationOption creationOption = {});

    // contains either Content to be parsed or LoadResult if loading / parsing is not needed
    using PreloadResult = std::variant<ContentWithDate, LoadResult>;
    PreloadResult preload(const DomItem &univ, const FileToLoad &file, DomType fType) const;

    std::shared_ptr<QmlFile> parseQmlFile(const QString &code, const FileToLoad &file,
                                          const QDateTime &contentDate,
                                          DomCreationOption creationOption);
    std::shared_ptr<JsFile> parseJsFile(const QString &code, const FileToLoad &file,
                                        const QDateTime &contentDate);
    std::shared_ptr<ExternalItemPairBase> getPathValueOrNull(DomType fType,
                                                             const QString &path) const;
    std::optional<DomItem> getItemIfMostRecent(const DomItem &univ, DomType fType,
                                               const QString &path) const;
    std::optional<DomItem> getItemIfHasSameCode(const DomItem &univ, DomType fType,
                                                const QString &canonicalPath,
                                                const ContentWithDate &codeWithDate) const;
    static bool valueHasMostRecentItem(const ExternalItemPairBase *value,
                                       const QDateTime &lastModified);
    static bool valueHasSameContent(const ExternalItemPairBase *value, const QString &content);

    // TODO better name / consider proper public get/set
    template <typename T>
    QMap<QString, std::shared_ptr<ExternalItemPair<T>>> &getMutableRefToMap()
    {
        Q_ASSERT(!mutex()->tryLock());
        if constexpr (std::is_same_v<T, QmlDirectory>) {
            return m_qmlDirectoryWithPath;
        }
        if constexpr (std::is_same_v<T, QmldirFile>) {
            return m_qmldirFileWithPath;
        }
        if constexpr (std::is_same_v<T, QmlFile>) {
            return m_qmlFileWithPath;
        }
        if constexpr (std::is_same_v<T, JsFile>) {
            return m_jsFileWithPath;
        }
        if constexpr (std::is_same_v<T, QmltypesFile>) {
            return m_qmltypesFileWithPath;
        }
        if constexpr (std::is_same_v<T, GlobalScope>) {
            return m_globalScopeWithName;
        }
        Q_UNREACHABLE();
    }

    // Inserts or updates an entry reflecting ExternalItem in the corresponding map
    // Returns a pair of:
    // - current ExternalItemPair, current value in the map (might be empty, or equal to curValue)
    // - new current ExternalItemPair, value in the map after after the execution of this function
    template <typename T>
    std::pair<std::shared_ptr<ExternalItemPair<T>>, std::shared_ptr<ExternalItemPair<T>>>
    insertOrUpdateEntry(std::shared_ptr<T> newItem)
    {
        std::shared_ptr<ExternalItemPair<T>> curValue;
        std::shared_ptr<ExternalItemPair<T>> newCurValue;
        QString canonicalPath = newItem->canonicalFilePath();
        QDateTime now = QDateTime::currentDateTimeUtc();
        {
            QMutexLocker l(mutex());
            auto &map = getMutableRefToMap<T>();
            auto it = map.find(canonicalPath);
            if (it != map.cend() && (*it) && (*it)->current) {
                curValue = *it;
                if (valueHasSameContent(curValue.get(), newItem->code())) {
                    // value in the map has same content as newItem, a.k.a. most recent
                    newCurValue = curValue;
                    if (newCurValue->current->lastDataUpdateAt() < newItem->lastDataUpdateAt()) {
                        // update timestamp in the current, as if its content was refreshed by
                        // NewItem
                        newCurValue->current->refreshedDataAt(newItem->lastDataUpdateAt());
                    }
                } else if (curValue->current->lastDataUpdateAt() > newItem->lastDataUpdateAt()) {
                    // value in the map is more recent than newItem, nothing to update
                    newCurValue = curValue;
                } else {
                    // perform update with newItem
                    curValue->current = std::move(newItem);
                    curValue->currentExposedAt = now;
                    if (curValue->current->isValid()) {
                        curValue->valid = curValue->current;
                        curValue->validExposedAt = std::move(now);
                    }
                    newCurValue = curValue;
                }
            } else {
                // not found / invalid, just insert
                newCurValue = std::make_shared<ExternalItemPair<T>>(
                        (newItem->isValid() ? newItem : std::shared_ptr<T>()), newItem, now, now);
                map.insert(canonicalPath, newCurValue);
            }
        }
        return std::make_pair(curValue, newCurValue);
    }

    // Inserts or updates an entry reflecting ExternalItem in the corresponding map
    // returns LoadResult reflecting the change made to the map
    template <typename T>
    LoadResult insertOrUpdateExternalItem(std::shared_ptr<T> extItem)
    {
        auto change = insertOrUpdateEntry<T>(std::move(extItem));
        DomItem univ(shared_from_this());
        return { univ.copy(change.first), univ.copy(change.second) };
    }

private:
    QString m_name;
    QMap<QString, std::shared_ptr<ExternalItemPair<GlobalScope>>> m_globalScopeWithName;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmlDirectory>>> m_qmlDirectoryWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmldirFile>>> m_qmldirFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmlFile>>> m_qmlFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<JsFile>>> m_jsFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmltypesFile>>> m_qmltypesFileWithPath;
};

class QMLDOM_EXPORT ExternalItemInfoBase: public OwningItem {
    Q_DECLARE_TR_FUNCTIONS(ExternalItemInfoBase);
public:
    constexpr static DomType kindValue = DomType::ExternalItemInfo;
    DomType kind() const final override { return kindValue; }
    ExternalItemInfoBase(
            const Path &canonicalPath,
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : OwningItem(derivedFrom, lastDataUpdateAt),
          m_canonicalPath(canonicalPath),
          m_currentExposedAt(currentExposedAt)
    {}
    ExternalItemInfoBase(const ExternalItemInfoBase &o) = default;

    virtual std::shared_ptr<ExternalOwningItem> currentItem() const = 0;
    virtual DomItem currentItem(const DomItem &) const = 0;

    QString canonicalFilePath(const DomItem &) const final override;
    Path canonicalPath() const { return m_canonicalPath; }
    Path canonicalPath(const DomItem &) const final override { return canonicalPath(); }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const final override;
    DomItem field(const DomItem &self, QStringView name) const final override
    {
        return OwningItem::field(self, name);
    }

    int currentRevision(const DomItem &self) const;
    int lastRevision(const DomItem &self) const;
    int lastValidRevision(const DomItem &self) const;

    std::shared_ptr<ExternalItemInfoBase> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemInfoBase>(doCopy(self));
    }

    QDateTime lastDataUpdateAt() const final override
    {
        if (currentItem())
            return currentItem()->lastDataUpdateAt();
        return OwningItem::lastDataUpdateAt();
    }

    void refreshedDataAt(QDateTime tNew) final override
    {
        if (currentItem())
            currentItem()->refreshedDataAt(tNew);
        return OwningItem::refreshedDataAt(tNew);
    }

    void ensureLogicalFilePath(const QString &path) {
        QMutexLocker l(mutex());
        if (!m_logicalFilePaths.contains(path))
            m_logicalFilePaths.append(path);
    }

    QDateTime currentExposedAt() const {
        QMutexLocker l(mutex()); // should not be needed, as it should not change...
        return m_currentExposedAt;
    }

    void setCurrentExposedAt(QDateTime d) {
        QMutexLocker l(mutex()); // should not be needed, as it should not change...
        m_currentExposedAt = d;
    }


    QStringList logicalFilePaths() const {
        QMutexLocker l(mutex());
        return m_logicalFilePaths;
    }

 private:
    friend class DomEnvironment;
    Path m_canonicalPath;
    QDateTime m_currentExposedAt;
    QStringList m_logicalFilePaths;
};

template<typename T>
class ExternalItemInfo final : public ExternalItemInfoBase
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<ExternalItemInfo>(*this);
    }

public:
    constexpr static DomType kindValue = DomType::ExternalItemInfo;
    ExternalItemInfo(
            const std::shared_ptr<T> &current = std::shared_ptr<T>(),
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : ExternalItemInfoBase(current->canonicalPath().dropTail(), currentExposedAt, derivedFrom,
                               lastDataUpdateAt),
          current(current)
    {}
    ExternalItemInfo(const QString &canonicalPath) : current(new T(canonicalPath)) { }
    ExternalItemInfo(const ExternalItemInfo &o):
        ExternalItemInfoBase(o), current(o.current)
    {
    }

    std::shared_ptr<ExternalItemInfo> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemInfo>(doCopy(self));
    }

    std::shared_ptr<ExternalOwningItem> currentItem() const override {
        return current;
    }
    DomItem currentItem(const DomItem &self) const override { return self.copy(current); }

    std::shared_ptr<T> current;
};

class Dependency
{ // internal, should be cleaned, but nobody should use this...
public:
    bool operator==(Dependency const &o) const
    {
        return uri == o.uri && version.majorVersion == o.version.majorVersion
                && version.minorVersion == o.version.minorVersion && filePath == o.filePath;
    }
    QString uri; // either dotted uri or file:, http: https: uri
    Version version;
    QString filePath; // for file deps
    DomType fileType;
};

class QMLDOM_EXPORT LoadInfo final : public OwningItem
{
    Q_DECLARE_TR_FUNCTIONS(LoadInfo);

protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

public:
    constexpr static DomType kindValue = DomType::LoadInfo;
    DomType kind() const override { return kindValue; }

    enum class Status {
        NotStarted, // dependencies non checked yet
        Starting, // adding deps
        InProgress, // waiting for all deps to be loaded
        CallingCallbacks, // calling callbacks
        Done // fully loaded
    };

    LoadInfo(const Path &elPath = Path(), Status status = Status::NotStarted, int nLoaded = 0,
             int derivedFrom = 0,
             const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : OwningItem(derivedFrom, lastDataUpdateAt),
          m_elementCanonicalPath(elPath),
          m_status(status),
          m_nLoaded(nLoaded)
    {
    }
    LoadInfo(const LoadInfo &o) : OwningItem(o), m_elementCanonicalPath(o.elementCanonicalPath())
    {
        {
            QMutexLocker l(o.mutex());
            m_status = o.m_status;
            m_nLoaded = o.m_nLoaded;
            m_toDo = o.m_toDo;
            m_inProgress = o.m_inProgress;
            m_endCallbacks = o.m_endCallbacks;
        }
    }

    Path canonicalPath(const DomItem &self) const override;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    std::shared_ptr<LoadInfo> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<LoadInfo>(doCopy(self));
    }
    void addError(const DomItem &self, ErrorMessage &&msg) override
    {
        self.path(elementCanonicalPath()).addError(std::move(msg));
    }

    void addEndCallback(const DomItem &self, std::function<void(Path, const DomItem &, const DomItem &)> callback);

    void advanceLoad(const DomItem &self);
    void finishedLoadingDep(const DomItem &self, const Dependency &d);
    void execEnd(const DomItem &self);

    Status status() const
    {
        QMutexLocker l(mutex());
        return m_status;
    }

    int nLoaded() const
    {
        QMutexLocker l(mutex());
        return m_nLoaded;
    }

    Path elementCanonicalPath() const
    {
        QMutexLocker l(mutex()); // we should never change this, remove lock?
        return m_elementCanonicalPath;
    }

    int nNotDone() const
    {
        QMutexLocker l(mutex());
        return m_toDo.size() + m_inProgress.size();
    }

    QList<Dependency> inProgress() const
    {
        QMutexLocker l(mutex());
        return m_inProgress;
    }

    QList<Dependency> toDo() const
    {
        QMutexLocker l(mutex());
        return m_toDo;
    }

    int nCallbacks() const
    {
        QMutexLocker l(mutex());
        return m_endCallbacks.size();
    }

private:
    void doAddDependencies(const DomItem &self);
    void addDependency(const DomItem &self, const Dependency &dep);

    Path m_elementCanonicalPath;
    Status m_status;
    int m_nLoaded;
    QQueue<Dependency> m_toDo;
    QList<Dependency> m_inProgress;
    QList<std::function<void(Path, const DomItem &, const DomItem &)>> m_endCallbacks;
};

enum class EnvLookup { Normal, NoBase, BaseOnly };

enum class Changeable { ReadOnly, Writable };

class QMLDOM_EXPORT RefCacheEntry
{
    Q_GADGET
public:
    enum class Cached { None, First, All };
    Q_ENUM(Cached)

    static RefCacheEntry forPath(const DomItem &el, const Path &canonicalPath);
    static bool addForPath(const DomItem &el, const Path &canonicalPath, const RefCacheEntry &entry,
                           AddOption addOption = AddOption::KeepExisting);

    Cached cached = Cached::None;
    QList<Path> canonicalPaths;
};

class QMLDOM_EXPORT DomEnvironment final : public DomTop,
                                           public std::enable_shared_from_this<DomEnvironment>
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(DomEnvironment);
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

private:
    struct TypeReader
    {
        TypeReader(const std::weak_ptr<DomEnvironment> &env, const QStringList &importPaths)
            : m_env(env), m_importPaths(importPaths)
        {
        }

        std::weak_ptr<DomEnvironment> m_env;
        QStringList m_importPaths;

        QList<QQmlJS::DiagnosticMessage>
        operator()(QQmlJSImporter *importer, const QString &filePath,
                   const QSharedPointer<QQmlJSScope> &scopeToPopulate);
    };

public:
    enum class Option {
        Default = 0x0,
        KeepValid = 0x1, // if there is a previous valid version, use that instead of the latest
        Exported = 0x2, // the current environment is accessible by multiple threads, one should only modify whole OwningItems, and in general load and do other operations in other (Child) environments
        NoReload = 0x4, // never reload something that was already loaded by the parent environment
        WeakLoad = 0x8, // load only the names of the available types, not the types (qml files) themselves
        SingleThreaded = 0x10, // do all operations in a single thread
        NoDependencies = 0x20 // will not load dependencies (useful when editing)
    };
    Q_ENUM(Option)
    Q_DECLARE_FLAGS(Options, Option);

    static ErrorGroups myErrors();
    constexpr static DomType kindValue = DomType::DomEnvironment;
    DomType kind() const override;

    Path canonicalPath() const override;
    using DomTop::canonicalPath;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    DomItem field(const DomItem &self, QStringView name) const final override;

    std::shared_ptr<DomEnvironment> makeCopy(const DomItem &self) const;

    void loadFile(const FileToLoad &file, const Callback &callback,
                  std::optional<DomType> fileType = std::optional<DomType>(),
                  const ErrorHandler &h = nullptr /* used only in loadPendingDependencies*/);
    void loadBuiltins(const Callback &callback = nullptr, const ErrorHandler &h = nullptr);
    void loadModuleDependency(const QString &uri, Version v, const Callback &callback = nullptr,
                              const ErrorHandler & = nullptr);

    void removePath(const QString &path);

    std::shared_ptr<DomUniverse> universe() const;

    QSet<QString> moduleIndexUris(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    QSet<int> moduleIndexMajorVersions(const DomItem &self, const QString &uri,
                                       EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ModuleIndex> moduleIndexWithUri(const DomItem &self, const QString &uri, int majorVersion,
                                                    EnvLookup lookup, Changeable changeable,
                                                    const ErrorHandler &errorHandler = nullptr);
    std::shared_ptr<ModuleIndex> moduleIndexWithUri(const DomItem &self, const QString &uri, int majorVersion,
                                                    EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmlDirectory>>
    qmlDirectoryWithPath(const DomItem &self, const QString &path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmlDirectoryPaths(const DomItem &self, EnvLookup options = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmldirFile>>
    qmldirFileWithPath(const DomItem &self, const QString &path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmldirFilePaths(const DomItem &self, EnvLookup options = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfoBase>
    qmlDirWithPath(const DomItem &self, const QString &path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmlDirPaths(const DomItem &self, EnvLookup options = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmlFile>>
    qmlFileWithPath(const DomItem &self, const QString &path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmlFilePaths(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<JsFile>>
    jsFileWithPath(const DomItem &self, const QString &path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> jsFilePaths(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmltypesFile>>
    qmltypesFileWithPath(const DomItem &self, const QString &path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmltypesFilePaths(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<GlobalScope>>
    globalScopeWithName(const DomItem &self, const QString &name, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<GlobalScope>>
    ensureGlobalScopeWithName(const DomItem &self, const QString &name, EnvLookup lookup = EnvLookup::Normal);
    QSet<QString> globalScopeNames(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;

    explicit DomEnvironment(const QStringList &loadPaths, Options options = Option::SingleThreaded,
                            DomCreationOption domCreationOption = Default,
                            const std::shared_ptr<DomUniverse> &universe = nullptr);
    explicit DomEnvironment(const std::shared_ptr<DomEnvironment> &parent,
                            const QStringList &loadPaths, Options options = Option::SingleThreaded,
                            DomCreationOption domCreationOption = Default);
    DomEnvironment(const DomEnvironment &o) = delete;
    static std::shared_ptr<DomEnvironment>
    create(const QStringList &loadPaths, Options options = Option::SingleThreaded,
           DomCreationOption creationOption = DomCreationOption::Default,
           const DomItem &universe = DomItem::empty);

    // TODO AddOption can easily be removed later. KeepExisting option only used in one
    // place which will be removed in https://codereview.qt-project.org/c/qt/qtdeclarative/+/523217
    void addQmlFile(const std::shared_ptr<QmlFile> &file,
                    AddOption option = AddOption::KeepExisting);
    void addQmlDirectory(const std::shared_ptr<QmlDirectory> &file,
                         AddOption option = AddOption::KeepExisting);
    void addQmldirFile(const std::shared_ptr<QmldirFile> &file,
                       AddOption option = AddOption::KeepExisting);
    void addQmltypesFile(const std::shared_ptr<QmltypesFile> &file,
                         AddOption option = AddOption::KeepExisting);
    void addJsFile(const std::shared_ptr<JsFile> &file, AddOption option = AddOption::KeepExisting);
    void addGlobalScope(const std::shared_ptr<GlobalScope> &file,
                        AddOption option = AddOption::KeepExisting);

    bool commitToBase(
            const DomItem &self, const std::shared_ptr<DomEnvironment> &validEnv = nullptr);

    void addDependenciesToLoad(const Path &path);
    void addLoadInfo(
            const DomItem &self, const std::shared_ptr<LoadInfo> &loadInfo);
    std::shared_ptr<LoadInfo> loadInfo(const Path &path) const;
    QList<Path> loadInfoPaths() const;
    QHash<Path, std::shared_ptr<LoadInfo>> loadInfos() const;
    void loadPendingDependencies();
    bool finishLoadingDependencies(int waitMSec = 30000);
    void addWorkForLoadInfo(const Path &elementCanonicalPath);

    Options options() const;

    std::shared_ptr<DomEnvironment> base() const;

    QStringList loadPaths() const;
    QStringList qmldirFiles() const;

    QString globalScopeName() const;

    static QList<Import> defaultImplicitImports();
    QList<Import> implicitImports() const;

    void addAllLoadedCallback(const DomItem &self, Callback c);

    void clearReferenceCache();
    void setLoadPaths(const QStringList &v);

    // Helper structure reflecting the change in the map once loading / fetching is completed
    // formerItem - DomItem representing value (ExternalItemInfo) existing in the map before the
    // loading && parsing. Might be empty (if didn't exist / failure) or equal to currentItem
    // currentItem - DomItem representing current map value
    struct LoadResult
    {
        DomItem formerItem;
        DomItem currentItem;
    };
    // TODO(QTBUG-121171)
    template <typename T>
    LoadResult insertOrUpdateExternalItemInfo(const QString &path, std::shared_ptr<T> extItem)
    {
        // maybe in the next revision this all can be just substituted by the addExternalItem
        DomItem env(shared_from_this());
        // try to fetch from the current env.
        if (auto curValue = lookup<T>(path, EnvLookup::NoBase)) {
            // found in the "initial" env
            return { env.copy(curValue), env.copy(curValue) };
        }
        std::shared_ptr<ExternalItemInfo<T>> newCurValue;
        // try to fetch from the base env
        auto valueInBase = lookup<T>(path, EnvLookup::BaseOnly);
        if (!valueInBase) {
            // Nothing found. Just create an externalItemInfo which will be inserted
            newCurValue = std::make_shared<ExternalItemInfo<T>>(std::move(extItem),
                                                                QDateTime::currentDateTimeUtc());
        } else {
            // prepare updated value as a copy of the value from the Base to be inserted
            newCurValue = valueInBase->makeCopy(env);
            if (newCurValue->current != extItem) {
                newCurValue->current = std::move(extItem);
                newCurValue->setCurrentExposedAt(QDateTime::currentDateTimeUtc());
            }
        }
        // Before inserting new or updated value, check one more time, if ItemInfo is already
        // present
        // lookup<> can't be used here because of the data-race
        {
            QMutexLocker l(mutex());
            auto &map = getMutableRefToMap<T>();
            const auto &it = map.find(path);
            if (it != map.end())
                return { env.copy(*it), env.copy(*it) };
            // otherwise insert
            map.insert(path, newCurValue);
        }
        return { env.copy(valueInBase), env.copy(newCurValue) };
    }

    template <typename T>
    void addExternalItemInfo(const DomItem &newExtItem, const Callback &loadCallback,
                             const Callback &endCallback)
    {
        // get either Valid "file" from the ExternalItemPair or the current (wip) "file"
        std::shared_ptr<T> newItemPtr;
        if (options() & DomEnvironment::Option::KeepValid)
            newItemPtr = newExtItem.field(Fields::validItem).ownerAs<T>();
        if (!newItemPtr)
            newItemPtr = newExtItem.field(Fields::currentItem).ownerAs<T>();
        Q_ASSERT(newItemPtr && "envCallbackForFile reached without current file");

        auto loadResult = insertOrUpdateExternalItemInfo(newExtItem.canonicalFilePath(),
                                                         std::move(newItemPtr));
        Path p = loadResult.currentItem.canonicalPath();
        {
            auto depLoad = qScopeGuard([p, this, endCallback] {
                addDependenciesToLoad(p);
                // add EndCallback to the queue, which should be called once all dependencies are
                // loaded
                if (endCallback) {
                    DomItem env = DomItem(shared_from_this());
                    addAllLoadedCallback(
                            env, [p, endCallback](Path, const DomItem &, const DomItem &env) {
                                DomItem el = env.path(p);
                                endCallback(p, el, el);
                            });
                }
            });
            // call loadCallback
            if (loadCallback) {
                loadCallback(p, loadResult.formerItem, loadResult.currentItem);
            }
        }
    }
    void populateFromQmlFile(MutableDomItem &&qmlFile);
    DomCreationOption domCreationOption() const { return m_domCreationOption; }

private:
    friend class RefCacheEntry;

    void loadFile(const FileToLoad &file, const Callback &loadCallback, const Callback &endCallback,
                  std::optional<DomType> fileType = std::optional<DomType>(),
                  const ErrorHandler &h = nullptr);

    void loadModuleDependency(const DomItem &self, const QString &uri, Version v,
                              Callback loadCallback = nullptr, Callback endCallback = nullptr,
                              const ErrorHandler & = nullptr);

    template <typename T>
    QSet<QString> getStrings(function_ref<QSet<QString>()> getBase, const QMap<QString, T> &selfMap,
                             EnvLookup lookup) const;

    template <typename T>
    const QMap<QString, std::shared_ptr<ExternalItemInfo<T>>> &getConstRefToMap() const
    {
        Q_ASSERT(!mutex()->tryLock());
        if constexpr (std::is_same_v<T, GlobalScope>) {
            return m_globalScopeWithName;
        }
        if constexpr (std::is_same_v<T, QmlDirectory>) {
            return m_qmlDirectoryWithPath;
        }
        if constexpr (std::is_same_v<T, QmldirFile>) {
            return m_qmldirFileWithPath;
        }
        if constexpr (std::is_same_v<T, QmlFile>) {
            return m_qmlFileWithPath;
        }
        if constexpr (std::is_same_v<T, JsFile>) {
            return m_jsFileWithPath;
        }
        if constexpr (std::is_same_v<T, QmltypesFile>) {
            return m_qmltypesFileWithPath;
        }
        Q_UNREACHABLE();
    }

    template <typename T>
    std::shared_ptr<ExternalItemInfo<T>> lookup(const QString &path, EnvLookup options) const
    {
        if (options != EnvLookup::BaseOnly) {
            QMutexLocker l(mutex());
            const auto &map = getConstRefToMap<T>();
            const auto &it = map.find(path);
            if (it != map.end())
                return *it;
        }
        if (options != EnvLookup::NoBase && m_base)
            return m_base->lookup<T>(path, options);
        return {};
    }

    template <typename T>
    QMap<QString, std::shared_ptr<ExternalItemInfo<T>>> &getMutableRefToMap()
    {
        Q_ASSERT(!mutex()->tryLock());
        if constexpr (std::is_same_v<T, QmlDirectory>) {
            return m_qmlDirectoryWithPath;
        }
        if constexpr (std::is_same_v<T, QmldirFile>) {
            return m_qmldirFileWithPath;
        }
        if constexpr (std::is_same_v<T, QmlFile>) {
            return m_qmlFileWithPath;
        }
        if constexpr (std::is_same_v<T, JsFile>) {
            return m_jsFileWithPath;
        }
        if constexpr (std::is_same_v<T, QmltypesFile>) {
            return m_qmltypesFileWithPath;
        }
        if constexpr (std::is_same_v<T, GlobalScope>) {
            return m_globalScopeWithName;
        }
        Q_UNREACHABLE();
    }

    template <typename T>
    void addExternalItem(std::shared_ptr<T> file, QString key, AddOption option)
    {
        if (!file)
            return;

        auto eInfo = std::make_shared<ExternalItemInfo<T>>(file, QDateTime::currentDateTimeUtc());
        // Lookup helper can't be used here, because it introduces data-race otherwise
        // (other modifications might happen between the lookup and the insert)
        QMutexLocker l(mutex());
        auto &map = getMutableRefToMap<T>();
        const auto &it = map.find(key);
        if (it != map.end() && option == AddOption::KeepExisting)
            return;
        map.insert(key, eInfo);
    }

    using FetchResult =
            std::pair<std::shared_ptr<ExternalItemInfoBase>, std::shared_ptr<ExternalItemInfoBase>>;
    // This function tries to get an Info object about the ExternalItem from the current env
    // and depending on the result and options tries to fetch it from the Parent env,
    // saving a copy with an updated timestamp
    template <typename T>
    FetchResult fetchFileFromEnvs(const FileToLoad &file)
    {
        const auto &path = file.canonicalPath();
        // lookup only in the current env
        if (auto value = lookup<T>(path, EnvLookup::NoBase)) {
            return std::make_pair(value, value);
        }
        // try to find the file in the base(parent) Env and insert if found
        if (options() & Option::NoReload) {
            if (auto baseV = lookup<T>(path, EnvLookup::BaseOnly)) {
                // Watch out! QTBUG-121171
                // It's possible between the lookup and creation of curVal, baseV && baseV->current
                // might have changed
                // Prepare a value to be inserted as copy of the value from Base
                auto curV = std::make_shared<ExternalItemInfo<T>>(
                        baseV->current, QDateTime::currentDateTimeUtc(), baseV->revision(),
                        baseV->lastDataUpdateAt());
                // Lookup one more time if the value was already inserted to the current env
                // Lookup can't be used here because of the data-race
                {
                    QMutexLocker l(mutex());
                    auto &map = getMutableRefToMap<T>();
                    const auto &it = map.find(path);
                    if (it != map.end())
                        return std::make_pair(*it, *it);
                    // otherwise insert
                    map.insert(path, curV);
                }
                return std::make_pair(baseV, curV);
            }
        }
        return std::make_pair(nullptr, nullptr);
    }

    Callback getLoadCallbackFor(DomType fileType, const Callback &loadCallback);

    std::shared_ptr<ModuleIndex> lookupModuleInEnv(const QString &uri, int majorVersion) const;
    // ModuleLookupResult contains the ModuleIndex pointer, and an indicator whether it was found
    // in m_base or in m_moduleIndexWithUri
    struct ModuleLookupResult {
        enum Origin :  bool {FromBase, FromGlobal};
        std::shared_ptr<ModuleIndex> module;
        Origin fromBase = FromGlobal;
    };
    // helper function used by the moduleIndexWithUri methods
    ModuleLookupResult moduleIndexWithUriHelper(const DomItem &self, const QString &uri, int majorVersion,
                                                    EnvLookup lookup = EnvLookup::Normal) const;

    const Options m_options;
    const std::shared_ptr<DomEnvironment> m_base;
    std::shared_ptr<DomEnvironment> m_lastValidBase;
    const std::shared_ptr<DomUniverse> m_universe;
    QStringList m_loadPaths; // paths for qml
    QString m_globalScopeName;
    QMap<QString, QMap<int, std::shared_ptr<ModuleIndex>>> m_moduleIndexWithUri;
    QMap<QString, std::shared_ptr<ExternalItemInfo<GlobalScope>>> m_globalScopeWithName;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmlDirectory>>> m_qmlDirectoryWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmldirFile>>> m_qmldirFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmlFile>>> m_qmlFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<JsFile>>> m_jsFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmltypesFile>>> m_qmltypesFileWithPath;
    QQueue<Path> m_loadsWithWork;
    QQueue<Path> m_inProgress;
    QHash<Path, std::shared_ptr<LoadInfo>> m_loadInfos;
    QList<Import> m_implicitImports;
    QList<Callback> m_allLoadedCallback;
    QHash<Path, RefCacheEntry> m_referenceCache;
    DomCreationOption m_domCreationOption;

    struct SemanticAnalysis
    {
        SemanticAnalysis(const QStringList &loadPaths);
        void updateLoadPaths(const QStringList &loadPaths);

        std::shared_ptr<QQmlJSResourceFileMapper> m_mapper;
        std::shared_ptr<QQmlJSImporter> m_importer;
    };
    std::optional<SemanticAnalysis> m_semanticAnalysis;
public:
    SemanticAnalysis semanticAnalysis();

private:
    SemanticAnalysis semanticAnalysisUnlocked();
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DomEnvironment::Options)

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // DOMTOP_H
