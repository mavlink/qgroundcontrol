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

        for (int i=0; i<_boardFallbackList.count(); i++) {
            const BoardFallback_t& boardFallback = _boardFallbackList[i];

            if (description().contains(QRegExp(boardFallback.regExp, Qt::CaseInsensitive))) {
#ifndef __android
                if (boardFallback.androidOnly) {
                    continue;
                }
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
