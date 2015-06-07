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
///     @brief Support for Intel Hex firmware file
///     @author Don Gagne <don@thegagnes.com>

#include "IntelHexFirmware.h"

#include <QFile>

IntelHexFirmware::IntelHexFirmware(QObject* parent) :
    QObject(parent)
{
    
}

bool IntelHexFirmware::loadFromFile(const QString& ihxFilename)
{
    _blockMap.clear();
    
    QFile ihxFile(ihxFilename);
    if (!ihxFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _errorString = QString("Unable to open firmware file %1, error: %2").arg(ihxFilename).arg(ihxFile.errorString());
        return false;
    }
    
    QTextStream stream(&ihxFile);
    
    while (true) {
        if (stream.read(1) != ":") {
            _errorString = "Incorrectly formatted .ihx file, line does not begin with :";
            return false;
        }
        
        uint8_t     byteCount;
        uint16_t    address;
        uint8_t     recordType;
        QByteArray  bytes;
        uint8_t     crc;
        
        if (!_readByteFromStream(stream, byteCount) ||
            !_readWordFromStream(stream, address) ||
            !_readByteFromStream(stream, recordType) ||
            !_readBytesFromStream(stream, byteCount, bytes) ||
            !_readByteFromStream(stream, crc)) {
            _errorString = "Incorrectly formatted line in .ihx file, line too short";
            return false;
        }
        
        if (!(recordType == 0 || recordType == 1)) {
            _errorString = QString("Unsupported record type in file: %1").arg(recordType);
            return false;
        }
        
        if (recordType == 0) {
            _blockMap[address] = bytes;
        } else if (recordType == 1) {
            // EOF
            break;
        }
        
        // Move to next line
        stream.readLine();
    }
    
    ihxFile.close();
    
    return true;
}

uint16_t IntelHexFirmware::blockCount(void)
{
    return _blockMap.keys.count();
}

bool IntelHexFirmware::getBlock(uint16_t index, uint16_t& address, QByteArray& bytes)
{
    address = 0;
    bytes.clear();
    
    if (index < blockCount()) {
        bytes = _blockMap[_blockMap.keys[index]];
        return true;
    } else {
        return false;
    }
}