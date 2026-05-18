#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QtSystemDetection>

QT_BEGIN_NAMESPACE
class QSerialPortInfo;
QT_END_NAMESPACE

class QGCSerialPortInfoTest;

/// \brief QGC's standalone port descriptor. On host this is populated from Qt's QSerialPortInfo;
/// on Android it is populated directly from the Java USB enumeration (see AndroidSerial.cc).
/// Standalone (i.e. not derived from QSerialPortInfo) so the Android build can avoid the
/// vendored qserialportinfo private API.
class QGCSerialPortInfo
{
    friend class QGCSerialPortInfoTest;
public:
    struct Data {
        QString portName;
        QString systemLocation;
        QString description;
        QString manufacturer;
        QString serialNumber;
        quint16 vendorIdentifier  = 0;
        quint16 productIdentifier = 0;
        bool    hasVendorIdentifier  = false;
        bool    hasProductIdentifier = false;
    };

    QGCSerialPortInfo();
    QGCSerialPortInfo(const QGCSerialPortInfo &other);
    QGCSerialPortInfo(QGCSerialPortInfo &&other) noexcept;
    explicit QGCSerialPortInfo(const QString &name);
    explicit QGCSerialPortInfo(Data data);
#ifndef Q_OS_ANDROID
    explicit QGCSerialPortInfo(const QSerialPortInfo &info);
#endif
    ~QGCSerialPortInfo();

    QGCSerialPortInfo &operator=(const QGCSerialPortInfo &other);
    QGCSerialPortInfo &operator=(QGCSerialPortInfo &&other) noexcept;

    enum BoardType_t {
        BoardTypePixhawk = 0,
        BoardTypeSiKRadio,
        BoardTypeOpenPilot,
        BoardTypeRTKGPS,
        BoardTypeUnknown
    };

    QString portName()         const { return _data.portName; }
    QString systemLocation()   const { return _data.systemLocation; }
    QString description()      const { return _data.description; }
    QString manufacturer()     const { return _data.manufacturer; }
    QString serialNumber()     const { return _data.serialNumber; }
    quint16 vendorIdentifier() const { return _data.vendorIdentifier; }
    quint16 productIdentifier()const { return _data.productIdentifier; }
    bool    hasVendorIdentifier()  const { return _data.hasVendorIdentifier; }
    bool    hasProductIdentifier() const { return _data.hasProductIdentifier; }
    bool    isNull() const { return _data.portName.isEmpty() && _data.systemLocation.isEmpty(); }

    bool getBoardInfo(BoardType_t &boardType, QString &name) const;

    /// @return true: we can flash this board type
    bool canFlash() const;

    /// @return true: Board is currently in bootloader
    bool isBootloader() const;

    /// @return true: Board is BlackCube
    bool isBlackCube() const;

    /// Known operating system peripherals that are NEVER a peripheral that we should connect to.
    ///     @return true: Port is a system port and not an autopilot
    static bool isSystemPort(const QGCSerialPortInfo &port);

    static QList<QGCSerialPortInfo> availablePorts();

    /// Platform-independent table of standard baud rates (matches Qt SerialPort
    /// internal kStandardBaudRates). Provided locally so callers don't need to
    /// pull in QtSerialPort/QSerialPortInfo just for this constant.
    static QList<qint32> standardBaudRates();

private:
    struct BoardClassString2BoardType_t {
        const QString classString;
        const BoardType_t boardType = BoardTypeUnknown;
    };

    static bool _loadJsonData();
    static BoardType_t _boardClassStringToType(const QString &boardClass);
    static QString _boardTypeToString(BoardType_t boardType);

    static bool _jsonLoaded;
    static bool _jsonDataValid;

    struct BoardInfo_t {
        int vendorId;
        int productId;
        BoardType_t boardType;
        QString name;
    };
    static QList<BoardInfo_t> _boardInfoList;

    struct BoardRegExpFallback_t {
        QRegularExpression regExp;
        BoardType_t boardType;
        bool androidOnly;
    };
    static QList<BoardRegExpFallback_t> _boardDescriptionFallbackList;
    static QList<BoardRegExpFallback_t> _boardManufacturerFallbackList;

    static constexpr const char *_jsonFileTypeValue = "USBBoardInfo";
    static constexpr const char *_jsonBoardInfoKey = "boardInfo";
    static constexpr const char *_jsonBoardDescriptionFallbackKey = "boardDescriptionFallback";
    static constexpr const char *_jsonBoardManufacturerFallbackKey = "boardManufacturerFallback";
    static constexpr const char *_jsonVendorIDKey = "vendorID";
    static constexpr const char *_jsonProductIDKey = "productID";
    static constexpr const char *_jsonBoardClassKey = "boardClass";
    static constexpr const char *_jsonNameKey = "name";
    static constexpr const char *_jsonRegExpKey = "regExp";
    static constexpr const char *_jsonAndroidOnlyKey = "androidOnly";

    Data _data;
};
Q_DECLARE_METATYPE(QGCSerialPortInfo)
