/*
 * Unofficial Qt Serial Port Library
 *
 * Copyright (c) 2010 Inbiza Systems Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * author labs@inbiza.com
 * @brief Declares QPortSettings class that is required to set the port attributes.
 */

#ifndef TNX_QSERIALPORTSETTINGS_H__
#define TNX_QSERIALPORTSETTINGS_H__

#include <QString>
#include "qserialport_export.h"

#ifdef TNX_POSIX_SERIAL_PORT
#undef TNX_POSIX_SERIAL_PORT
#endif
#ifdef TNX_WINDOWS_SERIAL_PORT
#undef TNX_WINDOWS_SERIAL_PORT
#endif

#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
#define TNX_POSIX_SERIAL_PORT
#endif
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
#include <windows.h>
#define TNX_WINDOWS_SERIAL_PORT
#endif

namespace TNX {

enum ChangeApplyTypes { AllAppTy, PortAttrOnlyAppTy, CommTimeoutsOnlyAppTy };

/**
 * Communication timeout values for Win32/CE and Posix platforms.
 * @see www.unixwiz.net/techtips/termios-vmin-vtime.html
 */
struct CommTimeouts {
#ifdef TNX_WINDOWS_SERIAL_PORT
    typedef DWORD commt_t;
    static const DWORD NoTimeout = MAXDWORD;
#else
    typedef quint8 commt_t;
    static const qint8 NoTimeout = -1;
#endif

    // Win32 only section
    commt_t Win32ReadIntervalTimeout;          ///< Maximum time between read chars. Win32 only.
    commt_t Win32ReadTotalTimeoutMultiplier;   ///< Multiplier of characters. Win32 only.
    commt_t Win32ReadTotalTimeoutConstant;     ///< Constant in milliseconds. Win32 only.
    commt_t Win32WriteTotalTimeoutMultiplier;  ///< Multiplier of characters. Win32 only.
    commt_t Win32WriteTotalTimeoutConstant;    ///< Constant in milliseconds. Win32 only.

    // Posix only section
    commt_t PosixVTIME;                         ///< Read timeout. Posix only.
    commt_t PosixVMIN;                          ///< Minimum number of bytes before returning from
    ///< read operation. Posix only.
    CommTimeouts()
        : Win32ReadIntervalTimeout(NoTimeout), Win32ReadTotalTimeoutMultiplier(0), Win32ReadTotalTimeoutConstant(0),
          Win32WriteTotalTimeoutMultiplier(25), Win32WriteTotalTimeoutConstant(250),
          PosixVTIME(0), PosixVMIN(1)
    {
    }
};

/**
 * Wrapper class for serial port settings.
 */
class TONIX_EXPORT QPortSettings
{
public:
    enum BaudRate {
        BAUDR_UNKNOWN = 0,
#ifdef TNX_POSIX_SERIAL_PORT
        BAUDR_50,
        BAUDR_75,
        BAUDR_134,
        BAUDR_150,
        BAUDR_200,
        BAUDR_1800,
        /* BAUDR_76800, */
#endif
#ifdef Q_OS_LINUX
//        BAUDR_500000,
//        BAUDR_576000,
#endif
#ifdef TNX_WINDOWS_SERIAL_PORT
        BAUDR_14400,
        BAUDR_56000,
        BAUDR_128000,
        BAUDR_256000,
#endif
        /* baud rates supported by all OSs */
        BAUDR_110,
        BAUDR_300,
        BAUDR_600,
        BAUDR_1200,
        BAUDR_2400,
        BAUDR_4800,
        BAUDR_9600,
        BAUDR_19200,
        BAUDR_38400,
        BAUDR_57600,
        BAUDR_115200,
        BAUDR_230400,
        BAUDR_460800,
        BAUDR_500000,
        BAUDR_576000,
        BAUDR_921600
    };

    enum DataBits {
        DB_5,   // simulated in POSIX
        DB_6,
        DB_7,
        DB_8,
        DB_UNKNOWN
    };

