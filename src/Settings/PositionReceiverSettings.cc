#include "PositionReceiverSettings.h"

DECLARE_SETTINGGROUP(PositionReceiver, "PositionReceiver") {}

DECLARE_SETTINGSFACT(PositionReceiverSettings, enabled)
DECLARE_SETTINGSFACT(PositionReceiverSettings, connectionType)
DECLARE_SETTINGSFACT(PositionReceiverSettings, tcpHost)
DECLARE_SETTINGSFACT(PositionReceiverSettings, tcpPort)
DECLARE_SETTINGSFACT(PositionReceiverSettings, udpPort)
DECLARE_SETTINGSFACT(PositionReceiverSettings, serialDevice)
DECLARE_SETTINGSFACT(PositionReceiverSettings, serialBaudRate)
