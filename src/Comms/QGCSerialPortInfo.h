/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QtSystemDetection>
#ifdef Q_OS_ANDROID
    #include "qserialportinfo.h"
#else
    #include <QtSerialPort/QSerialPortInfo>
#endif

class QGCSerialPortInfoTest;

Q_DECLARE_LOGGING_CATEGORY(QGCSerialPortInfoLog)

/// QGC's version of Qt QSerialPortInfo. It provides additional information about board types
/// that QGC cares about.
class QGCSerialPortInfo : public QSerialPortInfo
{
    friend class QGCSerialPortInfoTest;
public:
    QGCSerialPortInfo();
    explicit QGCSerialPortInfo(const QSerialPort &port);
    ~QGCSerialPortInfo();

    enum BoardType_t {
        BoardTypePixhawk = 0,
        BoardTypeSiKRadio,
        BoardTypeOpenPilot,
        BoardTypeRTKGPS,
        BoardTypeUnknown
    };

    bool getBoardInfo(BoardType_t &boardType, QString &name) const;

    /// @return true: we can flash this board type
    bool canFlash() const;

    /// @return true: Board is currently in bootloader
    bool isBootloader() const;

    /// Known operating system peripherals that are NEVER a peripheral that we should connect to.
    ///     @return true: Port is a system port and not an autopilot
    static bool isSystemPort(const QSerialPortInfo &port);

    /// Override of QSerialPortInfo::availablePorts
    static QList<QGCSerialPortInfo> availablePorts();

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
        QString regExp;
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
};
Q_DECLARE_METATYPE(QGCSerialPortInfo)
