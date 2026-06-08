#include "QGCSerialPortInfo.h"

#include "JsonParsing.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>

#include <algorithm>
#include <ranges>

#include <QtSerialPort/QSerialPortInfo>

QGC_LOGGING_CATEGORY(QGCSerialPortInfoLog, "Comms.QGCSerialPortInfo")

// Curated baud list; QSerialPortInfo::standardBaudRates() caps at 256000 on Windows and drops high rates on Android.
QList<qint32> QGCSerialPortInfo::standardBaudRates()
{
    static const QList<qint32> kRates = {
        50,     75,     110,     134,     150,     200,     300,     600,     1200,    1800,
        2400,   4800,   9600,    19200,   38400,   57600,   115200,  230400,  460800,  500000,
        576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000,
    };
    return kRates;
}

QStringList QGCSerialPortInfo::supportedBaudRateStrings(const QString &portName)
{
    static const QSet<qint32> kDefaultSupportedBaudRates = {
#ifdef Q_OS_UNIX
        50,     75,
#endif
        110,
#ifdef Q_OS_UNIX
        150,    200,    134,
#endif
        300,    600,    1200,
#ifdef Q_OS_UNIX
        1800,
#endif
        2400,   4800,   9600,
#ifdef Q_OS_WIN
        14400,
#endif
        19200,  38400,
#ifdef Q_OS_WIN
        56000,
#endif
        57600,  115200,
#ifdef Q_OS_WIN
        128000,
#endif
        230400,
#ifdef Q_OS_WIN
        256000,
#endif
        460800, 500000,
#ifdef Q_OS_LINUX
        576000,
#endif
        921600,
    };

    QList<qint32> activeSupportedBaudRates = portSpecificBaudRates(portName);
    const bool usePortSpecificRates = !activeSupportedBaudRates.isEmpty();
    if (!usePortSpecificRates) {
        activeSupportedBaudRates = standardBaudRates();
    }

    QList<qint32> mergedBaudRateList;
    if (usePortSpecificRates) {
        mergedBaudRateList = activeSupportedBaudRates;
    } else {
        mergedBaudRateList = QList<qint32>(kDefaultSupportedBaudRates.constBegin(), kDefaultSupportedBaudRates.constEnd());
        mergedBaudRateList.append(activeSupportedBaudRates);
    }

    std::ranges::sort(mergedBaudRateList);
    const auto duplicates = std::ranges::unique(mergedBaudRateList);
    mergedBaudRateList.erase(duplicates.begin(), duplicates.end());

    QStringList supportBaudRateStrings;
    supportBaudRateStrings.reserve(mergedBaudRateList.size());
    for (const qint32 rate : std::as_const(mergedBaudRateList)) {
        supportBaudRateStrings.append(QString::number(rate));
    }

    return supportBaudRateStrings;
}

QGCSerialPortInfo::QGCSerialPortInfo() = default;
QGCSerialPortInfo::QGCSerialPortInfo(const QGCSerialPortInfo &) = default;
QGCSerialPortInfo::QGCSerialPortInfo(QGCSerialPortInfo &&) noexcept = default;
QGCSerialPortInfo &QGCSerialPortInfo::operator=(const QGCSerialPortInfo &) = default;
QGCSerialPortInfo &QGCSerialPortInfo::operator=(QGCSerialPortInfo &&) noexcept = default;
QGCSerialPortInfo::~QGCSerialPortInfo() = default;

QGCSerialPortInfo::QGCSerialPortInfo(Data data)
    : _data(std::move(data))
{
}

QGCSerialPortInfo::QGCSerialPortInfo(const QString &name)
{
    const QList<QGCSerialPortInfo> ports = availablePorts();
    for (const QGCSerialPortInfo &info : ports) {
        if (info.portName() == name || info.systemLocation() == name) {
            _data = info._data;
            return;
        }
    }
}

QGCSerialPortInfo::QGCSerialPortInfo(const QSerialPortInfo &info)
{
    _data.portName            = info.portName();
    _data.systemLocation      = info.systemLocation();
    _data.description         = info.description();
    _data.manufacturer        = info.manufacturer();
    _data.serialNumber        = info.serialNumber();
    _data.vendorIdentifier    = info.vendorIdentifier();
    _data.productIdentifier   = info.productIdentifier();
    _data.hasVendorIdentifier = info.hasVendorIdentifier();
    _data.hasProductIdentifier= info.hasProductIdentifier();
}

