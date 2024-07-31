/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCSerialPortInfo.h"

#include "JsonHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

QGC_LOGGING_CATEGORY(QGCSerialPortInfoLog, "qgc.comms.qgcserialportinfo")

bool QGCSerialPortInfo::_jsonLoaded = false;
QList<QGCSerialPortInfo::BoardInfo_t> QGCSerialPortInfo::_boardInfoList;
QList<QGCSerialPortInfo::BoardRegExpFallback_t> QGCSerialPortInfo::_boardDescriptionFallbackList;
QList<QGCSerialPortInfo::BoardRegExpFallback_t> QGCSerialPortInfo::_boardManufacturerFallbackList;

QGCSerialPortInfo::QGCSerialPortInfo()
    : QSerialPortInfo()
{
    // qCDebug(QGCSerialPortInfoLog) << Q_FUNC_INFO << this;
}

QGCSerialPortInfo::QGCSerialPortInfo(const QSerialPort &port)
    : QSerialPortInfo(port)
{
    // qCDebug(QGCSerialPortInfoLog) << Q_FUNC_INFO << this;
}

QGCSerialPortInfo::~QGCSerialPortInfo()
{
    // qCDebug(QGCSerialPortInfoLog) << Q_FUNC_INFO << this;
}

void QGCSerialPortInfo::_loadJsonData()
{
    if (_jsonLoaded) {
        return;
    }

    _jsonLoaded = true;

    QFile file(QStringLiteral(":/json/USBBoardInfo.json"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(QGCSerialPortInfoLog) << "Unable to open board info json:" << file.errorString();
        return;
    }

    const QByteArray data = file.readAll();
    file.close();
    QJsonParseError jsonParseError;
    const QJsonDocument jsonDoc(QJsonDocument::fromJson(data, &jsonParseError));
    if (jsonParseError.error != QJsonParseError::NoError) {
        qCWarning(QGCSerialPortInfoLog) << "Unable to parse board info json:" << jsonParseError.errorString();
        return;
    }

    const QJsonObject json = jsonDoc.object();
    int fileVersion;
    QString errorString;
    if (!JsonHelper::validateInternalQGCJsonFile(json, _jsonFileTypeValue, 1, 1, fileVersion, errorString)) {
        qCWarning(QGCSerialPortInfoLog) << errorString;
        return;
    }

    static const QList<JsonHelper::KeyValidateInfo> rootKeyInfoList = {
        { _jsonBoardInfoKey, QJsonValue::Array, true },
        { _jsonBoardDescriptionFallbackKey, QJsonValue::Array, true },
        { _jsonBoardManufacturerFallbackKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(json, rootKeyInfoList, errorString)) {
        qCWarning(QGCSerialPortInfoLog) << errorString;
        return;
    }

    static const QList<JsonHelper::KeyValidateInfo> boardKeyInfoList = {
        { _jsonVendorIDKey, QJsonValue::Double, true },
        { _jsonProductIDKey, QJsonValue::Double, true },
        { _jsonBoardClassKey, QJsonValue::String, true },
        { _jsonNameKey, QJsonValue::String, true },
    };
    const QJsonArray rgBoardInfo = json[_jsonBoardInfoKey].toArray();
    for (const QJsonValue &jsonValue : rgBoardInfo) {
        if (!jsonValue.isObject()) {
            qCWarning(QGCSerialPortInfoLog) << "Entry in boardInfo array is not object";
            return;
        }

        const QJsonObject boardObject = jsonValue.toObject();
        if (!JsonHelper::validateKeys(boardObject, boardKeyInfoList, errorString)) {
            qCWarning(QGCSerialPortInfoLog) << errorString;
            return;
        }

        const BoardInfo_t boardInfo = {
            boardObject[_jsonVendorIDKey].toInt(),
            boardObject[_jsonProductIDKey].toInt(),
            _boardClassStringToType(boardObject[_jsonBoardClassKey].toString()),
            boardObject[_jsonNameKey].toString()
        };
        if (boardInfo.boardType == BoardTypeUnknown) {
            qCWarning(QGCSerialPortInfoLog) << "Bad board class" << boardObject[_jsonBoardClassKey].toString();
            return;
        }
        (void) _boardInfoList.append(boardInfo);
    }

    static const QList<JsonHelper::KeyValidateInfo> fallbackKeyInfoList = {
        { _jsonRegExpKey, QJsonValue::String, true },
        { _jsonBoardClassKey, QJsonValue::String, true },
        { _jsonAndroidOnlyKey, QJsonValue::Bool, false },
    };
    const QJsonArray rgBoardDescriptionFallback = json[_jsonBoardDescriptionFallbackKey].toArray();
    for (const QJsonValue &jsonValue : rgBoardDescriptionFallback) {
        if (!jsonValue.isObject()) {
            qCWarning(QGCSerialPortInfoLog) << "Entry in boardFallback array is not object";
            return;
        }

        const QJsonObject fallbackObject = jsonValue.toObject();
        if (!JsonHelper::validateKeys(fallbackObject, fallbackKeyInfoList, errorString)) {
            qCWarning(QGCSerialPortInfoLog) << errorString;
            return;
        }

        const BoardRegExpFallback_t boardFallback = {
            fallbackObject[_jsonRegExpKey].toString(),
            _boardClassStringToType(fallbackObject[_jsonBoardClassKey].toString()),
            fallbackObject[_jsonAndroidOnlyKey].toBool(false)
        };
        if (boardFallback.boardType == BoardTypeUnknown) {
            qCWarning(QGCSerialPortInfoLog) << "Bad board class" << fallbackObject[_jsonBoardClassKey].toString();
            return;
        }
        (void) _boardDescriptionFallbackList.append(boardFallback);
    }

    const QJsonArray rgBoardManufacturerFallback = json[_jsonBoardManufacturerFallbackKey].toArray();
    for (const QJsonValue &jsonValue : rgBoardManufacturerFallback) {
        if (!jsonValue.isObject()) {
            qCWarning(QGCSerialPortInfoLog) << "Entry in boardFallback array is not object";
            return;
        }

        const QJsonObject fallbackObject = jsonValue.toObject();
        if (!JsonHelper::validateKeys(fallbackObject, fallbackKeyInfoList, errorString)) {
            qCWarning(QGCSerialPortInfoLog) << errorString;
            return;
        }

        const BoardRegExpFallback_t boardFallback = {
            fallbackObject[_jsonRegExpKey].toString(),
            _boardClassStringToType(fallbackObject[_jsonBoardClassKey].toString()),
            fallbackObject[_jsonAndroidOnlyKey].toBool(false)
        };
        if (boardFallback.boardType == BoardTypeUnknown) {
            qCWarning(QGCSerialPortInfoLog) << "Bad board class" << fallbackObject[_jsonBoardClassKey].toString();
            return;
        }
        (void) _boardManufacturerFallbackList.append(boardFallback);
    }
}

QGCSerialPortInfo::BoardType_t QGCSerialPortInfo::_boardClassStringToType(const QString &boardClass)
{
    for (const BoardClassString2BoardType_t &board : _rgBoardClass2BoardType) {
        if (boardClass == board.classString) {
            return board.boardType;
        }
    }

    return BoardTypeUnknown;
}

bool QGCSerialPortInfo::getBoardInfo(QGCSerialPortInfo::BoardType_t &boardType, QString &name) const
{
    boardType = BoardTypeUnknown;

    _loadJsonData();

    if (isNull()) {
        return false;
    }

    for (const BoardInfo_t &boardInfo : _boardInfoList) {
        if ((vendorIdentifier() == boardInfo.vendorId) && ((productIdentifier() == boardInfo.productId) || (boardInfo.productId == 0))) {
            boardType = boardInfo.boardType;
            name = boardInfo.name;
            return true;
        }
    }

    Q_ASSERT(boardType == BoardTypeUnknown);

    for (const BoardRegExpFallback_t &boardFallback : _boardDescriptionFallbackList) {
        if (description().contains(QRegularExpression(boardFallback.regExp, QRegularExpression::CaseInsensitiveOption))) {
#ifndef Q_OS_ANDROID
            if (boardFallback.androidOnly) {
                continue;
            }
#endif
            boardType = boardFallback.boardType;
            name = _boardTypeToString(boardType);
            return true;
        }
    }

    for (const BoardRegExpFallback_t &boardFallback : _boardManufacturerFallbackList) {
        if (manufacturer().contains(QRegularExpression(boardFallback.regExp, QRegularExpression::CaseInsensitiveOption))) {
#ifndef Q_OS_ANDROID
            if (boardFallback.androidOnly) {
                continue;
            }
#endif
            boardType = boardFallback.boardType;
            name = _boardTypeToString(boardType);
            return true;
        }
    }

    return false;
}

QString QGCSerialPortInfo::_boardTypeToString(BoardType_t boardType)
{
    switch (boardType) {
    case BoardTypePixhawk:
        return QStringLiteral("Pixhawk");
    case BoardTypeSiKRadio:
        return QStringLiteral("SiK Radio");
    case BoardTypePX4Flow:
        return QStringLiteral("PX4 Flow");
    case BoardTypeOpenPilot:
        return QStringLiteral("OpenPilot");
    case BoardTypeRTKGPS:
        return QStringLiteral("RTK GPS");
    case BoardTypeUnknown:
    default:
        return QStringLiteral("Unknown");
    }
}

QList<QGCSerialPortInfo> QGCSerialPortInfo::availablePorts()
{
    QList<QGCSerialPortInfo> list;
    for (const QSerialPortInfo &portInfo : QSerialPortInfo::availablePorts()) {
        if (isSystemPort(portInfo)) {
            continue;
        }

        const QGCSerialPortInfo* const qgcPortInfo = reinterpret_cast<const QGCSerialPortInfo*>(&portInfo);
        list << *qgcPortInfo;
    }

    return list;
}

bool QGCSerialPortInfo::isBootloader() const
{
    BoardType_t boardType;
    QString name;
    if (getBoardInfo(boardType, name)) {
        // TODO: bool SerialLink::_isBootloader()
        return ((boardType == BoardTypePixhawk) && description().contains(QStringLiteral("BL")));
    }

    return false;
}

bool QGCSerialPortInfo::isSystemPort(const QSerialPortInfo &port)
{
#ifdef Q_OS_MACOS
    static const QList<QString> systemPortLocations = {
        QStringLiteral("tty.MALS"),
        QStringLiteral("tty.SOC"),
        QStringLiteral("tty.Bluetooth-Incoming-Port"),
        QStringLiteral("tty.usbserial"),
        QStringLiteral("tty.usbmodem")
    };
    for (const QString &systemPortLocation : systemPortLocations) {
        if (port.systemLocation().contains(systemPortLocation)) {
            return true;
        }
    }
#endif

    // TODO: Add Linux (LTE modems, etc) and Windows as needed

    return false;
}

bool QGCSerialPortInfo::canFlash() const
{
    BoardType_t boardType;
    QString name;
    if (getBoardInfo(boardType, name)) {
        switch(boardType) {
        case QGCSerialPortInfo::BoardTypePixhawk:
        case QGCSerialPortInfo::BoardTypePX4Flow:
        case QGCSerialPortInfo::BoardTypeSiKRadio:
            return true;
        default:
            return false;
        }
    }

    return false;
}
