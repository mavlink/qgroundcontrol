#include "PX4SensorDevice.h"

#include <string_view>

#include <QtCore/QCoreApplication>

namespace {

struct SensorDeviceType
{
    quint8 type;
    std::string_view name;
};

// DroneCAN sensors have no entry, they show as the bus
constexpr SensorDeviceType kSensorDeviceTypes[] = {
    {0x01, "HMC5883"},   {0x03, "MAGSIM"},    {0x04, "AK8963"},    {0x05, "LIS3MDL"},    {0x06, "IST8310"},
    {0x07, "RM3100"},    {0x08, "QMC5883L"},  {0x09, "AK09916"},   {0x0A, "VCM1193L"},   {0x0B, "IST8308"},
    {0x0D, "MMC5983MA"}, {0x0E, "IIS2MDC"},   {0x0F, "QMC5883P"},  {0x10, "AF9838"},     {0x11, "LSM303D"},
    {0x12, "LSM6DSV"},   {0x14, "SIM"},       {0x15, "SIM"},       {0x21, "MPU6000"},    {0x22, "L3GD20"},
    {0x24, "MPU9250"},   {0x25, "ICM20649"},  {0x26, "ICM42688P"}, {0x27, "ICM40609D"},  {0x28, "ICM20948"},
    {0x29, "ICM42605"},  {0x2A, "ICM42670P"}, {0x2B, "IIM42652"},  {0x2C, "IAM20680HP"}, {0x2D, "ICM42686P"},
    {0x2E, "IIM42653"},  {0x33, "MPU6050"},   {0x34, "ICM45686"},  {0x35, "MPU6050"},    {0x36, "MPU6500"},
    {0x37, "BMI270"},    {0x38, "ICM20602"},  {0x3A, "ICM20608G"}, {0x3C, "ICM20689"},   {0x3D, "MS5611"},
    {0x3E, "MS5607"},    {0x3F, "BMP280"},    {0x40, "LPS25H"},    {0x41, "BMI055"},     {0x42, "BMI055"},
    {0x43, "BMM150"},    {0x44, "LSM9DS1"},   {0x45, "LSM9DS1"},   {0x46, "ETS3"},       {0x47, "MS4515"},
    {0x48, "MS4525DO"},  {0x49, "MS5525DSO"}, {0x4A, "SDP31"},     {0x4B, "SDP32"},      {0x4C, "SDP33"},
    {0x4D, "TCBP001TA"}, {0x4E, "MS5837"},    {0x4F, "SPL06"},     {0x50, "LPS33HW"},    {0x51, "MPL3115A2"},
    {0x52, "FXOS8701C"}, {0x54, "FXAS2100C"}, {0x57, "ADIS16448"}, {0x58, "ADIS16470"},  {0x59, "ADIS16477"},
    {0x5A, "ADIS16507"}, {0x5B, "SCH16T"},    {0x5C, "ADIS16607"}, {0x5D, "ADIS1650X"},  {0x5E, "ADIS1657X"},
    {0x5F, "MPC2520"},   {0x60, "LPS22HB"},   {0x61, "LSM303AGR"}, {0x62, "LSM303AGR"},  {0x63, "ADIS16497"},
    {0x65, "BAROSIM"},   {0x66, "BMI088"},    {0x67, "BMP388"},    {0x68, "DPS310"},     {0x6A, "BMI088"},
    {0x6C, "BMI085"},    {0x6D, "BMI085"},    {0x6E, "BMP390"},    {0x6F, "BMP581"},     {0xB7, "ICP101XX"},
    {0xB8, "ICP201XX"},  {0xE4, "ASP5033"},   {0xE5, "BMM350"},    {0xE6, "AUAV"},       {0xE7, "AUAV"},
    {0xE8, "SPA06"},     {0xF2, "AK09940A"},
};

// Layout of the firmware's DeviceStructure bitfield
constexpr quint32 kBusTypeMask = 0x7;
constexpr int kDeviceTypeShift = 16;
constexpr quint32 kDeviceTypeMask = 0xFF;

enum SensorBusType : quint32
{
    SensorBusUnknown = 0,
    SensorBusI2c = 1,
    SensorBusSpi = 2,
    SensorBusDroneCan = 3,
    SensorBusSimulation = 4,
    SensorBusSerial = 5,
    SensorBusMavlink = 6,
};

quint8 sensorDeviceType(quint32 deviceId)
{
    return static_cast<quint8>((deviceId >> kDeviceTypeShift) & kDeviceTypeMask);
}

}  // namespace

QString PX4SensorDevice::name(quint32 deviceId)
{
    const quint8 type = sensorDeviceType(deviceId);
    for (const SensorDeviceType& entry : kSensorDeviceTypes) {
        if (entry.type == type) {
            return QString::fromLatin1(entry.name.data(), static_cast<qsizetype>(entry.name.size()));
        }
    }
    return QString();
}

QString PX4SensorDevice::busName(quint32 deviceId)
{
    switch (deviceId & kBusTypeMask) {
        case SensorBusI2c:
            return QStringLiteral("I2C");
        case SensorBusSpi:
            return QStringLiteral("SPI");
        case SensorBusDroneCan:
            return QStringLiteral("DroneCAN");
        case SensorBusSimulation:
            return QCoreApplication::translate("PX4SensorDevice", "Simulation");
        case SensorBusSerial:
            return QStringLiteral("Serial");
        case SensorBusMavlink:
            return QStringLiteral("MAVLink");
        default:
            return QString();
    }
}

QString PX4SensorDevice::description(quint32 deviceId)
{
    if (deviceId == 0) {
        return QString();
    }

    const QString chip = name(deviceId);
    const QString bus = busName(deviceId);
    if (chip.isEmpty() && bus.isEmpty()) {
        return QStringLiteral("0x%1").arg(deviceId, 8, 16, QLatin1Char('0'));
    }
    if (chip.isEmpty()) {
        return bus;
    }
    return bus.isEmpty() ? chip : QStringLiteral("%1 · %2").arg(chip, bus);
}
