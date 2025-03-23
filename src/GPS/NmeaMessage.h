//
// Created by zdanek on 13.11.24.
//

#pragma once

#include <QGeoCoordinate>
#include <QString>

class NmeaMessage {
public:
  NmeaMessage(QGeoCoordinate coordinate);
  QString getGGA();

private:
  QString _getCheckSum(QString line);
  QGeoCoordinate _coordinate;
};


