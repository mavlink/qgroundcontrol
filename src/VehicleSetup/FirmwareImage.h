/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FirmwareImage_H
#define FirmwareImage_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QTextStream>

#include <stdint.h>

/// Support for Intel Hex firmware file
class FirmwareImage : public QObject
{
    Q_OBJECT
    
public:
    FirmwareImage(QObject *parent = 0);
    
    /// Loads the specified firmware file. Supported formats: .px4, .bin, .ihx.
    /// Emits errorMesssage and statusMessage signals while loading.
    /// @return true: success, false: failure
    bool load(const QString& firmwareFilename);
    
    bool firmwareIsBinFormat(void) { return _binFormat; }
    
    QString binFilename(void) { return _binFilename; }
    uint16_t ihxBlockCount(void) const;
    uint32_t ihxByteCount(void) const { return _ihxByteCount; }
    bool ihxGetBlock(uint16_t index, uint16_t& address, QByteArray& bytes) const;
    
signals:
    void errorMessage(const QString& errorString);
    void statusMessage(const QString& warningtring);
    
private:
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

    bool                        _binFormat;
    QString                     _binFilename;
    QMap<uint16_t, QByteArray>  _ihxBlockMap;
    uint32_t                    _ihxByteCount;
};

#endif
