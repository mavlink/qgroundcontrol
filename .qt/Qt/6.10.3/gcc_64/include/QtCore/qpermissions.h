// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPERMISSIONS_H
#define QPERMISSIONS_H

#if 0
#pragma qt_class(QPermissions)
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qtmetamacros.h>
#include <QtCore/qvariant.h>

#include <QtCore/qshareddata_impl.h>
#include <QtCore/qtypeinfo.h>
#include <QtCore/qmetatype.h>

#include <optional>

#if !defined(Q_QDOC)
QT_REQUIRE_CONFIG(permissions);
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
#endif

struct QMetaObject;
class QCoreApplication;

class QPermission
{
    template <typename T, typename Enable = void>
    static constexpr inline bool is_permission_v = false;

    template <typename T>
    using if_permission = std::enable_if_t<is_permission_v<T>, bool>;
public:
    explicit QPermission() = default;

    template <typename T, if_permission<T> = true>
    QPermission(const T &t) : m_data(QVariant::fromValue(t)) {}

    Qt::PermissionStatus status() const { return m_status; }

    QMetaType type() const { return m_data.metaType(); }

    template <typename T, if_permission<T> = true>
    std::optional<T> value() const
    {
        if (auto p = data(QMetaType::fromType<T>()))
            return *static_cast<const T *>(p);
        return std::nullopt;
    }

#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug debug, const QPermission &);
#endif

private:
    Q_CORE_EXPORT const void *data(QMetaType id) const;

    Qt::PermissionStatus m_status = Qt::PermissionStatus::Undetermined;
    QVariant m_data;

    friend class QCoreApplication;
};

template <typename T>
constexpr bool QPermission::is_permission_v<T, typename T::QtPermissionHelper> = true;

#define QT_PERMISSION(ClassName) \
    using QtPermissionHelper = void; \
    friend class QPermission; \
    union U { \
        U() : d(nullptr) {} \
        U(ShortData _data) : data(_data) {} \
        U(ClassName##Private *_d) : d(_d) {} \
        ShortData data; \
        ClassName##Private *d; \
    } u; \
public: \
    Q_CORE_EXPORT ClassName(); \
    Q_CORE_EXPORT ClassName(const ClassName &other) noexcept; \
    ClassName(ClassName &&other) noexcept \
        : u{other.u} { other.u.d = nullptr; } \
    Q_CORE_EXPORT ~ClassName(); \
    Q_CORE_EXPORT ClassName &operator=(const ClassName &other) noexcept; \
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(ClassName) \
    void swap(ClassName &other) noexcept { std::swap(u, other.u); } \
private: \
    /*end*/

class QLocationPermissionPrivate;
class QLocationPermission
{
    Q_GADGET_EXPORT(Q_CORE_EXPORT)
public:
    enum Accuracy : quint8 {
        Approximate,
        Precise,
    };
    Q_ENUM(Accuracy)

    Q_CORE_EXPORT void setAccuracy(Accuracy accuracy);
    Q_CORE_EXPORT Accuracy accuracy() const;

    enum Availability : quint8 {
        WhenInUse,
        Always,
    };
    Q_ENUM(Availability)

    Q_CORE_EXPORT void setAvailability(Availability availability);
    Q_CORE_EXPORT Availability availability() const;

private:
    struct ShortData {
        Accuracy accuracy;
        Availability availability;
        char reserved[sizeof(void*) - sizeof(accuracy) - sizeof(availability)];
    };
    QT_PERMISSION(QLocationPermission)
};
Q_DECLARE_SHARED(QLocationPermission)

class QCalendarPermissionPrivate;
class QCalendarPermission
{
    Q_GADGET_EXPORT(Q_CORE_EXPORT)
public:
    enum AccessMode : quint8 {
        ReadOnly,
        ReadWrite,
    };
    Q_ENUM(AccessMode)

    Q_CORE_EXPORT void setAccessMode(AccessMode mode);
    Q_CORE_EXPORT AccessMode accessMode() const;

private:
    struct ShortData {
        AccessMode mode;
        char reserved[sizeof(void*) - sizeof(mode)];
    };
    QT_PERMISSION(QCalendarPermission)
};
Q_DECLARE_SHARED(QCalendarPermission)

class QContactsPermissionPrivate;
class QContactsPermission
{
    Q_GADGET_EXPORT(Q_CORE_EXPORT)
public:
    enum AccessMode : quint8 {
        ReadOnly,
        ReadWrite,
    };
    Q_ENUM(AccessMode)

    Q_CORE_EXPORT void setAccessMode(AccessMode mode);
    Q_CORE_EXPORT AccessMode accessMode() const;

private:
    struct ShortData {
        AccessMode mode;
        char reserved[sizeof(void*) - sizeof(mode)];
    };
    QT_PERMISSION(QContactsPermission)
};
Q_DECLARE_SHARED(QContactsPermission)

class QBluetoothPermissionPrivate;
class QBluetoothPermission
{
    Q_GADGET_EXPORT(Q_CORE_EXPORT)
public:
    enum CommunicationMode : quint8 {
        Access = 0x01,
        Advertise = 0x02,
        Default = Access | Advertise,
    };
    Q_DECLARE_FLAGS(CommunicationModes, CommunicationMode)
    Q_FLAG(CommunicationModes)

    Q_CORE_EXPORT void setCommunicationModes(CommunicationModes modes);
    Q_CORE_EXPORT CommunicationModes communicationModes() const;

private:
    struct ShortData {
        CommunicationMode mode;
        char reserved[sizeof(void*) - sizeof(mode)];
    };
    QT_PERMISSION(QBluetoothPermission)
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QBluetoothPermission::CommunicationModes)
Q_DECLARE_SHARED(QBluetoothPermission)

#define Q_DECLARE_MINIMAL_PERMISSION(ClassName) \
    class ClassName##Private; \
    class ClassName \
    { \
        struct ShortData { char reserved[sizeof(void*)]; }; \
        QT_PERMISSION(ClassName) \
    }; \
    Q_DECLARE_SHARED(ClassName)

Q_DECLARE_MINIMAL_PERMISSION(QCameraPermission)
Q_DECLARE_MINIMAL_PERMISSION(QMicrophonePermission)

#undef QT_PERMISSION
#undef Q_DECLARE_MINIMAL_PERMISSION

QT_END_NAMESPACE

#endif // QPERMISSIONS_H
