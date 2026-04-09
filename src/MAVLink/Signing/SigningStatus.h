#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

struct SigningStatus
{
    Q_GADGET
    QML_VALUE_TYPE(signingStatus)
    Q_PROPERTY(SigningStatus::State state MEMBER state)
    Q_PROPERTY(bool enabled MEMBER enabled)
    Q_PROPERTY(bool pending READ pending)
    Q_PROPERTY(QString keyName MEMBER keyName)
    Q_PROPERTY(QString statusText MEMBER statusText)
    Q_PROPERTY(int streamCount MEMBER streamCount)

public:
    enum class State : uint8_t { Off, Enabling, On, Disabling };
    Q_ENUM(State)

    State state{State::Off};
    bool enabled{false};
    QString keyName;
    QString statusText;
    int streamCount{0};

    bool pending() const { return state == State::Enabling || state == State::Disabling; }

    bool operator==(const SigningStatus&) const = default;
};

Q_DECLARE_METATYPE(SigningStatus)
