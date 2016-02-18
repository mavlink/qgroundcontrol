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

#ifndef ValuesWidgetController_H
#define ValuesWidgetController_H

#include <QObject>

class ValuesWidgetController : public QObject
{
    Q_OBJECT
    
public:
    ValuesWidgetController(void);

    Q_PROPERTY(QStringList largeValues READ largeValues WRITE setLargeValues NOTIFY largeValuesChanged)
    Q_PROPERTY(QStringList smallValues READ smallValues WRITE setSmallValues NOTIFY smallValuesChanged)

    Q_PROPERTY(QStringList altitudeProperties READ altitudeProperties CONSTANT)

    QStringList largeValues(void) const { return _largeValues; }
    QStringList smallValues(void) const { return _smallValues; }
    void setLargeValues(const QStringList& values);
    void setSmallValues(const QStringList& values);
    QStringList altitudeProperties(void) const { return _altitudeProperties; }

signals:
    void largeValuesChanged(QStringList values);
    void smallValuesChanged(QStringList values);

private:
    QStringList _largeValues;
    QStringList _smallValues;
    QStringList _altitudeProperties;

    static const char* _groupKey;
    static const char* _largeValuesKey;
    static const char* _smallValuesKey;
};

#endif
