// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYCACHE_P_H
#define QQMLPROPERTYCACHE_P_H

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

#include <private/qlinkedstringhash_p.h>
#include <private/qqmlenumdata_p.h>
#include <private/qqmlenumvalue_p.h>
#include <private/qqmlpropertydata_p.h>
#include <private/qqmlrefcount_p.h>

#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>
#include <QtCore/qversionnumber.h>

#include <limits>

QT_BEGIN_NAMESPACE

class QCryptographicHash;
class QJSEngine;
class QMetaObjectBuilder;
class QQmlContextData;
class QQmlPropertyCache;
class QQmlPropertyCacheMethodArguments;
class QQmlVMEMetaObject;

class QQmlMetaObjectPointer
{
public:
    Q_NODISCARD_CTOR QQmlMetaObjectPointer() = default;

    Q_NODISCARD_CTOR QQmlMetaObjectPointer(const QMetaObject *staticMetaObject)
        : d(quintptr(staticMetaObject))
    {
        Q_ASSERT((d.loadRelaxed() & Shared) == 0);
    }

    ~QQmlMetaObjectPointer()
    {
        const auto dd = d.loadAcquire();
        if (dd & Shared)
            reinterpret_cast<SharedHolder *>(dd ^ Shared)->release();
    }

private:
    friend class QQmlPropertyCache;
    Q_NODISCARD_CTOR QQmlMetaObjectPointer(const QQmlMetaObjectPointer &other)
        : d(other.d.loadRelaxed())
    {
        // other has to survive until this ctor is done. So d cannot disappear before.
        const auto od = other.d.loadRelaxed();
        if (od & Shared)
            reinterpret_cast<SharedHolder *>(od ^ Shared)->addref();
    }

    QQmlMetaObjectPointer(QQmlMetaObjectPointer &&other) = delete;
    QQmlMetaObjectPointer &operator=(QQmlMetaObjectPointer &&other) = delete;
    QQmlMetaObjectPointer &operator=(const QQmlMetaObjectPointer &other) = delete;

public:
    void setSharedOnce(QMetaObject *shared) const
    {
        SharedHolder *holder = new SharedHolder(shared);
        if (!d.testAndSetRelease(0, quintptr(holder) | Shared))
            holder->release();
    }

    const QMetaObject *metaObject() const
    {
        const auto dd = d.loadAcquire();
        if (dd & Shared)
            return reinterpret_cast<SharedHolder *>(dd ^ Shared)->metaObject;
        return reinterpret_cast<const QMetaObject *>(dd);
    }

    bool isShared() const
    {
        // This works because static metaobjects need to be set in the ctor and once a shared
        // metaobject has been set, it cannot be removed anymore.
        const auto dd = d.loadRelaxed();
        return !dd || (dd & Shared);
    }

    bool isNull() const
    {
        return d.loadRelaxed() == 0;
    }

private:
    enum Tag {
        Static = 0,
        Shared = 1
    };

    struct SharedHolder final : public QQmlRefCounted<SharedHolder>
    {
        Q_DISABLE_COPY_MOVE(SharedHolder)
        SharedHolder(QMetaObject *shared) : metaObject(shared) {}
        ~SharedHolder() { free(metaObject); }
        QMetaObject *metaObject;
    };

    mutable QBasicAtomicInteger<quintptr> d = 0;
};

