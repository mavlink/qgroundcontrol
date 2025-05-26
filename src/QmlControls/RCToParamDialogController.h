/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(RCToParamDialogControllerLog)

class Fact;
class FactMetaData;

class RCToParamDialogController : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    Q_MOC_INCLUDE("Fact.h")
    Q_PROPERTY(Fact *tuningFact READ tuningFact WRITE setTuningFact NOTIFY tuningFactChanged)
    Q_PROPERTY(Fact *scale      READ scale                          CONSTANT)
    Q_PROPERTY(Fact *center     READ center                         CONSTANT)
    Q_PROPERTY(Fact *min        READ min                            CONSTANT)
    Q_PROPERTY(Fact *max        READ max                            CONSTANT)
    Q_PROPERTY(bool  ready      MEMBER _ready                       NOTIFY readyChanged) // true: editing can begin, false: still waiting for param update from vehicle

public:
    explicit RCToParamDialogController(QObject *parent = nullptr);
    ~RCToParamDialogController();

    Fact *tuningFact() { return _tuningFact; }
    Fact *scale() { return _scaleFact; }
    Fact *center() { return _centerFact; }
    Fact *min() { return _minFact; }
    Fact *max() { return _maxFact; }
    void setTuningFact(Fact *tuningFact);

signals:
    void tuningFactChanged(Fact *fact);
    void readyChanged(bool ready);

private slots:
    void _parameterUpdated();

private:
    Fact *_tuningFact = nullptr;
    Fact *_scaleFact = nullptr;
    Fact *_centerFact = nullptr;
    Fact *_minFact = nullptr;
    Fact *_maxFact = nullptr;
    bool _ready = false;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static constexpr const char *_scaleFactName = "Scale";
    static constexpr const char *_centerFactName = "CenterValue";
    static constexpr const char *_minFactName = "MinValue";
    static constexpr const char *_maxFactName = "MaxValue";
};