const QGCSerialPortInfo::BoardDatabase &QGCSerialPortInfo::_boardDatabase()
{
    static const BoardDatabase db = _loadBoardDatabase();
    return db;
}

QGCSerialPortInfo::BoardDatabase QGCSerialPortInfo::_loadBoardDatabase()
{
    BoardDatabase db;

    QString errorString;
    int version;
    const QJsonObject json = JsonParsing::openInternalQGCJsonFile(QStringLiteral(":/json/USBBoardInfo.json"), QString(_jsonFileTypeValue), 1, 1, version, errorString);
    if (!errorString.isEmpty()) {
        qCWarning(QGCSerialPortInfoLog) << "Internal Error:" << errorString;
        return db;
    }

    static const QList<JsonParsing::KeyValidateInfo> rootKeyInfoList = {
        { _jsonBoardInfoKey, QJsonValue::Array, true },
        { _jsonBoardDescriptionFallbackKey, QJsonValue::Array, true },
        { _jsonBoardManufacturerFallbackKey, QJsonValue::Array, true },
    };
    if (!JsonParsing::validateKeys(json, rootKeyInfoList, errorString)) {
        qCWarning(QGCSerialPortInfoLog) << errorString;
        return db;
    }

    static const QList<JsonParsing::KeyValidateInfo> boardKeyInfoList = {
        { _jsonVendorIDKey, QJsonValue::Double, true },
        { _jsonProductIDKey, QJsonValue::Double, true },
        { _jsonBoardClassKey, QJsonValue::String, true },
        { _jsonNameKey, QJsonValue::String, true },
    };
    const QJsonArray rgBoardInfo = json[_jsonBoardInfoKey].toArray();
    for (const QJsonValue &jsonValue : rgBoardInfo) {
        if (!jsonValue.isObject()) {
            qCWarning(QGCSerialPortInfoLog) << "Entry in boardInfo array is not object";
            return db;
        }

        const QJsonObject boardObject = jsonValue.toObject();
        if (!JsonParsing::validateKeys(boardObject, boardKeyInfoList, errorString)) {
            qCWarning(QGCSerialPortInfoLog) << errorString;
            return db;
        }

        const BoardInfo_t boardInfo = {
            boardObject[_jsonVendorIDKey].toInt(),
            boardObject[_jsonProductIDKey].toInt(),
            _boardClassStringToType(boardObject[_jsonBoardClassKey].toString()),
            boardObject[_jsonNameKey].toString()
        };
        if (boardInfo.boardType == BoardTypeUnknown) {
            qCWarning(QGCSerialPortInfoLog) << "Bad board class" << boardObject[_jsonBoardClassKey].toString();
            return db;
        }

        db.boardInfo.append(boardInfo);
    }

    static const QList<JsonParsing::KeyValidateInfo> fallbackKeyInfoList = {
        { _jsonRegExpKey, QJsonValue::String, true },
        { _jsonBoardClassKey, QJsonValue::String, true },
    };

    const auto parseFallback = [&](const char *jsonKey, const char *label,
                                   QList<BoardRegExpFallback_t> &out) -> bool {
        const QJsonArray arr = json[jsonKey].toArray();
        for (const QJsonValue &jsonValue : arr) {
            if (!jsonValue.isObject()) {
                qCWarning(QGCSerialPortInfoLog) << "Entry in boardFallback array is not object";
                return false;
            }
            const QJsonObject obj = jsonValue.toObject();
            if (!JsonParsing::validateKeys(obj, fallbackKeyInfoList, errorString)) {
                qCWarning(QGCSerialPortInfoLog) << errorString;
                return false;
            }
            const QString pattern = obj[_jsonRegExpKey].toString();
            const QRegularExpression regExp(pattern, QRegularExpression::CaseInsensitiveOption);
            if (!regExp.isValid()) {
                qCWarning(QGCSerialPortInfoLog) << "Invalid regular expression in board" << label << "fallback:"
                                                 << regExp.errorString() << "pattern:" << pattern;
                return false;
            }
            const BoardRegExpFallback_t fb = {
                regExp,
                _boardClassStringToType(obj[_jsonBoardClassKey].toString())
            };
            if (fb.boardType == BoardTypeUnknown) {
                qCWarning(QGCSerialPortInfoLog) << "Bad board class" << obj[_jsonBoardClassKey].toString();
                return false;
            }
            out.append(fb);
        }
        return true;
    };

    if (!parseFallback(_jsonBoardDescriptionFallbackKey,  "description",  db.descriptionFallback) ||
        !parseFallback(_jsonBoardManufacturerFallbackKey, "manufacturer", db.manufacturerFallback)) {
        return db;
    }

    for (const BoardRegExpFallback_t &fb : _additionalDescriptionFallbacks()) {
        db.descriptionFallback.append(fb);
    }

    db.valid = true;

    return db;
}