class Q_QML_EXPORT QQmlPropertyCache final
    : public QQmlRefCounted<QQmlPropertyCache>
{
public:
    using Ptr = QQmlRefPointer<QQmlPropertyCache>;

    struct ConstPtr : public QQmlRefPointer<const QQmlPropertyCache>
    {
        using QQmlRefPointer<const QQmlPropertyCache>::QQmlRefPointer;

        ConstPtr(const Ptr &ptr) : ConstPtr(ptr.data(), AddRef) {}
        ConstPtr(Ptr &&ptr) : ConstPtr(ptr.take(), Adopt) {}
        ConstPtr &operator=(const Ptr &ptr) { return operator=(ConstPtr(ptr)); }
        ConstPtr &operator=(Ptr &&ptr) { return operator=(ConstPtr(std::move(ptr))); }
    };

    static Ptr createStandalone(
            const QMetaObject *, QTypeRevision metaObjectRevision = QTypeRevision::zero());

    QQmlPropertyCache() = default;
    ~QQmlPropertyCache();

    void update(const QMetaObject *);
    void invalidate(const QMetaObject *);

    QQmlPropertyCache::Ptr copy() const;

    QQmlPropertyCache::Ptr copyAndAppend(
                const QMetaObject *, QTypeRevision typeVersion,
                QQmlPropertyData::Flags propertyFlags = QQmlPropertyData::Flags(),
                QQmlPropertyData::Flags methodFlags = QQmlPropertyData::Flags(),
                QQmlPropertyData::Flags signalFlags = QQmlPropertyData::Flags()) const;

    QQmlPropertyCache::Ptr copyAndReserve(
            int propertyCount, int methodCount, int signalCount, int enumCount) const;
    void appendProperty(const QString &, QQmlPropertyData::Flags flags, int coreIndex,
                        QMetaType propType, QTypeRevision revision, int notifyIndex);
    void appendAlias(const QString &, QQmlPropertyData::Flags flags, int coreIndex,
                     QMetaType propType, QTypeRevision version, int notifyIndex,
                     int encodedTargetIndex);
    void appendSignal(const QString &, QQmlPropertyData::Flags, int coreIndex,
                      const QMetaType *types = nullptr,
                      const QList<QByteArray> &names = QList<QByteArray>());
    void appendMethod(const QString &, QQmlPropertyData::Flags flags, int coreIndex,
                      QMetaType returnType, const QList<QByteArray> &names,
                      const QVector<QMetaType> &parameterTypes);
    void appendEnum(const QString &, const QVector<QQmlEnumValue> &);

    const QMetaObject *metaObject() const;
    const QMetaObject *createMetaObject() const;
    const QMetaObject *firstCppMetaObject() const;

    template<typename K>
    const QQmlPropertyData *property(const K &key, QObject *object,
                               const QQmlRefPointer<QQmlContextData> &context) const
    {
        return findProperty(stringCache.find(key), object, context);
    }

    const QQmlPropertyData *property(int) const;
    const QQmlPropertyData *maybeUnresolvedProperty(int) const;
    const QQmlPropertyData *method(int) const;
    const QQmlPropertyData *signal(int index) const;
    QQmlEnumData *qmlEnum(int) const;
    int methodIndexToSignalIndex(int) const;

    QString defaultPropertyName() const;
    const QQmlPropertyData *defaultProperty() const;

    // Return a reference here so that we don't have to addref/release all the time
    inline const QQmlPropertyCache::ConstPtr &parent() const;

    // is used by the Qml Designer
    void setParent(QQmlPropertyCache::ConstPtr newParent);

    inline const QQmlPropertyData *overrideData(const QQmlPropertyData *) const;
    inline bool isAllowedInRevision(const QQmlPropertyData *) const;

    static const QQmlPropertyData *property(
            QObject *, QStringView, const QQmlRefPointer<QQmlContextData> &,
            QQmlPropertyData *);
    static const QQmlPropertyData *property(QObject *, const QLatin1String &, const QQmlRefPointer<QQmlContextData> &,
            QQmlPropertyData *);
    static const QQmlPropertyData *property(QObject *, const QV4::String *, const QQmlRefPointer<QQmlContextData> &,
            QQmlPropertyData *);

    //see QMetaObjectPrivate::originalClone
    int originalClone(int index) const;
    static int originalClone(const QObject *, int index);

    QList<QByteArray> signalParameterNames(int index) const;
    static QString signalParameterStringForJS(
            const QList<QByteArray> &parameterNameList, QString *errorString = nullptr);

    const char *className() const;

    inline int propertyCount() const;
    inline int ownPropertyCount() const { return int(propertyIndexCache.count()); }
    inline int propertyOffset() const;
    inline int methodCount() const;
    inline int ownMethodCount() const { return int(methodIndexCache.count()); }
    inline int methodOffset() const;
    inline int signalCount() const;
    inline int ownSignalCount() const { return int(signalHandlerIndexCache.count()); }
    inline int signalOffset() const;
    inline int qmlEnumCount() const;

    void toMetaObjectBuilder(QMetaObjectBuilder &) const;

    inline bool callJSFactoryMethod(QObject *object, void **args) const;

    static bool determineMetaObjectSizes(const QMetaObject &mo, int *fieldCount, int *stringCount);
    static bool addToHash(QCryptographicHash &hash, const QMetaObject &mo);

    QByteArray checksum(QHash<quintptr, QByteArray> *checksums, bool *ok) const;

    QTypeRevision allowedRevision(int index) const { return allowedRevisionCache[index]; }
    void setAllowedRevision(int index, QTypeRevision allowed) { allowedRevisionCache[index] = allowed; }

private:
    friend class QQmlEnginePrivate;
    friend class QQmlCompiler;
    template <typename T> friend class QQmlPropertyCacheCreator;
    template <typename T> friend class QQmlPropertyCacheAliasCreator;
    template <typename T> friend class QQmlComponentAndAliasResolver;
    friend class QQmlMetaObject;

    QQmlPropertyCache(const QQmlMetaObjectPointer &metaObject) : _metaObject(metaObject) {}

    inline QQmlPropertyCache::Ptr copy(const QQmlMetaObjectPointer &mo, int reserve) const;

    void append(const QMetaObject *, QTypeRevision typeVersion,
                QQmlPropertyData::Flags propertyFlags = QQmlPropertyData::Flags(),
                QQmlPropertyData::Flags methodFlags = QQmlPropertyData::Flags(),
                QQmlPropertyData::Flags signalFlags = QQmlPropertyData::Flags());

    QQmlPropertyCacheMethodArguments *createArgumentsObject(int count, const QList<QByteArray> &names);

    typedef QVector<QQmlPropertyData> IndexCache;
    typedef QLinkedStringMultiHash<std::pair<int, QQmlPropertyData *> > StringCache;
    typedef QVector<QTypeRevision> AllowedRevisionCache;

    const QQmlPropertyData *findProperty(StringCache::ConstIterator it, QObject *,
                                   const QQmlRefPointer<QQmlContextData> &) const;
    const QQmlPropertyData *findProperty(StringCache::ConstIterator it, const QQmlVMEMetaObject *,
                                   const QQmlRefPointer<QQmlContextData> &) const;

    template<typename K>
    QQmlPropertyData *findNamedProperty(const K &key) const
    {
        StringCache::mapped_type *it = stringCache.value(key);
        return it ? it->second : 0;
    }

    template<typename K>
    void setNamedProperty(const K &key, int index, QQmlPropertyData *data)
    {
        stringCache.insert(key, std::make_pair(index, data));
    }

private:
    enum OverrideResult { NoOverride, InvalidOverride, ValidOverride };

    template<typename String>
    OverrideResult handleOverride(const String &name, QQmlPropertyData *data, QQmlPropertyData *old)
    {
        if (!old)
            return NoOverride;

        if (data->markAsOverrideOf(old))
            return ValidOverride;

        qWarning("Final member %s is overridden in class %s. The override won't be used.",
                 qPrintable(name), className());
        return InvalidOverride;
    }

    template<typename String>
    OverrideResult handleOverride(const String &name, QQmlPropertyData *data)
    {
        return handleOverride(name, data, findNamedProperty(name));
    }

    void doAppendPropertyData(const QString &name, QQmlPropertyData &&data)
    {
        QQmlPropertyData *old = findNamedProperty(name);
        const OverrideResult overrideResult = handleOverride(name, &data, old);
        if (overrideResult == InvalidOverride) {
            // Insert the overridden member once more, to keep the counts in sync
            propertyIndexCache.append(*old);
            return;
        }

        const int index = propertyIndexCache.size();
        propertyIndexCache.append(std::move(data));

        setNamedProperty(name, index + propertyOffset(), propertyIndexCache.data() + index);
    }

    int propertyIndexCacheStart = 0; // placed here to avoid gap between QQmlRefCount and _parent
    QQmlPropertyCache::ConstPtr _parent;

    IndexCache propertyIndexCache;
    IndexCache methodIndexCache;
    IndexCache signalHandlerIndexCache;
    StringCache stringCache;
    AllowedRevisionCache allowedRevisionCache;
    QVector<QQmlEnumData> enumCache;

    QQmlMetaObjectPointer _metaObject;
    QByteArray _dynamicClassName;
    QByteArray _dynamicStringData;
    QByteArray _listPropertyAssignBehavior;
    QString _defaultPropertyName;
    QQmlPropertyCacheMethodArguments *argumentsCache = nullptr;
    int methodIndexCacheStart = 0;
    int signalHandlerIndexCacheStart = 0;
    int _jsFactoryMethodIndex = -1;
};

