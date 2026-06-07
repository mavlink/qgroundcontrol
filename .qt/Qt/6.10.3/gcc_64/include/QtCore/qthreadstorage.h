// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTHREADSTORAGE_H
#define QTHREADSTORAGE_H

#include <QtCore/qglobal.h>

#if !QT_CONFIG(thread)
#include <memory>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread)

template <bool ShouldWarn> struct QThreadStorageTraits
{
    static constexpr void warnAboutTrivial() {}
};
template <> struct QThreadStorageTraits<true>
{
#ifndef Q_NO_THREAD_STORAGE_TRIVIAL_WARNING
    Q_DECL_DEPRECATED_X("QThreadStorage used with a trivial non-pointer type; consider using thread_local")
#endif
    static constexpr void warnAboutTrivial() noexcept {}
};

class Q_CORE_EXPORT QThreadStorageData
{
public:
    explicit QThreadStorageData(void (*func)(void *));
    ~QThreadStorageData();

    void** get() const;
    void** set(void* p);

    int id;
};

// pointer specialization
template <typename T>
inline
T *&qThreadStorage_localData(QThreadStorageData &d, T **)
{
    void **v = d.get();
    if (!v) v = d.set(nullptr);
    return *(reinterpret_cast<T**>(v));
}

template <typename T>
inline
T *qThreadStorage_localData_const(const QThreadStorageData &d, T **)
{
    void **v = d.get();
    return v ? *(reinterpret_cast<T**>(v)) : 0;
}

template <typename T>
inline
void qThreadStorage_setLocalData(QThreadStorageData &d, T **t)
{ (void) d.set(*t); }

template <typename T>
inline
void qThreadStorage_deleteData(void *d, T **)
{ delete static_cast<T *>(d); }

// value-based specialization
template <typename T>
inline
T &qThreadStorage_localData(QThreadStorageData &d, T *)
{
    void **v = d.get();
    if (!v) v = d.set(new T());
    return *(reinterpret_cast<T*>(*v));
}

template <typename T>
inline
T qThreadStorage_localData_const(const QThreadStorageData &d, T *)
{
    void **v = d.get();
    return v ? *(reinterpret_cast<T*>(*v)) : T();
}

template <typename T>
inline
void qThreadStorage_setLocalData(QThreadStorageData &d, T *t)
{ (void) d.set(new T(*t)); }

template <typename T>
inline
void qThreadStorage_deleteData(void *d, T *)
{ delete static_cast<T *>(d); }

template <class T>
class QThreadStorage
{
private:
    using Trait = QThreadStorageTraits<std::is_trivially_default_constructible_v<T> &&
                                       std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>>;
    QThreadStorageData d;

    Q_DISABLE_COPY(QThreadStorage)

    static inline void deleteData(void *x)
    { qThreadStorage_deleteData(x, reinterpret_cast<T*>(0)); }

public:
    inline QThreadStorage() : d(deleteData) { Trait::warnAboutTrivial(); }
    inline ~QThreadStorage() { }

    inline bool hasLocalData() const
    { return d.get() != nullptr; }

    inline T& localData()
    { return qThreadStorage_localData(d, reinterpret_cast<T*>(0)); }
    inline T localData() const
    { return qThreadStorage_localData_const(d, reinterpret_cast<T*>(0)); }

    inline void setLocalData(T t)
    { qThreadStorage_setLocalData(d, &t); }
};

#else // !QT_CONFIG(thread)

template <typename T, typename U>
inline bool qThreadStorage_hasLocalData(const std::unique_ptr<T, U> &data)
{
    return !!data;
}

template <typename T, typename U>
inline bool qThreadStorage_hasLocalData(const std::unique_ptr<T*, U> &data)
{
    return !!data ? *data != nullptr : false;
}

template <typename T>
inline void qThreadStorage_deleteLocalData(T *t)
{
    delete t;
}

template <typename T>
inline void qThreadStorage_deleteLocalData(T **t)
{
    delete *t;
    delete t;
}

template <class T>
class QThreadStorage
{
private:
    struct ScopedPointerThreadStorageDeleter
    {
        void operator()(T *t) const noexcept
        {
            if (t == nullptr)
                return;
            qThreadStorage_deleteLocalData(t);
        }
    };
    std::unique_ptr<T, ScopedPointerThreadStorageDeleter> data;

public:
    QThreadStorage() = default;
    ~QThreadStorage() = default;
    QThreadStorage(const QThreadStorage &rhs) = delete;
    QThreadStorage &operator=(const QThreadStorage &rhs) = delete;

    inline bool hasLocalData() const
    {
        return qThreadStorage_hasLocalData(data);
    }

    inline T &localData()
    {
        if (!data)
            data.reset(new T());
        return *data;
    }

    inline T localData() const
    {
        return !!data ? *data : T();
    }

    inline void setLocalData(T t)
    {
        data.reset(new T(t));
    }
};

#endif // QT_CONFIG(thread)

QT_END_NAMESPACE

#endif // QTHREADSTORAGE_H
