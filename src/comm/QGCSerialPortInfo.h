/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef QGCSerialPortInfo_H
#define QGCSerialPortInfo_H

#ifdef __android__
    #include "qserialportinfo.h"
#else
    #include <QSerialPortInfo>
#endif

#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(QGCSerialPortInfoLog)

/// QGC's version of Qt QSerialPortInfo. It provides additional information about board types
/// that QGC cares about.
class QGCSerialPortInfo : public QSerialPortInfo
{
public:
    typedef enum {
        BoardTypePX4FMUV1,
        BoardTypePX4FMUV2,
        BoardTypePX4FMUV4,
        BoardTypePX4Flow,
        BoardTypeSikRadio,
        BoardTypeAeroCore,
        BoardTypeRTKGPS,
        BoardTypeUnknown
    } BoardType_t;

    // Vendor and products ids for the boards we care about

    static const int px4VendorId =                          9900;   ///< Vendor ID for Pixhawk board (V2 and V1) and PX4 Flow

    static const int pixhawkFMUV4ProductId =                18;     ///< Product ID for Pixhawk V2 board
    static const int pixhawkFMUV2ProductId =                17;     ///< Product ID for Pixhawk V2 board
    static const int pixhawkFMUV2OldBootloaderProductId =   22;     ///< Product ID for Bootloader on older Pixhawk V2 boards
    static const int pixhawkFMUV1ProductId =                16;     ///< Product ID for PX4 FMU V1 board

    static const int AeroCoreProductId =                    4097;   ///< Product ID for the AeroCore board
    
    static const int px4FlowProductId =                     21;     ///< Product ID for PX4 Flow board

    static const int threeDRRadioVendorId =                 1027;   ///< Vendor ID for 3DR Radio
    static const int threeDRRadioProductId =                24597;  ///< Product ID for 3DR Radio

    static const int siLabsRadioVendorId =                  0x10c4; ///< Vendor ID for SILabs Radio
    static const int siLabsRadioProductId =                 0xea60; ///< Product ID for SILabs Radio

    static const int ubloxRTKVendorId =                     5446;   ///< Vendor ID for ublox RTK
    static const int ubloxRTKProductId =                    424;    ///< Product ID for ublox RTK

    QGCSerialPortInfo(void);
    QGCSerialPortInfo(const QSerialPort & port);

    /// Override of QSerialPortInfo::availablePorts
    static QList<QGCSerialPortInfo> availablePorts(void);

    BoardType_t boardType(void) const;

    /// @return true: we can flash this board type
    bool canFlash(void);

    /// @return true: board is a Pixhawk board
    bool boardTypePixhawk(void) const;

    /// @return true: Board is currently in bootloader
    bool isBootloader(void) const;

private:
};

#endif
