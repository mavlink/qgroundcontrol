/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


// Based on https://github.com/ArduPilot/MAVProxy/blob/9bf8b00fbc355650b060e546110877c7898baa81/MAVProxy/modules/lib/mp_util.py#L384

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts  1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCLabel {
    property Fact fact: Fact { }
    text: decode(fact)


    property var busTypes: {
        0: '-',
        1: 'I2C',
        2: 'SPI',
        3: 'UAVCAN',
        4: 'SITL',
        5: 'MSP',
        6: 'EAHRS'
    }

    property var compassTypes: {
        0x01: 'HMC5883_OLD',
        0x07: 'HMC5883',
        0x02: 'LSM303D',
        0x04: 'AK8963 ',
        0x05: 'BMM150 ',
        0x06: 'LSM9DS1',
        0x08: 'LIS3MDL',
        0x09: 'AK09916',
        0x0A: 'IST8310',
        0x0B: 'ICM20948',
        0x0C: 'MMC3416',
        0x0D: 'QMC5883L',
        0x0E: 'MAG3110',
        0x0F: 'SITL',
        0x10: 'IST8308',
        0x11: 'RM3100',
        0x12: 'RM3100_2',
        0x13: 'MMC5883',
        0x14: 'AK09918',
        0x15: 'AK09915',
    }

    property var imuTypes: {
        0x09: 'BMI160',
        0x10: 'L3G4200D',
        0x11: 'ACC_LSM303D',
        0x12: 'ACC_BMA180',
        0x13: 'ACC_MPU6000',
        0x16: 'ACC_MPU9250',
        0x17: 'ACC_IIS328DQ',
        0x18: 'ACC_LSM9DS1',
        0x21: 'GYR_MPU6000',
        0x22: 'GYR_L3GD20',
        0x24: 'GYR_MPU9250',
        0x25: 'GYR_I3G4250D',
        0x26: 'GYR_LSM9DS1',
        0x27: 'INS_ICM20789',
        0x28: 'INS_ICM20689',
        0x29: 'INS_BMI055',
        0x2A: 'SITL',
        0x2B: 'INS_BMI088',
        0x2C: 'INS_ICM20948',
        0x2D: 'INS_ICM20648',
        0x2E: 'INS_ICM20649',
        0x2F: 'INS_ICM20602',
        0x30: 'INS_ICM20601',
        0x31: 'INS_ADIS1647x',
        0x32: 'INS_SERIAL',
        0x33: 'INS_ICM40609',
        0x34: 'INS_ICM42688',
        0x35: 'INS_ICM42605'
    }

    property var baroTypes: {
        0x01: 'SITL',
        0x02: 'BMP085',
        0x03: 'BMP280',
        0x04: 'BMP388',
        0x05: 'DPS280',
        0x06: 'DPS310',
        0x07: 'FBM320',
        0x08: 'ICM20789',
        0x09: 'KELLERLD',
        0x0A: 'LPS2XH',
        0x0B: 'MS5611',
        0x0C: 'SPL06',
        0x0D: 'UAVCAN'
    }

    property var airspeedTypes: {
        0x01: 'SITL',
        0x02: 'MS4525',
        0x03: 'MS5525',
        0x04: 'DLVR',
        0x05: 'MSP',
        0x06: 'SDP3X',
        0x07: 'UAVCAN',
        0x08: 'ANALOG',
        0x09: 'NMEA',
        0x0A: 'ASP5033'
    }

    function decode (device) {
        var devid = parseInt(device.valueString)
        var deviceName = device.name
        var busType = busTypes[devid & 0x07]
        var bus = (devid >> 3) & 0x1F
        var address = (devid >> 8) & 0xFF
        var devtype = (devid >> 16)
        var decodedDevname;

        if (devid === 0) {
            return ""
        }
        if (deviceName.startsWith('COMPASS')) {
            if (busType === 3 && devtype === 1) {
                decodedDevname = 'UAVCAN'
            } else if (busType === 6 && devtype === 1) {
                decodedDevname = 'EAHRS'
            } else {
                decodedDevname = compassTypes[devtype] || '?'
            }
        }
        if (deviceName.startsWith('INS')) {
            decodedDevname = imuTypes[devtype] || '?'
        }
        if (deviceName.startsWith('GND_BARO')) {
            decodedDevname = baroTypes[devtype] || '?'
        }
        if (deviceName.startsWith('BARO')) {
            decodedDevname = baroTypes[devtype] || '?'
        }
        if (deviceName.startsWith('ARSP')) {
            decodedDevname = airspeedTypes[devtype] || '?'
        }
        return `${decodedDevname} (${busType}${bus})`
    }
}
