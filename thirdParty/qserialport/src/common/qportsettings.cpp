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
 */

#include <QDebug>
#include <QStringList>
#include "qportsettings.h"

namespace TNX {

/*!
  Constructs a QPortSettings object with default values.
*/

QPortSettings::QPortSettings()
{
    setBaudRate(BAUDR_9600);
    dataBits_ = DB_8;
    parity_ = PAR_NONE;
    stopBits_ = STOP_1;
    flowControl_ = FLOW_OFF;
}

/*!
  Constructs a QPortSettings object with the given \a settings.
*/

QPortSettings::QPortSettings(const QString &settings)
{
    set(settings);
}

bool QPortSettings::set(const QString &settings)
{
    setBaudRate(BAUDR_9600);
    dataBits_ = DB_8;
    parity_ = PAR_NONE;
    stopBits_ = STOP_1;
    flowControl_ = FLOW_OFF;

    QStringList list = settings.split(",", QString::SkipEmptyParts);
    bool ok = true;
    for (int i = 0; i < list.size(); i++) {
        switch (i) {
        case 0:
            setBaudRate(baudRateFromInt(list.at(i).toInt(&ok), ok));
            if ( !ok ) {
                qWarning() << QString("QPortSettings::QPortSettings(%1) " \
                                      "failed when setting baud rate [value: %2]; using default value")
                              .arg(settings)
                              .arg(list.at(i));
            }
            break;
        case 1:
            dataBits_ = dataBitsFromString(list.at(i), ok);
            if ( !ok ) {
                qWarning() << QString("QPortSettings::QPortSettings(%1) " \
                                      "failed when setting data bits [value: %2]; using default value")
                              .arg(settings)
                              .arg(list.at(i));
            }
            break;
        case 2:
            parity_ = parityFromString(list.at(i), ok);
            if ( !ok ) {
                qWarning() << QString("QPortSettings::QPortSettings(%1) " \
                                      "failed when setting parity [value: %2]; using default value")
                              .arg(settings)
                              .arg(list.at(i));
            }
            break;
        case 3:
            stopBits_ = stopBitsFromString(list.at(i), ok);
            if ( !ok ) {
                qWarning() << QString("QPortSettings::QPortSettings(%1) " \
                                      "failed when setting stop bits [value: %2]; using default value")
                              .arg(settings)
                              .arg(list.at(i));
            }
            break;
        case 4:
            flowControl_ = flowControlFromString(list.at(i), ok);
            if ( !ok ) {
                qWarning() << QString("QPortSettings::QPortSettings(%1) " \
                                      "failed when setting flow control type [value: %2]; using default value")
                              .arg(settings)
                              .arg(list.at(i));
            }
            break;
        default:
            break;
        } //switch
    } //for

    return ok;
}

/*!

*/
QPortSettings::BaudRate QPortSettings::baudRateFromInt(int baud, bool &ok)
{
    ok = true;

    switch ( baud ) {
#ifdef TNX_POSIX_SERIAL_PORT
    case 50:
        return BAUDR_50;
    case 75:
        return BAUDR_75;
    case 134:
        return BAUDR_134;
    case 150:
        return BAUDR_150;
    case 200:
        return BAUDR_200;
    case 1800:
        return BAUDR_1800;
        //case 76800:
        //  return BAUDR_76800;
#endif
#ifdef TNX_WINDOWS_SERIAL_PORT
    case 14400:
        return BAUDR_14400;
    case 56000:
        return BAUDR_56000;
    case 128000:
        return BAUDR_128000;
    case 256000:
        return BAUDR_256000;
#endif
#if defined(Q_OS_LINUX)
    case 500000:
        return BAUDR_500000;
    case 576000:
        return BAUDR_576000;
#endif
        // baud rates supported by all platforms
    case 110:
        return BAUDR_110;
    case 300:
        return BAUDR_300;
    case 600:
        return BAUDR_600;
    case 1200:
        return BAUDR_1200;
    case 2400:
        return BAUDR_2400;
    case 4800:
        return BAUDR_4800;
    case 9600:
        return BAUDR_9600;
    case 19200:
        return BAUDR_19200;
    case 38400:
        return BAUDR_38400;
    case 57600:
        return BAUDR_57600;
    case 115200:
        return BAUDR_115200;
    case 230400:
        return BAUDR_230400;
    case 460800:
        return BAUDR_460800;
    case 921600:
        return BAUDR_921600;
    default:
        ok = false;
        return BAUDR_9600;
    }
}

/*!

*/
QPortSettings::DataBits QPortSettings::dataBitsFromString(const QString &dataBits, bool &ok)
{
    ok = true;
    if ( dataBits.trimmed() == "5" )
        return DB_5;
    else if ( dataBits.trimmed() == "6" )
        return DB_6;
    else if ( dataBits.trimmed() == "7" )
        return DB_7;
    else if ( dataBits.trimmed() == "8")
        return DB_8;
    else {
        ok = false;
        return DB_8;
    }
}

/*!

*/
QPortSettings::Parity QPortSettings::parityFromString(const QString &parity, bool &ok)
{
    ok = true;
    if ( !QString::compare(parity.trimmed(), "n", Qt::CaseInsensitive) )
        return PAR_NONE;
    else if ( !QString::compare(parity.trimmed(), "o", Qt::CaseInsensitive) )
        return PAR_ODD;
    else if ( !QString::compare(parity.trimmed(), "e", Qt::CaseInsensitive) )
        return PAR_EVEN;
#ifdef TNX_WINDOWS_SERIAL_PORT
    else if ( !QString::compare(parity.trimmed(), "m", Qt::CaseInsensitive) )
        return PAR_MARK;
#endif
    else if ( !QString::compare(parity.trimmed(), "s", Qt::CaseInsensitive) )
        return PAR_SPACE;
    else {
        ok = false;
        return PAR_NONE;
    }
}

/*!

*/
QPortSettings::StopBits QPortSettings::stopBitsFromString(const QString &stopBits, bool &ok)
{
    ok = true;
    if ( stopBits.trimmed() == "1" )
        return STOP_1;
#ifdef TNX_WINDOWS_SERIAL_PORT
    else if ( stopBits.trimmed() == "1.5" )
        return STOP_1_5;
#endif
    else if ( stopBits.trimmed() == "2" )
        return STOP_2;
    else {
        ok = false;
        return STOP_1;
    }
}

/*!

*/
QPortSettings::FlowControl QPortSettings::flowControlFromString(const QString &flow, bool &ok)
{
    ok = true;
    if ( !QString::compare(flow.trimmed(), "off", Qt::CaseInsensitive) )
        return FLOW_OFF;
    else if ( !QString::compare(flow.trimmed(), "xon/xoff", Qt::CaseInsensitive) )
        return FLOW_XONXOFF;
    else if ( !QString::compare(flow.trimmed(), "hardware", Qt::CaseInsensitive) )
        return FLOW_HARDWARE;
    else {
        ok = false;
        return FLOW_OFF;
    }
}

/*!

*/
QString QPortSettings::toString() const
{
    QString txt;

    txt.setNum(baudRateInt_, 10);
    txt.append(",");

    if ( dataBits() == DB_5 )
        txt.append("5,");
    else if ( dataBits() == DB_6 )
        txt.append("6,");
    else if( dataBits() == DB_7 )
        txt.append("7,");
    else if ( dataBits() == DB_8 )
        txt.append("8,");

    if ( parity() == PAR_NONE )
        txt.append("N,");
    else if ( parity() == PAR_ODD )
        txt.append("O,");
    else if ( parity() == PAR_EVEN )
        txt.append("E,");
#ifdef TNX_WINDOWS_SERIAL_PORT
    else if ( parity() == PAR_MARK  )
        txt.append("M,");
#endif
    else if ( parity() == PAR_SPACE )
        txt.append("S,");

    if ( stopBits() == STOP_1 )
        txt.append("1,");
#ifdef TNX_WINDOWS_SERIAL_PORT
    else if ( stopBits() == STOP_1_5 )
        txt.append("1.5,");
#endif
    else if ( stopBits() == STOP_2 )
        txt.append("2,");

    if ( flowControl() == FLOW_OFF )
        txt.append("off");
    else if ( flowControl() == FLOW_XONXOFF )
        txt.append("xon/xoff");
    else if ( flowControl() == FLOW_HARDWARE )
        txt.append("hardware");

    return txt;
}

} // namespace


