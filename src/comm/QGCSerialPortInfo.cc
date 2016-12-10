/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCSerialPortInfo.h"
#include "JsonHelper.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(QGCSerialPortInfoLog, "QGCSerialPortInfoLog")

bool         QGCSerialPortInfo::_jsonLoaded =           false;
const char*  QGCSerialPortInfo::_jsonFileTypeValue =    "USBBoardInfo";
const char*  QGCSerialPortInfo::_jsonBoardInfoKey =     "boardInfo";
const char*  QGCSerialPortInfo::_jsonBoardFallbackKey = "boardFallback";
const char*  QGCSerialPortInfo::_jsonVendorIDKey =      "vendorID";
const char*  QGCSerialPortInfo::_jsonProductIDKey =     "productID";
const char*  QGCSerialPortInfo::_jsonBoardClassKey =    "boardClass";
const char*  QGCSerialPortInfo::_jsonNameKey =          "name";
const char*  QGCSerialPortInfo::_jsonRegExpKey =        "regExp";
const char*  QGCSerialPortInfo::_jsonAndroidOnlyKey =   "androidOnly";

const QGCSerialPortInfo::BoardClassString2BoardType_t QGCSerialPortInfo::_rgBoardClass2BoardType[] = {
    { "Pixhawk",    QGCSerialPortInfo::BoardTypePixhawk },
    { "PX4 Flow",   QGCSerialPortInfo::BoardTypePX4Flow },
    { "RTK GPS",    QGCSerialPortInfo::BoardTypeRTKGPS },
    { "SiK Radio",  QGCSerialPortInfo::BoardTypeSiKRadio },
    { "OpenPilot",  QGCSerialPortInfo::BoardTypeOpenPilot },
};

QList<QGCSerialPortInfo::BoardInfo_t>       QGCSerialPortInfo::_boardInfoList;
QList<QGCSerialPortInfo::BoardFallback_t>   QGCSerialPortInfo::_boardFallbackList;

QGCSerialPortInfo::QGCSerialPortInfo(void) :
    QSerialPortInfo()
{

}

QGCSerialPortInfo::QGCSerialPortInfo(const QSerialPort & port) :
    QSerialPortInfo(port)
{

}

