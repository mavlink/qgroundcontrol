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

#include "QGCSerialPortInfo.h"

QGC_LOGGING_CATEGORY(QGCSerialPortInfoLog, "QGCSerialPortInfoLog")

QGCSerialPortInfo::QGCSerialPortInfo(void) :
    QSerialPortInfo()
{

}

QGCSerialPortInfo::QGCSerialPortInfo(const QSerialPort & port) :
    QSerialPortInfo(port)
{

}

QGCSerialPortInfo::BoardType_t QGCSerialPortInfo::boardType(void) const
{
    if (isNull()) {
        return BoardTypeUnknown;
    }

    BoardType_t boardType = BoardTypeUnknown;

    switch (vendorIdentifier()) {
        case px4VendorId:
            if (productIdentifier() == pixhawkFMUV4ProductId) {
                qCDebug(QGCSerialPortInfoLog) << "Found PX4 FMU V4";
                boardType = BoardTypePX4FMUV4;
            } else if (productIdentifier() == pixhawkFMUV2ProductId || productIdentifier() == pixhawkFMUV2OldBootloaderProductId) {
                qCDebug(QGCSerialPortInfoLog) << "Found PX4 FMU V2";
                boardType = BoardTypePX4FMUV2;
            } else if (productIdentifier() == pixhawkFMUV1ProductId) {
                qCDebug(QGCSerialPortInfoLog) << "Found PX4 FMU V1";
                boardType = BoardTypePX4FMUV1;
            } else if (productIdentifier() == px4FlowProductId) {
                qCDebug(QGCSerialPortInfoLog) << "Found PX4 Flow";
                boardType = BoardTypePX4Flow;
            } else if (productIdentifier() == AeroCoreProductId) {
                qCDebug(QGCSerialPortInfoLog) << "Found AeroCore";
                boardType = BoardTypeAeroCore;
            }
            break;
        case threeDRRadioVendorId:
            if (productIdentifier() == threeDRRadioProductId) {
                qCDebug(QGCSerialPortInfoLog) << "Found 3DR Radio";
                boardType = BoardType3drRadio;
            }
            break;
    }

    if (boardType == BoardTypeUnknown) {
        // Fall back to port name matching which could lead to incorrect board mapping. But in some cases the
        // vendor and product id do not come through correctly so this is used as a last chance detection method.
        if (description() == "PX4 FMU v4.x" || description() == "PX4 BL FMU v4.x") {
            qCDebug(QGCSerialPortInfoLog) << "Found PX4 FMU V4 (by name matching fallback)";
            boardType = BoardTypePX4FMUV4;
        } else if (description() == "PX4 FMU v2.x" || description() == "PX4 BL FMU v2.x") {
            qCDebug(QGCSerialPortInfoLog) << "Found PX4 FMU V2 (by name matching fallback)";
            boardType = BoardTypePX4FMUV2;
        } else if (description() == "PX4 FMU v1.x" || description() == "PX4 BL FMU v1.x") {
            qCDebug(QGCSerialPortInfoLog) << "Found PX4 FMU V1 (by name matching fallback)";
            boardType = BoardTypePX4FMUV1;
        } else if (description().startsWith("PX4 FMU")) {
            qCDebug(QGCSerialPortInfoLog) << "Found PX4 FMU, assuming V2 (by name matching fallback)";
            boardType = BoardTypePX4FMUV2;
        } else if (description().contains(QRegExp("PX4.*Flow", Qt::CaseInsensitive))) {
            qCDebug(QGCSerialPortInfoLog) << "Found possible px4 flow camera (by name matching fallback)";
            boardType = BoardTypePX4Flow;
        } else if (description() == "FT231X USB UART") {
            qCDebug(QGCSerialPortInfoLog) << "Found possible Radio (by name matching fallback)";
            boardType = BoardType3drRadio;
#ifdef __android__
        } else if (description().endsWith("USB UART")) {
            // This is a fairly broad fallbacks for radios which will also catch most FTDI devices. That would
            // cause problems on desktop due to incorrect connections. Since mobile is more anal about connecting
            // it will work fine here.
            boardType = BoardType3drRadio;
#endif
        }
    }

    return boardType;
}

QList<QGCSerialPortInfo> QGCSerialPortInfo::availablePorts(void)
{
    QList<QGCSerialPortInfo>    list;

    foreach(QSerialPortInfo portInfo, QSerialPortInfo::availablePorts()) {
        list << *((QGCSerialPortInfo*)&portInfo);
    }

    return list;
}

bool QGCSerialPortInfo::boardTypePixhawk(void) const
{
    BoardType_t boardType = this->boardType();

    return boardType == BoardTypePX4FMUV1 || boardType == BoardTypePX4FMUV2 || boardType == BoardTypePX4FMUV4 || boardType == BoardTypeAeroCore;
}

bool QGCSerialPortInfo::isBootloader(void) const
{
    // FIXME: Check SerialLink bootloade detect code which is different
    return boardTypePixhawk() && description().contains("BL");
}