// Returns this property cache's metaObject.  May be null if it hasn't been created yet.
inline const QMetaObject *QQmlPropertyCache::metaObject() const
{
    return _metaObject.metaObject();
}

// Returns the first C++ type's QMetaObject - that is, the first QMetaObject not created by
// QML
inline const QMetaObject *QQmlPropertyCache::firstCppMetaObject() const
{
    const QQmlPropertyCache *p = this;
    while (p->_metaObject.isShared())
        p = p->parent().data();
    return p->_metaObject.metaObject();
}

inline const QQmlPropertyData *QQmlPropertyCache::property(int index) const
{
    if (index < 0 || index >= propertyCount())
        return nullptr;

    if (index < propertyIndexCacheStart)
        return _parent->property(index);

    return &propertyIndexCache.at(index - propertyIndexCacheStart);
}

inline const QQmlPropertyData *QQmlPropertyCache::method(int index) const
{
    if (index < 0 || index >= (methodIndexCacheStart + methodIndexCache.size()))
        return nullptr;

    if (index < methodIndexCacheStart)
        return _parent->method(index);

    return const_cast<const QQmlPropertyData *>(&methodIndexCache.at(index - methodIndexCacheStart));
}

/*! \internal
    \a index MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
inline const QQmlPropertyData *QQmlPropertyCache::signal(int index) const
{
    if (index < 0 || index >= (signalHandlerIndexCacheStart + signalHandlerIndexCache.size()))
        return nullptr;

    if (index < signalHandlerIndexCacheStart)
        return _parent->signal(index);

    const QQmlPropertyData *rv = const_cast<const QQmlPropertyData *>(&methodIndexCache.at(index - signalHandlerIndexCacheStart));
    Q_ASSERT(rv->isSignal() || rv->coreIndex() == -1);
    return rv;
}

inline QQmlEnumData *QQmlPropertyCache::qmlEnum(int index) const
{
    if (index < 0 || index >= enumCache.size())
        return nullptr;

    return const_cast<QQmlEnumData *>(&enumCache.at(index));
}

inline int QQmlPropertyCache::methodIndexToSignalIndex(int index) const
{
    if (index < 0 || index >= (methodIndexCacheStart + methodIndexCache.size()))
        return index;

    if (index < methodIndexCacheStart)
        return _parent->methodIndexToSignalIndex(index);

    return index - methodIndexCacheStart + signalHandlerIndexCacheStart;
}

// Returns the name of the default property for this cache
inline QString QQmlPropertyCache::defaultPropertyName() const
{
    return _defaultPropertyName;
}

inline const QQmlPropertyCache::ConstPtr &QQmlPropertyCache::parent() const
{
    return _parent;
}

const QQmlPropertyData *
QQmlPropertyCache::overrideData(const QQmlPropertyData *data) const
{
    if (!data->hasOverride())
        return nullptr;

    if (data->overrideIndexIsProperty())
        return property(data->overrideIndex());
    else
        return method(data->overrideIndex());
}

bool QQmlPropertyCache::isAllowedInRevision(const QQmlPropertyData *data) const
{
    const QTypeRevision requested = data->revision();
    const int offset = data->metaObjectOffset();
    if (offset == -1 && requested == QTypeRevision::zero())
        return true;

    Q_ASSERT(offset >= 0);
    Q_ASSERT(offset < allowedRevisionCache.size());
    const QTypeRevision allowed = allowedRevisionCache[offset];

    if (requested.hasMajorVersion()) {
        if (requested.majorVersion() > allowed.majorVersion())
            return false;
        if (requested.majorVersion() < allowed.majorVersion())
            return true;
    }

    return !requested.hasMinorVersion() || requested.minorVersion() <= allowed.minorVersion();
}

int QQmlPropertyCache::propertyCount() const
{
    return propertyIndexCacheStart + int(propertyIndexCache.size());
}

int QQmlPropertyCache::propertyOffset() const
{
    return propertyIndexCacheStart;
}

int QQmlPropertyCache::methodCount() const
{
    return methodIndexCacheStart + int(methodIndexCache.size());
}

int QQmlPropertyCache::methodOffset() const
{
    return methodIndexCacheStart;
}

int QQmlPropertyCache::signalCount() const
{
    return signalHandlerIndexCacheStart + int(signalHandlerIndexCache.size());
}

int QQmlPropertyCache::signalOffset() const
{
    return signalHandlerIndexCacheStart;
}

int QQmlPropertyCache::qmlEnumCount() const
{
    return int(enumCache.size());
}

bool QQmlPropertyCache::callJSFactoryMethod(QObject *object, void **args) const
{
    if (_jsFactoryMethodIndex != -1) {
        if (const QMetaObject *mo = _metaObject.metaObject()) {
            mo->d.static_metacall(object, QMetaObject::InvokeMetaMethod,
                                  _jsFactoryMethodIndex, args);
            return true;
        }
        return false;
    }
    if (_parent)
        return _parent->callJSFactoryMethod(object, args);
    return false;
}

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHE_P_H
