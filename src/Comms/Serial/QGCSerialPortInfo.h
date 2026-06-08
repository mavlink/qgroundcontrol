#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtSystemDetection>
#include <QtCore/QtTypes>

QT_BEGIN_NAMESPACE
class QSerialPortInfo;
QT_END_NAMESPACE

class QGCSerialPortInfoTest;

/// Value-type port descriptor with board-classifier. Standalone (not QSerialPortInfo) because QSerialPortInfo's
/// populating ctor is friend-locked to Qt's internal enumerators — Android JNI rows can't use it.
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
        QList<qint32> supportedBaudRates;
    };

    QGCSerialPortInfo();
    QGCSerialPortInfo(const QGCSerialPortInfo &other);
    QGCSerialPortInfo(QGCSerialPortInfo &&other) noexcept;
    /// Expensive: triggers a full availablePorts() enumeration to match by name.
    explicit QGCSerialPortInfo(const QString &name);
    explicit QGCSerialPortInfo(Data data);
    explicit QGCSerialPortInfo(const QSerialPortInfo &info);
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

    struct BoardRegExpFallback_t {
        QRegularExpression regExp;
        BoardType_t boardType;
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
    QList<qint32> supportedBaudRates() const { return _data.supportedBaudRates; }
    bool    isNull() const { return _data.portName.isEmpty() && _data.systemLocation.isEmpty(); }

    bool getBoardInfo(BoardType_t &boardType, QString &name) const;

    bool canFlash() const;

    bool isBootloader() const;

    bool isBlackCube() const;

    /// Known OS peripherals that are never an autopilot — never auto-connect to these.
    static bool isSystemPort(const QGCSerialPortInfo &port);

    static QList<QGCSerialPortInfo> availablePorts();

    /// Port-specific baud list when the platform enumerator knows it (Android USB-host); empty so the
    /// caller falls back to the standard set on host. Per-platform TU (QGCSerialPortInfo_host/android.cc).
    static QList<qint32> portSpecificBaudRates(const QString &portName);

    /// Human-readable name for a systemLocation() from the current enumeration; empty if the device isn't present.
    static QString displayNameForLocation(const QString &systemLocation);

    /// Curated baud-rate list for UI dropdowns; QSerialPortInfo::standardBaudRates() caps at 256000 on Windows.
    static QList<qint32> standardBaudRates();

    /// UI baud-rate strings: platform-specific rates when the enumerator knows them (Android USB-host),
    /// else the curated default set merged with standardBaudRates(). Sorted ascending, unique.
    static QStringList supportedBaudRateStrings(const QString &portName = QString());

private:
    struct BoardClassString2BoardType_t {
        const QString classString;
        const BoardType_t boardType = BoardTypeUnknown;
    };

    struct BoardInfo_t {
        int vendorId;
        int productId;
        BoardType_t boardType;
        QString name;
    };

    struct BoardDatabase {
        QList<BoardInfo_t> boardInfo;
        QList<BoardRegExpFallback_t> descriptionFallback;
        QList<BoardRegExpFallback_t> manufacturerFallback;
        bool valid = false;
    };

    // Platform enumeration glue, defined in the per-platform TU (QGCSerialPortInfo_host/android.cc) so this
    // class stays #ifdef-free.
    /// Devices enumerated natively (Android: JNI USB results); host returns empty (uses QSerialPortInfo).
    static QList<QGCSerialPortInfo> _nativeDevices();
    /// Whether a QSerialPortInfo row joins the visible list; Android keeps only "/dev/tty*".
    static bool _acceptQSerialPortInfo(const QSerialPortInfo &info);
    /// Platform-specific system-port test backing isSystemPort().
    static bool _platformIsSystemPort(const QGCSerialPortInfo &port);
    /// Extra description-regex fallbacks on top of USBBoardInfo.json (Android: SiK USB-UART adapters).
    static QList<BoardRegExpFallback_t> _additionalDescriptionFallbacks();

    // Parsed once on first access (magic-static is the thread-safe-once guard — no manual mutex).
    static const BoardDatabase &_boardDatabase();
    static BoardDatabase _loadBoardDatabase();
    static BoardType_t _boardClassStringToType(const QString &boardClass);
    static QString _boardTypeToString(BoardType_t boardType);

    static constexpr const char *_jsonFileTypeValue = "USBBoardInfo";
    static constexpr const char *_jsonBoardInfoKey = "boardInfo";
    static constexpr const char *_jsonBoardDescriptionFallbackKey = "boardDescriptionFallback";
    static constexpr const char *_jsonBoardManufacturerFallbackKey = "boardManufacturerFallback";
    static constexpr const char *_jsonVendorIDKey = "vendorID";
    static constexpr const char *_jsonProductIDKey = "productID";
    static constexpr const char *_jsonBoardClassKey = "boardClass";
    static constexpr const char *_jsonNameKey = "name";
    static constexpr const char *_jsonRegExpKey = "regExp";

    Data _data;
};
Q_DECLARE_METATYPE(QGCSerialPortInfo)
