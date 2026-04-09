#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include <cstdint>

/// Reason a signing operation failed. Used by SigningController error path and Vehicle::signingFailed.
class SigningFailure
{
    Q_GADGET
    QML_VALUE_TYPE(signingFailure)

public:
    enum class Reason : uint8_t
    {
        Timeout,
        InitFailed,
        VehicleUnreachable,
    };
    Q_ENUM(Reason)

    Q_PROPERTY(SigningFailure::Reason reason MEMBER reason)
    Q_PROPERTY(QString detail MEMBER detail)

    Reason reason{};
    QString detail;

    bool operator==(const SigningFailure&) const = default;
};

Q_DECLARE_METATYPE(SigningFailure)
