/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef SerialPortIds_H
#define SerialPortIds_H

// SerialPortInfo Vendor and Product Ids for known boards
class SerialPortIds {

public:
    static const int px4VendorId =                          9900;   ///< Vendor ID for Pixhawk board (V2 and V1) and PX4 Flow

    static const int pixhawkFMUV2ProductId =                17;     ///< Product ID for Pixhawk V2 board
    static const int pixhawkFMUV2OldBootloaderProductId =   22;     ///< Product ID for Bootloader on older Pixhawk V2 boards
    static const int pixhawkFMUV1ProductId =                16;     ///< Product ID for PX4 FMU V1 board

    static const int AeroCoreProductId =                    4097;   ///< Product ID for the AeroCore board
    
    static const int px4FlowProductId =                     21;     ///< Product ID for PX4 Flow board

    static const int threeDRRadioVendorId =                 1027;   ///< Vendor ID for 3DR Radio
    static const int threeDRRadioProductId =                24597;  ///< Product ID for 3DR Radio
};

#endif