    enum Parity {
        PAR_NONE,
        PAR_ODD,
        PAR_EVEN,
#ifdef TNX_WINDOWS_SERIAL_PORT
        PAR_MARK,
#endif
        PAR_SPACE,   // simulated in POSIX
        PAR_UNKNOWN
    };

    enum StopBits {
        STOP_1,
#ifdef TNX_WINDOWS_SERIAL_PORT
        STOP_1_5,
#endif
        STOP_2,
        STOP_UNKNOWN
    };

    enum FlowControl {
        FLOW_OFF,
        FLOW_HARDWARE,
        FLOW_XONXOFF,
        FLOW_UNKNOWN
    };

    QPortSettings();
    QPortSettings(const QString &settings);

    // port configuration methods

    bool set(const QString &settings);

    inline BaudRate baudRate() const {
        return baudRate_;
    }
    void setBaudRate(BaudRate baudRate) {
        baudRate_ = baudRate;

        switch ( baudRate_ ) {
#ifdef TNX_POSIX_SERIAL_PORT
        case BAUDR_50: baudRateInt_=50; break;
        case BAUDR_75: baudRateInt_=75; break;
        case BAUDR_134: baudRateInt_=134; break;
        case BAUDR_150: baudRateInt_=150; break;
        case BAUDR_200: baudRateInt_=200; break;
        case BAUDR_1800: baudRateInt_=1800; break;
            //case 76800: baudRateInt_=76800; break;
#endif
#ifdef TNX_WINDOWS_SERIAL_PORT
        case BAUDR_14400: baudRateInt_=14400; break;
        case BAUDR_56000: baudRateInt_=56000; break;
        case BAUDR_128000: baudRateInt_=128000; break;
        case BAUDR_256000: baudRateInt_=256000; break;
#endif
            // baud rates supported by all platforms
        case BAUDR_110: baudRateInt_=110; break;
        case BAUDR_300: baudRateInt_=300; break;
        case BAUDR_600: baudRateInt_=600; break;
        case BAUDR_1200: baudRateInt_=1200; break;
        case BAUDR_2400: baudRateInt_=2400; break;
        case BAUDR_4800: baudRateInt_=4800; break;
        case BAUDR_9600: baudRateInt_=9600; break;
        case BAUDR_19200: baudRateInt_=19200; break;
        case BAUDR_38400: baudRateInt_=38400; break;
        case BAUDR_57600: baudRateInt_=57600; break;
        case BAUDR_115200: baudRateInt_=115200; break;
        case BAUDR_230400: baudRateInt_=230400; break;
        case BAUDR_460800: baudRateInt_=460800; break;
        case BAUDR_500000: baudRateInt_=500000; break;
        case BAUDR_576000: baudRateInt_=576000; break;
        case BAUDR_921600: baudRateInt_=921600; break;
        default:
            baudRateInt_ = 0; // unknown baudrate
        }
    }

    inline Parity parity() const {
        return parity_;
    }
    inline void setParity(Parity parity) {
        parity_ = parity;
    }

    inline StopBits stopBits() const {
        return stopBits_;
    }
    inline void setStopBits(StopBits stopBits) {
        stopBits_ = stopBits;
    }

    inline DataBits dataBits() const {
        return dataBits_;
    }
    inline void setDataBits(DataBits dataBits) {
        dataBits_ = dataBits;
    }

    inline FlowControl flowControl() const {
        return flowControl_;
    }
    inline void setFlowControl(FlowControl flowControl) {
        flowControl_ = flowControl;
    }

    QString toString() const;

    // helper methods to configure port settings
private:
    static BaudRate baudRateFromInt(int baud, bool &ok);
    static DataBits dataBitsFromString(const QString &databits, bool &ok);
    static Parity parityFromString(const QString &parity, bool &ok);
    static StopBits stopBitsFromString(const QString &stopbits, bool &ok);
    static FlowControl flowControlFromString(const QString &flow, bool &ok);

private:
    BaudRate baudRate_;
    DataBits dataBits_;
    Parity parity_;
    StopBits stopBits_;
    FlowControl flowControl_;
    qint32 baudRateInt_;
};

}

#endif // TNX_QSERIALPORTSETTINGS_H__
