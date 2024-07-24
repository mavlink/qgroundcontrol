/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QtSystemDetection>
#ifdef Q_OS_ANDROID
    #include "qserialportinfo.h"
#else
    #include <QtSerialPort/QSerialPortInfo>
#endif
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCSerialPortInfoLog)

/// QGC's version of Qt QSerialPortInfo. It provides additional information about board types
/// that QGC cares about.
class QGCSerialPortInfo : public QSerialPortInfo
{
public:
    typedef enum {
        BoardTypePixhawk,
        BoardTypeSiKRadio,
        BoardTypePX4Flow,
        BoardTypeOpenPilot,
        BoardTypeRTKGPS,
        BoardTypeUnknown
    } BoardType_t;

    QGCSerialPortInfo(void);
    QGCSerialPortInfo(const QSerialPort & port);

    /// Override of QSerialPortInfo::availablePorts
    static QList<QGCSerialPortInfo> availablePorts(void);

    bool getBoardInfo(BoardType_t& boardType, QString& name) const;

    /// @return true: we can flash this board type
    bool canFlash(void) const;

    /// @return true: Board is currently in bootloader
    bool isBootloader(void) const;

    /// @return true: Port is a system port and not an autopilot
    static bool isSystemPort(QSerialPortInfo* port);

private:
    typedef struct {
        const char* classString;
        BoardType_t boardType;
    } BoardClassString2BoardType_t;

    typedef struct {
        int         vendorId;
        int         productId;
        BoardType_t boardType;
        QString     name;
    } BoardInfo_t;

    typedef struct {
        QString     regExp;
        BoardType_t boardType;
        bool        androidOnly;
    } BoardRegExpFallback_t;

    static void _loadJsonData(void);
    static BoardType_t _boardClassStringToType(const QString& boardClass);
    static QString _boardTypeToString(BoardType_t boardType);

    static bool         _jsonLoaded;
    static constexpr const char*  _jsonFileTypeValue =                "USBBoardInfo";
    static constexpr const char*  _jsonBoardInfoKey =                 "boardInfo";
    static constexpr const char*  _jsonBoardDescriptionFallbackKey =  "boardDescriptionFallback";
    static constexpr const char*  _jsonBoardManufacturerFallbackKey = "boardManufacturerFallback";
    static constexpr const char*  _jsonVendorIDKey =                  "vendorID";
    static constexpr const char*  _jsonProductIDKey =                 "productID";
    static constexpr const char*  _jsonBoardClassKey =                "boardClass";
    static constexpr const char*  _jsonNameKey =                      "name";
    static constexpr const char*  _jsonRegExpKey =                    "regExp";
    static constexpr const char*  _jsonAndroidOnlyKey =               "androidOnly";

    static const BoardClassString2BoardType_t   _rgBoardClass2BoardType[BoardTypeUnknown];
    static QList<BoardInfo_t>                   _boardInfoList;
    static QList<BoardRegExpFallback_t>         _boardDescriptionFallbackList;
    static QList<BoardRegExpFallback_t>         _boardManufacturerFallbackList;
};
