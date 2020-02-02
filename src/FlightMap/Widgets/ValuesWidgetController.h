/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
