/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QGCSerialPortInfo_H
#define QGCSerialPortInfo_H

#ifdef __android__
    #include "qserialportinfo.h"
#else
    #include <QSerialPortInfo>
#endif

#include "QGCLoggingCategory.h"

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

    // Vendor and products ids for the boards we care about

    static const int px4VendorId =                          9900;   ///< Vendor ID for all Pixhawk boards and PX4 Flow
    static const int pixhawkFMUV4ProductId =                18;     ///< Product ID for Pixhawk V4 board
    static const int pixhawkFMUV4ProProductId =             19;     ///< Product ID for Pixhawk V4 Pro board
    static const int pixhawkFMUV2ProductId =                17;     ///< Product ID for Pixhawk V2 board
    static const int pixhawkFMUV2OldBootloaderProductId =   22;     ///< Product ID for Bootloader on older Pixhawk V2 boards
    static const int pixhawkFMUV1ProductId =                16;     ///< Product ID for PX4 FMU V1 board

    static const int AeroCoreProductId =                    4097;   ///< Product ID for the AeroCore board
    
    static const int px4FlowProductId =                     21;     ///< Product ID for PX4 Flow board

    static const int MindPXFMUV2ProductId =                 48;     ///< Product ID for the MindPX V2 board
    static const int TAPV1ProductId =                       64;     ///< Product ID for the TAP V1 board
    static const int ASCV1ProductId =                       65;     ///< Product ID for the ASC V1 board

    static const int threeDRRadioVendorId =                 1027;   ///< Vendor ID for 3DR Radio
    static const int threeDRRadioProductId =                24597;  ///< Product ID for 3DR Radio

    static const int siLabsRadioVendorId =                  0x10c4; ///< Vendor ID for SILabs Radio
    static const int siLabsRadioProductId =                 0xea60; ///< Product ID for SILabs Radio

    static const int ubloxRTKVendorId =                     5446;   ///< Vendor ID for ublox RTK
    static const int ubloxRTKProductId =                    424;    ///< Product ID for ublox RTK

    static const int openpilotVendorId =                    0x20A0; ///< Vendor ID for OpenPilot
    static const int revolutionProductId =                  0x415E; ///< Product ID for OpenPilot Revolution
    static const int oplinkProductId =                      0x415C; ///< Product ID for OpenPilot OPLink
    static const int sparky2ProductId =                     0x41D0; ///< Product ID for Taulabs Sparky2
    static const int CC3DProductId =                        0x415D; ///< Product ID for OpenPilot CC3D

    QGCSerialPortInfo(void);
    QGCSerialPortInfo(const QSerialPort & port);

    /// Override of QSerialPortInfo::availablePorts
    static QList<QGCSerialPortInfo> availablePorts(void);

    bool getBoardInfo(BoardType_t& boardType, QString& name) const;

    /// @return true: we can flash this board type
    bool canFlash(void);

    /// @return true: Board is currently in bootloader
    bool isBootloader(void) const;

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
    } BoardFallback_t;

    static void _loadJsonData(void);
    static BoardType_t _boardClassStringToType(const QString& boardClass);
    static QString _boardTypeToString(BoardType_t boardType);

    static bool         _jsonLoaded;
    static const char*  _jsonFileTypeValue;
    static const char*  _jsonBoardInfoKey;
    static const char*  _jsonBoardFallbackKey;
    static const char*  _jsonVendorIDKey;
    static const char*  _jsonProductIDKey;
    static const char*  _jsonBoardClassKey;
    static const char*  _jsonNameKey;
    static const char*  _jsonRegExpKey;
    static const char*  _jsonAndroidOnlyKey;

    static const BoardClassString2BoardType_t   _rgBoardClass2BoardType[BoardTypeUnknown];
    static QList<BoardInfo_t>                   _boardInfoList;
    static QList<BoardFallback_t>               _boardFallbackList;
};

#endif