void QGCSerialPortInfo::_loadJsonData(void)
{
    if (_jsonLoaded) {
        return;
    }
    _jsonLoaded = true;

    QFile file(QStringLiteral(":/json/USBBoardInfo.json"));

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open board info json:" << file.errorString();
        return;
    }

    QByteArray  bytes = file.readAll();

    QJsonParseError jsonParseError;
    QJsonDocument   jsonDoc(QJsonDocument::fromJson(bytes, &jsonParseError));
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() << "Unable to parse board info json:" << jsonParseError.errorString();
        return;
    }
    QJsonObject json = jsonDoc.object();

    int fileVersion;
    QString errorString;
    if (!JsonHelper::validateQGCJsonFile(json,
                                         _jsonFileTypeValue,    // expected file type
                                         1,                     // minimum supported version
                                         1,                     // maximum supported version
                                         fileVersion,
                                         errorString)) {
        qWarning() << errorString;
        return;
    }

    // Validate root object keys
    QList<JsonHelper::KeyValidateInfo> rootKeyInfoList = {
        { _jsonBoardInfoKey,        QJsonValue::Array, true },
        { _jsonBoardFallbackKey,    QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(json, rootKeyInfoList, errorString)) {
        qWarning() << errorString;
        return;
    }

    // Load board info used to detect known board from vendor/product id

    QList<JsonHelper::KeyValidateInfo> boardKeyInfoList = {
        { _jsonVendorIDKey,     QJsonValue::Double, true },
        { _jsonProductIDKey,    QJsonValue::Double, true },
        { _jsonBoardClassKey,   QJsonValue::String, true },
        { _jsonNameKey,         QJsonValue::String, true },
    };

    QJsonArray rgBoardInfo = json[_jsonBoardInfoKey].toArray();
    for (int i=0; i<rgBoardInfo.count(); i++) {
        const QJsonValue& jsonValue = rgBoardInfo[i];
        if (!jsonValue.isObject()) {
            qWarning() << "Entry in boardInfo array is not object";
            return;
        }

        QJsonObject  boardObject = jsonValue.toObject();
        if (!JsonHelper::validateKeys(boardObject, boardKeyInfoList, errorString)) {
            qWarning() << errorString;
            return;
        }

        BoardInfo_t boardInfo;
        boardInfo.vendorId = boardObject[_jsonVendorIDKey].toInt();
        boardInfo.productId = boardObject[_jsonProductIDKey].toInt();
        boardInfo.name = boardObject[_jsonNameKey].toString();
        boardInfo.boardType = _boardClassStringToType(boardObject[_jsonBoardClassKey].toString());

        if (boardInfo.boardType == BoardTypeUnknown) {
            qWarning() << "Bad board class" << boardObject[_jsonBoardClassKey].toString();
            return;
        }

        _boardInfoList.append(boardInfo);
    }

    // Load board fallback info used to detect known boards from description string match

    QList<JsonHelper::KeyValidateInfo> fallbackKeyInfoList = {
        { _jsonRegExpKey,       QJsonValue::String, true },
        { _jsonBoardClassKey,   QJsonValue::String, true },
        { _jsonAndroidOnlyKey,  QJsonValue::Bool,   false },
    };

    QJsonArray rgBoardFallback = json[_jsonBoardFallbackKey].toArray();
    for (int i=0; i<rgBoardFallback.count(); i++) {
        const QJsonValue& jsonValue = rgBoardFallback[i];
        if (!jsonValue.isObject()) {
            qWarning() << "Entry in boardFallback array is not object";
            return;
        }

        QJsonObject  fallbackObject = jsonValue.toObject();
        if (!JsonHelper::validateKeys(fallbackObject, fallbackKeyInfoList, errorString)) {
            qWarning() << errorString;
            return;
        }

        BoardFallback_t boardFallback;
        boardFallback.regExp = fallbackObject[_jsonRegExpKey].toString();
        boardFallback.androidOnly = fallbackObject[_jsonAndroidOnlyKey].toBool(false);
        boardFallback.boardType = _boardClassStringToType(fallbackObject[_jsonBoardClassKey].toString());

        if (boardFallback.boardType == BoardTypeUnknown) {
            qWarning() << "Bad board class" << fallbackObject[_jsonBoardClassKey].toString();
            return;
        }

        _boardFallbackList.append(boardFallback);
    }
}

QGCSerialPortInfo::BoardType_t QGCSerialPortInfo::_boardClassStringToType(const QString& boardClass)
{
    for (size_t j=0; j<sizeof(_rgBoardClass2BoardType)/sizeof(_rgBoardClass2BoardType[0]); j++) {
        if (boardClass == _rgBoardClass2BoardType[j].classString) {
            return _rgBoardClass2BoardType[j].boardType;
            break;
        }
    }

    return BoardTypeUnknown;
}

