/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QTextStream>

/// Support for Intel Hex firmware file
class FirmwareImage : public QObject
{
    Q_OBJECT
    
public:
    FirmwareImage(QObject *parent = 0);
    
    /// Loads the specified image file. Supported formats: .px4, .bin, .ihx.
    /// Emits errorMesssage and statusMessage signals while loading.
    ///     @param imageFilename Image file to load
    ///     @param boardId Board id that we are going to load this image onto
    /// @return true: success, false: failure
    bool load(const QString& imageFilename, uint32_t boardId);
    
    /// Returns the number of bytes in the image.
    uint32_t imageSize(void) const { return _imageSize; }
    
    /// @return true: image format is .bin
    bool imageIsBinFormat(void) const { return _binFormat; }
    
    /// @return Filename for .bin file
    QString binFilename(void) const { return _binFilename; }
    
    /// @return Block count from .ihx image
    uint16_t ihxBlockCount(void) const;
    
    /// Retrieves the specified block from the .ihx image
    ///     @param index Index of block to return
    ///     @param address Address of returned block
    ///     @param byets Bytes of returned block
    /// @return true: block retrieved
    bool ihxGetBlock(uint16_t index, uint16_t& address, QByteArray& bytes) const;
    
    /// @return true: actual boardId is compatible with firmware boardId
    bool isCompatible(uint32_t boardId, uint32_t firmwareId);

signals:
    void errorMessage(const QString& errorString);
    void statusMessage(const QString& warningtring);
    
private:
    bool _binLoad(const QString& px4Filename);
    bool _px4Load(const QString& px4Filename);
    bool _ihxLoad(const QString& ihxFilename);
    
    bool _readByteFromStream(QTextStream& stream, uint8_t& byte);
    bool _readWordFromStream(QTextStream& stream, uint16_t& word);
    bool _readBytesFromStream(QTextStream& stream, uint8_t byteCount, QByteArray& bytes);
    
    bool _decompressJsonValue(const QJsonObject&	jsonObject,
                              const QByteArray&     jsonDocBytes,
                              const QString&		sizeKey,
                              const QString&		bytesKey,
                              QByteArray&			decompressedBytes);
    
    typedef struct {
        uint16_t    address;
        QByteArray  bytes;
    } IntelHexBlock_t;

    bool                    _binFormat;
    uint32_t                _boardId;
    QString                 _binFilename;
    QList<IntelHexBlock_t>  _ihxBlocks;
    uint32_t                _imageSize;

    static constexpr const char* _jsonBoardIdKey =            "board_id";
    static constexpr const char* _jsonParamXmlSizeKey =       "parameter_xml_size";
    static constexpr const char* _jsonParamXmlKey =           "parameter_xml";
    static constexpr const char* _jsonAirframeXmlSizeKey =    "airframe_xml_size";
    static constexpr const char* _jsonAirframeXmlKey =        "airframe_xml";
    static constexpr const char* _jsonImageSizeKey =          "image_size";
    static constexpr const char* _jsonImageKey =              "image";
    static constexpr const char* _jsonMavAutopilotKey =       "mav_autopilot";
};

