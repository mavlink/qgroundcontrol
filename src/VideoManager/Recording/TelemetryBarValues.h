#pragma once

#include <optional>

#include <QtCore/QList>
#include <QtCore/QString>

/// The values the Fly View's telemetry bar shows, read from the active vehicle: what the video's telemetry subtitles
/// and overlay show.
class TelemetryBarValues
{
public:
    struct Value
    {
        QString name;   ///< The Fact's short description
        QString value;  ///< With its units
    };

    /// (Re)reads the bar's layout, defaults included: read() doesn't follow later changes to it. Without a layout,
    /// read() has no values.
    void readLayout();

    /// The active vehicle's value of each of the bar's Facts it has, in the bar's order; nullopt without an active
    /// vehicle.
    std::optional<QList<Value>> read() const;

private:
    struct Field
    {
        QString factGroupName;
        QString factName;
    };

    QList<Field> _fields;
};
