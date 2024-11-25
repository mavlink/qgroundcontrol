//
// Created by zdanek on 13.11.24.
//

#include "NmeaMessage.h"

#include <QDateTime>
#include <QtCore>
#include <cmath>

NmeaMessage::NmeaMessage(QGeoCoordinate coordinate) {
  _coordinate = coordinate;
}

QString NmeaMessage::getGGA() {

  double lat = _coordinate.latitude();
  double lng = _coordinate.longitude();
  double alt = _coordinate.altitude();

  //  qCDebug(NTRIPLog) << "lat : " << lat << " lon : " << lng << " alt : " << alt;

  QString time = QDateTime::currentDateTimeUtc().toString("hhmmss.zzz");

  if (lat != 0 || lng != 0) {
    if (std::isnan(alt))
      alt = 0.0;

    int latDegrees = (int)qFabs(lat);
    double latMinutes = (qFabs(lat) - latDegrees) * 60.0;

    int lngDegrees = (int)qFabs(lng);
    double lngMinutes = (qFabs(lng) - lngDegrees) * 60.0;

    // Format latitude degrees and minutes with leading zeros
    QString latDegreesStr = QString("%1").arg(latDegrees, 2, 10, QChar('0'));
    QString latMinutesStr =
        QString("%1").arg(latMinutes, 7, 'f', 4, QChar('0'));
    QString latStr = latDegreesStr + latMinutesStr;

    // Format longitude degrees and minutes with leading zeros
    QString lngDegreesStr = QString("%1").arg(lngDegrees, 3, 10, QChar('0'));
    QString lngMinutesStr =
        QString("%1").arg(lngMinutes, 7, 'f', 4, QChar('0'));
    QString lngStr = lngDegreesStr + lngMinutesStr;

    QString line =
        QString("$GPGGA,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14")
            .arg(time)                // %1 - UTC Time
            .arg(latStr)              // %2 - Latitude in NMEA format
            .arg(lat < 0 ? "S" : "N") // %3 - N/S Indicator
            .arg(lngStr)              // %4 - Longitude in NMEA format
            .arg(lng < 0 ? "W" : "E") // %5 - E/W Indicator
            .arg("1")                 // %6 - Fix Quality
            .arg("10")                // %7 - Number of Satellites
            .arg("1.0") // %8 - Horizontal Dilution of Precision (HDOP)
            .arg(QString::number(alt, 'f', 2)) // %9 - Altitude
            .arg("M")                          // %10 - Altitude Units
            .arg("0")                          // %11 - Geoidal Separation
            .arg("M")                          // %12 - Geoidal Separation Units
            .arg("0.0") // %13 - Age of Differential GPS Data
            .arg("0");  // %14 - Differential Reference Station ID

    // Calculate checksum and send message
    QString checkSum = _getCheckSum(line);
    QString nmeaMessage = line + "*" + checkSum + "\r\n";

    return nmeaMessage;
  }
  return QString();
}

QString NmeaMessage::_getCheckSum(QString line) {
//  qCDebug(NTRIPLog) << "Calculating checksum";
  QByteArray temp_Byte = line.toUtf8();
  const char* buf = temp_Byte.constData();

  char character;
  int checksum = 0;

  // Start from index 1 to skip the '$' character
  for(int i = 1; i < line.length(); i++) {
    character = buf[i];
    if(character == '*') {
      break;
    }
    checksum ^= character;
  }

  // Ensure the checksum is a two-digit uppercase hexadecimal string
  return QString("%1").arg(checksum, 2, 16, QChar('0')).toUpper();
}