QGCSerialPortInfo::BoardType_t QGCSerialPortInfo::_boardClassStringToType(const QString &boardClass)
{
    static const BoardClassString2BoardType_t rgBoardClass2BoardType[BoardTypeUnknown] = {
        { _boardTypeToString(BoardTypePixhawk), BoardTypePixhawk },
        { _boardTypeToString(BoardTypeRTKGPS), BoardTypeRTKGPS },
        { _boardTypeToString(BoardTypeSiKRadio), BoardTypeSiKRadio },
        { _boardTypeToString(BoardTypeOpenPilot), BoardTypeOpenPilot },
    };

    for (const BoardClassString2BoardType_t &board : rgBoardClass2BoardType) {
        if (boardClass == board.classString) {
            return board.boardType;
        }
    }

    return BoardTypeUnknown;
}

bool QGCSerialPortInfo::getBoardInfo(QGCSerialPortInfo::BoardType_t &boardType, QString &name) const
{
    boardType = BoardTypeUnknown;

    const BoardDatabase &db = _boardDatabase();
    if (!db.valid) {
        return false;
    }

    if (isNull()) {
        return false;
    }

    for (const BoardInfo_t &boardInfo : db.boardInfo) {
        if ((vendorIdentifier() == boardInfo.vendorId) && ((productIdentifier() == boardInfo.productId) || (boardInfo.productId == 0))) {
            boardType = boardInfo.boardType;
            name = boardInfo.name;
            return true;
        }
    }

    const auto matchFallback = [&](const QList<BoardRegExpFallback_t> &list, const QString &field) {
        for (const BoardRegExpFallback_t &fb : list) {
            if (field.contains(fb.regExp)) {
                boardType = fb.boardType;
                name = _boardTypeToString(boardType);
                return true;
            }
        }
        return false;
    };

    return matchFallback(db.descriptionFallback,  description())
        || matchFallback(db.manufacturerFallback, manufacturer());
}

QString QGCSerialPortInfo::_boardTypeToString(BoardType_t boardType)
{
    switch (boardType) {
    case BoardTypePixhawk:
        return QStringLiteral("Pixhawk");
    case BoardTypeSiKRadio:
        return QStringLiteral("SiK Radio");
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
    QList<QGCSerialPortInfo> list = _nativeDevices();
    QSet<QString> seenLocations;
    for (const QGCSerialPortInfo &portInfo : list) {
        seenLocations.insert(portInfo.systemLocation());
    }

    // Android: acceptQSerialPortInfo() keeps only kernel TTYs (/dev/tty*); USB-host rows come from nativeDevices().
    for (const QSerialPortInfo &portInfo : QSerialPortInfo::availablePorts()) {
        if (!_acceptQSerialPortInfo(portInfo)) {
            continue;
        }
        QGCSerialPortInfo qgcPortInfo(portInfo);
        if (isSystemPort(qgcPortInfo) || seenLocations.contains(qgcPortInfo.systemLocation())) {
            continue;
        }
        list << qgcPortInfo;
    }

    return list;
}

QString QGCSerialPortInfo::displayNameForLocation(const QString &systemLocation)
{
    for (const QGCSerialPortInfo &portInfo : availablePorts()) {
        if (portInfo.systemLocation() == systemLocation) {
            return portInfo.portName();
        }
    }

    return QString();
}

bool QGCSerialPortInfo::isBootloader() const
{
    BoardType_t boardType;
    QString name;
    if (!getBoardInfo(boardType, name)) {
        return false;
    }

    return ((boardType == BoardTypePixhawk) && description().contains(QStringLiteral("BL")));
}

bool QGCSerialPortInfo::isBlackCube() const
{
    return description().contains(QStringLiteral("CubeBlack"));
}

bool QGCSerialPortInfo::isSystemPort(const QGCSerialPortInfo &port)
{
    return _platformIsSystemPort(port);
}

bool QGCSerialPortInfo::canFlash() const
{
    BoardType_t boardType;
    QString name;
    if (!getBoardInfo(boardType, name)) {
        return false;
    }

    static const QList<BoardType_t> flashable = {
        BoardTypePixhawk,
        BoardTypeSiKRadio
    };

    return flashable.contains(boardType);
}
