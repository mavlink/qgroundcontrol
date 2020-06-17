/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Support for Intel Hex firmware file
///     @author Don Gagne <don@thegagnes.com>

#include "FirmwareImage.h"
#include "QGCLoggingCategory.h"
#include "JsonHelper.h"
#include "QGCMAVLink.h"
#include "QGCApplication.h"
#include "FirmwarePlugin.h"
#include "ParameterManager.h"
#include "Bootloader.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

const char* FirmwareImage::_jsonBoardIdKey =            "board_id";
const char* FirmwareImage::_jsonParamXmlSizeKey =       "parameter_xml_size";
const char* FirmwareImage::_jsonParamXmlKey =           "parameter_xml";
const char* FirmwareImage::_jsonAirframeXmlSizeKey =    "airframe_xml_size";
const char* FirmwareImage::_jsonAirframeXmlKey =        "airframe_xml";
const char* FirmwareImage::_jsonImageSizeKey =          "image_size";
const char* FirmwareImage::_jsonImageKey =              "image";
const char* FirmwareImage::_jsonMavAutopilotKey =       "mav_autopilot";

FirmwareImage::FirmwareImage(QObject* parent) :
    QObject(parent),
    _imageSize(0)
{
    
}

bool FirmwareImage::load(const QString& imageFilename, uint32_t boardId)
{
    _imageSize = 0;
    _boardId = boardId;
    
    if (imageFilename.endsWith(".bin")) {
        _binFormat = true;
        return _binLoad(imageFilename);
    } else if (imageFilename.endsWith(".px4")) {
        _binFormat = true;
        return _px4Load(imageFilename);
    } else if (imageFilename.endsWith(".apj")) {
        _binFormat = true;
        return _px4Load(imageFilename);
    } else if (imageFilename.endsWith(".ihx")) {
        _binFormat = false;
        return _ihxLoad(imageFilename);
    } else {
        emit statusMessage("Unsupported file format");
        return false;
    }
}

bool FirmwareImage::_readByteFromStream(QTextStream& stream, uint8_t& byte)
{
    QString hex = stream.read(2);
    
    if (hex.count() != 2) {
        return false;
    }
    
    bool success;
    byte = (uint8_t)hex.toInt(&success, 16);
    
    return success;
}

bool FirmwareImage::_readWordFromStream(QTextStream& stream, uint16_t& word)
{
    QString hex = stream.read(4);
    
    if (hex.count() != 4) {
        return false;
    }
    
    bool success;
    word = (uint16_t)hex.toInt(&success, 16);
    
    return success;
}

bool FirmwareImage::_readBytesFromStream(QTextStream& stream, uint8_t byteCount, QByteArray& bytes)
{
    bytes.clear();
    
    while (byteCount) {
        uint8_t byte;
        
        if (!_readByteFromStream(stream, byte)) {
            return false;
        }
        bytes += byte;
        
        byteCount--;
    }
    
    return true;
}

