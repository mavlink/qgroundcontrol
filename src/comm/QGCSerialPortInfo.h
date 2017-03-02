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
