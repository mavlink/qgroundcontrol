/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>

#include "FactSystem.h"

class RCToParamDialogController : public QObject
{
    Q_OBJECT
    
public:
    RCToParamDialogController(void);
    
    Q_PROPERTY(Fact* tuningFact READ tuningFact WRITE setTuningFact NOTIFY tuningFactChanged)
    Q_PROPERTY(bool  ready      MEMBER _ready                       NOTIFY readyChanged)        // true: editing can begin, false: still waiting for param update from vehicle
    Q_PROPERTY(Fact* scale      READ scale                          CONSTANT)
    Q_PROPERTY(Fact* center     READ center                         CONSTANT)
    Q_PROPERTY(Fact* min        READ min                            CONSTANT)
    Q_PROPERTY(Fact* max        READ max                            CONSTANT)

    Fact* tuningFact    (void) { return _tuningFact; }
    Fact* scale         (void) { return &_scaleFact; }
    Fact* center        (void) { return &_centerFact; }
    Fact* min           (void) { return &_minFact; }
    Fact* max           (void) { return &_maxFact; }
    void  setTuningFact (Fact* tuningFact);

signals:
    void tuningFactChanged  (Fact* fact);
    void readyChanged       (bool ready);

private slots:
    void _parameterUpdated(void);

private:
    static QMap<QString, FactMetaData*> _metaDataMap;

    Fact* _tuningFact = nullptr;
    bool _ready =       false;
    Fact _scaleFact;
    Fact _centerFact;
    Fact _minFact;
    Fact _maxFact;

    static const char*  _scaleFactName;
    static const char*  _centerFactName;
    static const char*  _minFactName;
    static const char*  _maxFactName;
};