bool FirmwareImage::_ihxLoad(const QString& ihxFilename)
{
    _imageSize = 0;
    _ihxBlocks.clear();
    
    QFile ihxFile(ihxFilename);
    if (!ihxFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit statusMessage(QString("Unable to open firmware file %1, error: %2").arg(ihxFilename, ihxFile.errorString()));
        return false;
    }
    
    QTextStream stream(&ihxFile);
    
    while (true) {
        if (stream.read(1) != ":") {
            emit statusMessage("Incorrectly formatted .ihx file, line does not begin with :");
            return false;
        }
        
        uint8_t     blockByteCount;
        uint16_t    address;
        uint8_t     recordType;
        QByteArray  bytes;
        uint8_t     crc;
        
        if (!_readByteFromStream(stream, blockByteCount) ||
            !_readWordFromStream(stream, address) ||
            !_readByteFromStream(stream, recordType) ||
            !_readBytesFromStream(stream, blockByteCount, bytes) ||
            !_readByteFromStream(stream, crc)) {
            emit statusMessage(tr("Incorrectly formatted line in .ihx file, line too short"));
            return false;
        }
        
        if (!(recordType == 0 || recordType == 1)) {
            emit statusMessage(tr("Unsupported record type in file: %1").arg(recordType));
            return false;
        }
        
        if (recordType == 0) {
            bool appendToLastBlock = false;
            
            // Can we append this block to the last one?
            
            if (_ihxBlocks.count()) {
                int lastBlockIndex = _ihxBlocks.count() - 1;
                
                if (_ihxBlocks[lastBlockIndex].address + _ihxBlocks[lastBlockIndex].bytes.count() == address) {
                    appendToLastBlock = true;
                }
            }
            
            if (appendToLastBlock) {
                _ihxBlocks[_ihxBlocks.count() - 1].bytes += bytes;
                // Too noisy even for verbose
                //qCDebug(FirmwareUpgradeVerboseLog) << QString("_ihxLoad - append - address:%1 size:%2 block:%3").arg(address).arg(blockByteCount).arg(ihxBlockCount());
            } else {
                IntelHexBlock_t block;
                
                block.address = address;
                block.bytes = bytes;
                
                _ihxBlocks += block;
                qCDebug(FirmwareUpgradeVerboseLog) << QString("_ihxLoad - new block - address:%1 size:%2 block:%3").arg(address).arg(blockByteCount).arg(ihxBlockCount());
            }
            
            _imageSize += blockByteCount;
        } else if (recordType == 1) {
            // EOF
            qCDebug(FirmwareUpgradeLog) << QString("_ihxLoad - EOF");
            break;
        }
        
        // Move to next line
        stream.readLine();
    }
    
    ihxFile.close();
    
    return true;
}

bool FirmwareImage::isCompatible(uint32_t boardId, uint32_t firmwareId) {
    bool result = false;
    if (boardId == firmwareId ) {
        result = true;
    }
    switch(boardId) {
    case Bootloader::boardIDAUAVX2_1: // AUAVX2.1 is compatible with px4-v2/v3
        if (firmwareId == 9) result = true;
        break;
    case Bootloader::boardIDPX4FMUV3:
        if (firmwareId == 9) result = true;
        break;
    default:
        break;
    }
    return result;
}