bool QGCSerialPortInfo::getBoardInfo(QGCSerialPortInfo::BoardType_t& boardType, QString& name) const
{
    _loadJsonData();

    if (isNull()) {
        return false;
    }

    for (int i=0; i<_boardInfoList.count(); i++) {
        const BoardInfo_t& boardInfo = _boardInfoList[i];

        if (vendorIdentifier() == boardInfo.vendorId && productIdentifier() == boardInfo.productId) {
            boardType = boardInfo.boardType;
            name = boardInfo.name;
            return true;
        }
    }

    if (boardType == BoardTypeUnknown) {
        // Fall back to port name matching which could lead to incorrect board mapping. But in some cases the
        // vendor and product id do not come through correctly so this is used as a last chance detection method.
<<<<<<< c4da69536e067addfbf394609e9369c1c2d00129

        for (int i=0; i<_boardFallbackList.count(); i++) {
            const BoardFallback_t& boardFallback = _boardFallbackList[i];

            if (description().contains(QRegExp(boardFallback.regExp, Qt::CaseInsensitive))) {
#ifndef __android
                if (boardFallback.androidOnly) {
                    continue;
                }
=======
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
        } else if (description() == "PX4 TAP v1.x" || description() == "PX4 BL TAP v1.x") {
            qCDebug(QGCSerialPortInfoLog) << "Found TAP V1 (by name matching fallback)";
            boardType = BoardTypeTAPV1;
        } else if (description() == "PX4 ASC v1.x" || description() == "PX4 BL ASC v1.x") {
            qCDebug(QGCSerialPortInfoLog) << "Found ASC V1 (by name matching fallback)";
            boardType = BoardTypeASCV1;
        } else if (description() == "PX4 Crazyflie v2.0" || description() == "Crazyflie BL") {
            qCDebug(QGCSerialPortInfoLog) << "Found Crazyflie 2.0 (by name matching fallback)";
            boardType = BoardTypeCrazyflie2;
        } else if (description() == "FT231X USB UART") {
            qCDebug(QGCSerialPortInfoLog) << "Found possible Radio (by name matching fallback)";
            boardType = BoardTypeSikRadio;
#ifdef __android__
        } else if (description().endsWith("USB UART")) {
            // This is a fairly broad fallbacks for radios which will also catch most FTDI devices. That would
            // cause problems on desktop due to incorrect connections. Since mobile is more anal about connecting
            // it will work fine here.
            boardType = BoardTypeSikRadio;
>>>>>>> Add firmware upgrade support for CF2
#endif
                boardType = boardFallback.boardType;
                name = _boardTypeToString(boardType);
                return true;
            }
        }
    }

    boardType = BoardTypeUnknown;
    return false;
}

QString QGCSerialPortInfo::_boardTypeToString(BoardType_t boardType)
{
    QString unknown = QObject::tr("Unknown");

    switch (boardType) {
    case BoardTypePixhawk:
        return QObject::tr("Pixhawk");
    case BoardTypeSiKRadio:
        return QObject::tr("SiK Radio");
    case BoardTypePX4Flow:
        return QObject::tr("PX4 Flow");
    case BoardTypeOpenPilot:
        return QObject::tr("OpenPilot");
    case BoardTypeRTKGPS:
        return QObject::tr("RTK GPS");
    case BoardTypeUnknown:
        return unknown;
    }

    return unknown;
}


QList<QGCSerialPortInfo> QGCSerialPortInfo::availablePorts(void)
{
    QList<QGCSerialPortInfo>    list;

    foreach(QSerialPortInfo portInfo, QSerialPortInfo::availablePorts()) {
        list << *((QGCSerialPortInfo*)&portInfo);
    }

    return list;
}

<<<<<<< c4da69536e067addfbf394609e9369c1c2d00129
=======
bool QGCSerialPortInfo::boardTypePixhawk(void) const
{
    BoardType_t boardType = this->boardType();

    return boardType == BoardTypePX4FMUV1 || boardType == BoardTypePX4FMUV2
            || boardType == BoardTypePX4FMUV4 || boardType == BoardTypeAeroCore
            || boardType == BoardTypeMINDPXFMUV2 || boardType == BoardTypeTAPV1
            || boardType == BoardTypeASCV1 || boardType == BoardTypeCrazyflie2;
}

>>>>>>> Add firmware upgrade support for CF2
bool QGCSerialPortInfo::isBootloader(void) const
{
    BoardType_t boardType;
    QString     name;

    if (getBoardInfo(boardType, name)) {
        // FIXME: Check SerialLink bootloade detect code which is different
        return boardType == BoardTypePixhawk && description().contains("BL");
    } else {
        return false;
    }
}

bool QGCSerialPortInfo::canFlash(void)
{
    BoardType_t boardType;
    QString     name;

    if (getBoardInfo(boardType, name)) {
        switch(boardType){
        case QGCSerialPortInfo::BoardTypePixhawk:
        case QGCSerialPortInfo::BoardTypePX4Flow:
        case QGCSerialPortInfo::BoardTypeSiKRadio:
            return true;
        default:
            return false;
        }
    } else {
        return false;
    }
}
