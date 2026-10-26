#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

namespace VehicleSummaryState {
Q_NAMESPACE
QML_ELEMENT

enum State
{
    Normal,
    Attention,  ///< Setup is needed or the value is not usable as it is
    Muted,      ///< Off, disabled or not set, which is fine
};
Q_ENUM_NS(State)
}  // namespace VehicleSummaryState

/// One caption and value shown on a component's card of the Summary page. Items with the same group are shown
/// together in a box titled with it; items without a group are shown ungrouped.
class VehicleSummaryItem
{
    Q_GADGET
    QML_VALUE_TYPE(vehicleSummaryItem)
    Q_PROPERTY(QString caption MEMBER caption)
    Q_PROPERTY(QString value MEMBER value)
    Q_PROPERTY(VehicleSummaryState::State state MEMBER state)
    Q_PROPERTY(QString group MEMBER group)

public:
    using State = VehicleSummaryState::State;
    using enum VehicleSummaryState::State;

    QString caption;
    QString value;
    State state = Normal;
    QString group;

    VehicleSummaryItem() = default;
    VehicleSummaryItem(const QString& itemCaption, const QString& itemValue, State itemState = Normal,
                       const QString& itemGroup = {})
        : caption(itemCaption), value(itemValue), state(itemState), group(itemGroup)
    {
    }

    bool operator==(const VehicleSummaryItem&) const = default;
};