bool FirmwareImage::_px4Load(const QString& imageFilename)
{
    _imageSize = 0;
    
    // We need to collect information from the .px4 file as well as pull the binary image out to a separate file.
    
    QFile px4File(imageFilename);
    if (!px4File.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit statusMessage(tr("Unable to open firmware file %1, error: %2").arg(imageFilename, px4File.errorString()));
        return false;
    }
    
    QByteArray bytes = px4File.readAll();
    px4File.close();
    QJsonDocument doc = QJsonDocument::fromJson(bytes);
    
    if (doc.isNull()) {
        emit statusMessage(tr("Supplied file is not a valid JSON document"));
        return false;
    }
    
    QJsonObject px4Json = doc.object();
    
    // Make sure the keys we need are available
    QString errorString;
    QStringList requiredKeys;
    requiredKeys << _jsonBoardIdKey << _jsonImageKey << _jsonImageSizeKey;
    if (!JsonHelper::validateRequiredKeys(px4Json, requiredKeys, errorString)) {
        emit statusMessage(tr("Firmware file mission required key: %1").arg(errorString));
        return false;
    }

    // Make sure the keys are the correct type
    QStringList keys;
    QList<QJsonValue::Type> types;
    keys << _jsonBoardIdKey << _jsonParamXmlSizeKey << _jsonParamXmlKey << _jsonAirframeXmlSizeKey << _jsonAirframeXmlKey << _jsonImageSizeKey << _jsonImageKey << _jsonMavAutopilotKey;
    types << QJsonValue::Double << QJsonValue::Double << QJsonValue::String << QJsonValue::Double << QJsonValue::String << QJsonValue::Double << QJsonValue::String << QJsonValue::Double;
    if (!JsonHelper::validateKeyTypes(px4Json, keys, types, errorString)) {
        emit statusMessage(tr("Firmware file has invalid key: %1").arg(errorString));
        return false;
    }

    uint32_t firmwareBoardId = (uint32_t)px4Json.value(_jsonBoardIdKey).toInt();
    if (!isCompatible(_boardId, firmwareBoardId)) {
        emit statusMessage(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardId).arg(_boardId));
        return false;
    }

    // What firmware type is this?
    MAV_AUTOPILOT firmwareType = (MAV_AUTOPILOT)px4Json[_jsonMavAutopilotKey].toInt(MAV_AUTOPILOT_PX4);
    emit statusMessage(QString("MAV_AUTOPILOT = %1").arg(firmwareType));
    
    // Decompress the parameter xml and save to file
    QByteArray decompressedBytes;
    bool success = _decompressJsonValue(px4Json,               // JSON object
                                        bytes,                 // Raw bytes of JSON document
                                        _jsonParamXmlSizeKey,  // key which holds byte size
                                        _jsonParamXmlKey,      // key which holds compressed bytes
                                        decompressedBytes);    // Returned decompressed bytes
    if (success) {
        QString parameterFilename = QGCApplication::cachedParameterMetaDataFile();
        QFile parameterFile(QGCApplication::cachedParameterMetaDataFile());

        if (parameterFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qint64 bytesWritten = parameterFile.write(decompressedBytes);
            if (bytesWritten != decompressedBytes.count()) {
                emit statusMessage(tr("Write failed for parameter meta data file, error: %1").arg(parameterFile.errorString()));
                parameterFile.close();
                QFile::remove(parameterFilename);
            } else {
                parameterFile.close();
            }
        } else {
            emit statusMessage(tr("Unable to open parameter meta data file %1 for writing, error: %2").arg(parameterFilename, parameterFile.errorString()));
        }

        // Cache this file with the system
        ParameterManager::cacheMetaDataFile(parameterFilename, firmwareType);
    }

    // Decompress the airframe xml and save to file
    success = _decompressJsonValue(px4Json,                         // JSON object
                                        bytes,                      // Raw bytes of JSON document
                                        _jsonAirframeXmlSizeKey,    // key which holds byte size
                                        _jsonAirframeXmlKey,        // key which holds compressed bytes
                                        decompressedBytes);         // Returned decompressed bytes
    if (success) {
        QString airframeFilename = QGCApplication::cachedAirframeMetaDataFile();
        //qDebug() << airframeFilename;
        QFile airframeFile(QGCApplication::cachedAirframeMetaDataFile());

        if (airframeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qint64 bytesWritten = airframeFile.write(decompressedBytes);
            if (bytesWritten != decompressedBytes.count()) {
                // FIXME: What about these warnings?
                emit statusMessage(tr("Write failed for airframe meta data file, error: %1").arg(airframeFile.errorString()));
                airframeFile.close();
                QFile::remove(airframeFilename);
            } else {
                airframeFile.close();
            }
        } else {
            emit statusMessage(tr("Unable to open airframe meta data file %1 for writing, error: %2").arg(airframeFilename, airframeFile.errorString()));
        }
    }
    
    // Decompress the image and save to file
    _imageSize = px4Json.value(QString("image_size")).toInt();
    success = _decompressJsonValue(px4Json,               // JSON object
                                   bytes,                 // Raw bytes of JSON document
                                   _jsonImageSizeKey,     // key which holds byte size
                                   _jsonImageKey,         // key which holds compressed bytes
                                   decompressedBytes);    // Returned decompressed bytes
    if (!success) {
        return false;
    }
    
    // Pad image to 4-byte boundary
    while ((decompressedBytes.count() % 4) != 0) {
        decompressedBytes.append(static_cast<char>(static_cast<unsigned char>(0xFF)));
    }
    
    // Store decompressed image file in same location as original download file
    QDir imageDir = QFileInfo(imageFilename).dir();
    QString decompressFilename = imageDir.filePath("PX4FlashUpgrade.bin");
    
    QFile decompressFile(decompressFilename);
    if (!decompressFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit statusMessage(tr("Unable to open decompressed file %1 for writing, error: %2").arg(decompressFilename, decompressFile.errorString()));
        return false;
    }
    
    qint64 bytesWritten = decompressFile.write(decompressedBytes);
    if (bytesWritten != decompressedBytes.count()) {
        emit statusMessage(tr("Write failed for decompressed image file, error: %1").arg(decompressFile.errorString()));
        return false;
    }
    decompressFile.close();
    
    _binFilename = decompressFilename;
    
    return true;
}

