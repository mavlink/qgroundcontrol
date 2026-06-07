// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTIPCCOMMON_H
#define QTIPCCOMMON_H

#include <QtCore/qglobal.h>
#include <QtCore/qtcore-config.h>

#if QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)
#  include <QtCore/qstring.h>
#  include <QtCore/qobjectdefs.h>

QT_BEGIN_NAMESPACE

class QNativeIpcKeyPrivate;
class QNativeIpcKey
{
    Q_GADGET_EXPORT(Q_CORE_EXPORT)
public:
    enum class Type : quint16 {
        // 0 is reserved for the invalid type
        // keep 1 through 0xff free, except for SystemV
        SystemV = 0x51,         // 'Q'

        PosixRealtime = 0x100,
        Windows,
    };
    Q_ENUM(Type)

    static constexpr Type DefaultTypeForOs =
#ifdef Q_OS_WIN
            Type::Windows
#else
            Type::PosixRealtime
#endif
            ;
    static Type legacyDefaultTypeForOs() noexcept;

    constexpr QNativeIpcKey() noexcept = default;

    explicit constexpr QNativeIpcKey(Type type) noexcept
        : typeAndFlags{type}
    {
    }

    Q_IMPLICIT QNativeIpcKey(const QString &k, Type type = DefaultTypeForOs)
        : key(k), typeAndFlags{type}
    {
    }

    QNativeIpcKey(const QNativeIpcKey &other)
        : d(other.d), key(other.key), typeAndFlags(other.typeAndFlags)
    {
        if (isSlowPath())
            copy_internal(other);
    }

    QNativeIpcKey(QNativeIpcKey &&other) noexcept
        : d(std::exchange(other.d, nullptr)), key(std::move(other.key)),
          typeAndFlags(std::move(other.typeAndFlags))
    {
        if (isSlowPath())
            move_internal(std::move(other));
    }

    ~QNativeIpcKey()
    {
        if (isSlowPath())
            destroy_internal();
    }

    QNativeIpcKey &operator=(const QNativeIpcKey &other)
    {
        typeAndFlags = other.typeAndFlags;
        key = other.key;
        if (isSlowPath() || other.isSlowPath())
            return assign_internal(other);
        Q_ASSERT(!d);
        return *this;
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QNativeIpcKey)
    void swap(QNativeIpcKey &other) noexcept
    {
        std::swap(d, other.d);
        key.swap(other.key);
        typeAndFlags.swap(other.typeAndFlags);
    }

    bool isEmpty() const noexcept
    {
        return key.isEmpty();
    }

    bool isValid() const noexcept
    {
        return type() != Type{};
    }

    constexpr Type type() const noexcept
    {
        return typeAndFlags.type;
    }

    constexpr void setType(Type type)
    {
        if (isSlowPath())
            return setType_internal(type);
        typeAndFlags.type = type;
    }

    QString nativeKey() const noexcept
    {
        return key;
    }
    void setNativeKey(const QString &newKey)
    {
        key = newKey;
        if (isSlowPath())
            setNativeKey_internal(newKey);
    }

    Q_CORE_EXPORT QString toString() const;
    Q_CORE_EXPORT static QNativeIpcKey fromString(const QString &string);

private:
    struct TypeAndFlags {
        Type type = DefaultTypeForOs;
        quint16 reserved1 = {};
        quint32 reserved2 = {};

        void swap(TypeAndFlags &other) noexcept
        {
            std::swap(type, other.type);
            std::swap(reserved1, other.reserved1);
            std::swap(reserved2, other.reserved2);
        }

        friend constexpr bool operator==(const TypeAndFlags &lhs, const TypeAndFlags &rhs) noexcept
        {
            return lhs.type == rhs.type &&
                    lhs.reserved1 == rhs.reserved1 &&
                    lhs.reserved2 == rhs.reserved2;
        }
    };

    QNativeIpcKeyPrivate *d = nullptr;
    QString key;
    TypeAndFlags typeAndFlags;

    friend class QNativeIpcKeyPrivate;
    constexpr bool isSlowPath() const noexcept
    { return Q_UNLIKELY(d); }

#ifdef Q_QDOC
    friend size_t qHash(const QNativeIpcKey &ipcKey, size_t seed = 0) noexcept { return 0; }
#else
    friend Q_CORE_EXPORT size_t qHash(const QNativeIpcKey &ipcKey, size_t seed) noexcept;
    friend size_t qHash(const QNativeIpcKey &ipcKey) noexcept
    { return qHash(ipcKey, 0); }
#endif

    Q_CORE_EXPORT void copy_internal(const QNativeIpcKey &other);
    Q_CORE_EXPORT void move_internal(QNativeIpcKey &&other) noexcept;
    Q_CORE_EXPORT QNativeIpcKey &assign_internal(const QNativeIpcKey &other);
    Q_CORE_EXPORT void destroy_internal() noexcept;
    Q_CORE_EXPORT void setType_internal(Type);
    Q_CORE_EXPORT void setNativeKey_internal(const QString &);
    Q_DECL_PURE_FUNCTION Q_CORE_EXPORT static int
    compare_internal(const QNativeIpcKey &lhs, const QNativeIpcKey &rhs) noexcept;

#ifdef Q_OS_DARWIN
    Q_DECL_CONST_FUNCTION Q_CORE_EXPORT static Type defaultTypeForOs_internal() noexcept;
#endif
    friend bool comparesEqual(const QNativeIpcKey &lhs, const QNativeIpcKey &rhs) noexcept
    {
        if (!(lhs.typeAndFlags == rhs.typeAndFlags))
            return false;
        if (lhs.key != rhs.key)
            return false;
        if (lhs.d == rhs.d)
            return true;
        return compare_internal(lhs, rhs) == 0;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QNativeIpcKey)
};

// not a shared type, exactly, but this works too
Q_DECLARE_SHARED(QNativeIpcKey)

inline auto QNativeIpcKey::legacyDefaultTypeForOs() noexcept -> Type
{
#if defined(Q_OS_WIN)
    return Type::Windows;
#elif defined(QT_POSIX_IPC)
    return Type::PosixRealtime;
#elif defined(Q_OS_DARWIN)
    return defaultTypeForOs_internal();
#else
    return Type::SystemV;
#endif
}

QT_END_NAMESPACE

#endif // QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)


#endif // QTIPCCOMMON_H
