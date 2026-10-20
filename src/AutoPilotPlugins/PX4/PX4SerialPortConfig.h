#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>

/// PX4 assigns serial ports per function: each function has a parameter (MAV_0_CONFIG, GPS_1_CONFIG, ...) whose
/// value is the port it runs on. These helpers invert that into a per-port view, working only from parameter
/// metadata and values so they can be tested without a vehicle.
namespace PX4SerialPortConfig {

inline constexpr int kDisabledPortId = 0;
inline constexpr int kEthernetPortId = 1000;

struct Port
{
    int id = kDisabledPortId;
    QString label;
};

struct Function
{
    QString paramName;
    QString label;
    QList<Port> ports;  ///< Ports the function can run on, excluding Disabled
    int currentPortId = kDisabledPortId;
    /// Port the function still runs on when its parameter is Disabled, else kDisabledPortId. Only RC Input Driver
    /// has one: PX4 always starts it, on the board's default RC port when its parameter is Disabled.
    int defaultPortId = kDisabledPortId;
};

struct ParamWrite
{
    QString paramName;
    int value = 0;
};

/// true: the enum values are a PX4 serial port selection, which makes the parameter a port function
bool isFunctionParam(const QVariantList& enumValues);

/// Baud rate parameter for a port, empty if the port has none (Ethernet, unknown ports)
QString baudParamName(int portId);

/// Display label from a PX4 "Serial Configuration for <label>" short description
QString functionLabel(const QString& paramName, const QString& shortDescription);

/// value is the function parameter's value. currentPortId is defaultPortId when it is Disabled.
Function makeFunction(const QString& paramName, const QString& shortDescription, const QVariantList& enumValues,
                      const QStringList& enumStrings, int value);

/// Every port any function can run on, in port id order
QList<Port> ports(const QList<Function>& functions);

/// true: several functions can run on the port at once. Only Ethernet: PX4 stops a second function from opening a
/// serial port, but network functions each use their own UDP port.
bool isSharedPort(int portId);

/// Parameter names of the functions assigned to a port. More than one on a port which isn't shared is a conflict:
/// PX4 starts only one of them.
QStringList functionsOnPort(const QList<Function>& functions, int portId);

/// The functions allowed on a port, with the port's usual functions (GPS on a GPS port, MAVLink on a TELEM port, ...)
/// first. Order is otherwise kept.
QList<Function> functionsForPort(const QList<Function>& functions, int portId);

/// Writes which put functionParamName on portId. On a port which isn't shared it becomes the only function there, and
/// an empty functionParamName clears the port. A function is never cleared from its default port, since Disabled
/// would leave it running there. Empty if nothing changes or the function can't run on the port.
QList<ParamWrite> assignPort(const QList<Function>& functions, int portId, const QString& functionParamName);

/// Writes which take functionParamName off portId. Empty if it isn't on the port, or the port is its default port.
QList<ParamWrite> removeFromPort(const QList<Function>& functions, int portId, const QString& functionParamName);

/// Extra settings shown for a function when it runs on portId
QStringList optionParamNames(const QString& functionParamName, int portId);

}  // namespace PX4SerialPortConfig
