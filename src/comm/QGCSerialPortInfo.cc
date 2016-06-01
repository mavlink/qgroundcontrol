/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCSerialPortInfo.h"

QGC_LOGGING_CATEGORY(QGCSerialPortInfoLog, "QGCSerialPortInfoLog")

static const struct VIDPIDMapInfo_s {
    int                             vendorId;
    int                             productId;
    QGCSerialPortInfo::BoardType_t  boardType;
    const char *                    boardString;
} s_rgVIDPIDMappings[] = {
    { QGCSerialPortInfo::px4VendorId,           QGCSerialPortInfo::pixhawkFMUV4ProductId,               QGCSerialPortInfo::BoardTypePX4FMUV4,   "Found PX4 FMU V4" },
    { QGCSerialPortInfo::px4VendorId,           QGCSerialPortInfo::pixhawkFMUV2ProductId,               QGCSerialPortInfo::BoardTypePX4FMUV2,   "Found PX4 FMU V2" },
    { QGCSerialPortInfo::px4VendorId,           QGCSerialPortInfo::pixhawkFMUV2OldBootloaderProductId,  QGCSerialPortInfo::BoardTypePX4FMUV2,   "Found PX4 FMU V2"},
    { QGCSerialPortInfo::px4VendorId,           QGCSerialPortInfo::pixhawkFMUV1ProductId,               QGCSerialPortInfo::BoardTypePX4FMUV1,   "Found PX4 FMU V1" },
    { QGCSerialPortInfo::px4VendorId,           QGCSerialPortInfo::px4FlowProductId,                    QGCSerialPortInfo::BoardTypePX4Flow,    "Found PX4 Flow" },
    { QGCSerialPortInfo::px4VendorId,           QGCSerialPortInfo::AeroCoreProductId,                   QGCSerialPortInfo::BoardTypeAeroCore,   "Found AeroCore" },
    { QGCSerialPortInfo::px4VendorId,           QGCSerialPortInfo::MindPXFMUV2ProductId,                QGCSerialPortInfo::BoardTypeMINDPXFMUV2,"Found MindPX FMU V2" },
    { QGCSerialPortInfo::threeDRRadioVendorId,  QGCSerialPortInfo::threeDRRadioProductId,               QGCSerialPortInfo::BoardTypeSikRadio,   "Found SiK Radio" },
    { QGCSerialPortInfo::siLabsRadioVendorId,   QGCSerialPortInfo::siLabsRadioProductId,                QGCSerialPortInfo::BoardTypeSikRadio,   "Found SiK Radio" },
    { QGCSerialPortInfo::ubloxRTKVendorId,      QGCSerialPortInfo::ubloxRTKProductId,                   QGCSerialPortInfo::BoardTypeRTKGPS,     "Found RTK GPS" },
};

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

    for (size_t i=0; i<sizeof(s_rgVIDPIDMappings)/sizeof(s_rgVIDPIDMappings[0]); i++) {
        const struct VIDPIDMapInfo_s* pIDMap = &s_rgVIDPIDMappings[i];

        if (vendorIdentifier() == pIDMap->vendorId && productIdentifier() == pIDMap->productId) {
            boardType = pIDMap->boardType;
            qCDebug(QGCSerialPortInfoLog) << pIDMap->boardString;
            break;
        }
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
        } else if (description() == "MindPX FMU v2.x" || description() == "MindPX BL FMU v2.x") {
            qCDebug(QGCSerialPortInfoLog) << "Found MindPX FMU V2 (by name matching fallback)";
            boardType = BoardTypeMINDPXFMUV2;
        } else if (description() == "FT231X USB UART") {
            qCDebug(QGCSerialPortInfoLog) << "Found possible Radio (by name matching fallback)";
            boardType = BoardTypeSikRadio;
#ifdef __android__
        } else if (description().endsWith("USB UART")) {
            // This is a fairly broad fallbacks for radios which will also catch most FTDI devices. That would
            // cause problems on desktop due to incorrect connections. Since mobile is more anal about connecting
            // it will work fine here.
            boardType = BoardTypeSikRadio;
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

    return boardType == BoardTypePX4FMUV1 || boardType == BoardTypePX4FMUV2
            || boardType == BoardTypePX4FMUV4 || boardType == BoardTypeAeroCore
            || boardType == BoardTypeMINDPXFMUV2;
}

bool QGCSerialPortInfo::isBootloader(void) const
{
    // FIXME: Check SerialLink bootloade detect code which is different
    return boardTypePixhawk() && description().contains("BL");
}

bool QGCSerialPortInfo::canFlash(void)
{
    BoardType_t boardType = this->boardType();

    return boardType != QGCSerialPortInfo::BoardTypeUnknown && boardType != QGCSerialPortInfo::BoardTypeRTKGPS;
}