/// Decompress a set of bytes stored in a Json document.
bool FirmwareImage::_decompressJsonValue(const QJsonObject&	jsonObject,			///< JSON object
                                         const QByteArray&	jsonDocBytes,		///< Raw bytes of JSON document
                                         const QString&		sizeKey,			///< key which holds byte size
                                         const QString&		bytesKey,			///< key which holds compress bytes
                                         QByteArray&		decompressedBytes)	///< Returned decompressed bytes
{
    // Validate decompressed size key
    if (!jsonObject.contains(sizeKey)) {
        emit statusMessage(QString("Firmware file missing %1 key").arg(sizeKey));
        return false;
    }
    int decompressedSize = jsonObject.value(QString(sizeKey)).toInt();
    if (decompressedSize == 0) {
        emit statusMessage(tr("Firmware file has invalid decompressed size for %1").arg(sizeKey));
        return false;
    }
    
    // XXX Qt's JSON string handling is terribly broken, strings
    // with some length (18K / 25K) are just weirdly cut.
    // The code below works around this by manually 'parsing'
    // for the image string. Since its compressed / checksummed
    // this should be fine.
    
    QStringList parts = QString(jsonDocBytes).split(QString("\"%1\": \"").arg(bytesKey));
    if (parts.count() == 1) {
        emit statusMessage(tr("Could not find compressed bytes for %1 in Firmware file").arg(bytesKey));
        return false;
    }
    parts = parts.last().split("\"");
    if (parts.count() == 1) {
        emit statusMessage(tr("Incorrectly formed compressed bytes section for %1 in Firmware file").arg(bytesKey));
        return false;
    }
    
    // Store decompressed size as first four bytes. This is required by qUncompress routine.
    QByteArray raw;
    raw.append((unsigned char)((decompressedSize >> 24) & 0xFF));
    raw.append((unsigned char)((decompressedSize >> 16) & 0xFF));
    raw.append((unsigned char)((decompressedSize >> 8) & 0xFF));
    raw.append((unsigned char)((decompressedSize >> 0) & 0xFF));
    
    QByteArray raw64 = parts.first().toUtf8();
    raw.append(QByteArray::fromBase64(raw64));
    decompressedBytes = qUncompress(raw);
    
    if (decompressedBytes.count() == 0) {
        emit statusMessage(tr("Firmware file has 0 length %1").arg(bytesKey));
        return false;
    }
    if (decompressedBytes.count() != decompressedSize) {
        emit statusMessage(tr("Size for decompressed %1 does not match stored size: Expected(%1) Actual(%2)").arg(decompressedSize).arg(decompressedBytes.count()));
        return false;
    }
    
    emit statusMessage(tr("Successfully decompressed %1").arg(bytesKey));
    
    return true;
}

uint16_t FirmwareImage::ihxBlockCount(void) const
{
    return _ihxBlocks.count();
}

bool FirmwareImage::ihxGetBlock(uint16_t index, uint16_t& address, QByteArray& bytes) const
{
    address = 0;
    bytes.clear();
    
    if (index < ihxBlockCount()) {
        address = _ihxBlocks[index].address;
        bytes = _ihxBlocks[index].bytes;
        return true;
    } else {
        return false;
    }
}

bool FirmwareImage::_binLoad(const QString& imageFilename)
{
    QFile binFile(imageFilename);
    if (!binFile.open(QIODevice::ReadOnly)) {
        emit statusMessage(tr("Unabled to open firmware file %1, %2").arg(imageFilename, binFile.errorString()));
        return false;
    }
    
    _imageSize = (uint32_t)binFile.size();
    
    binFile.close();
    
    _binFilename = imageFilename;
    
    return true;
}
