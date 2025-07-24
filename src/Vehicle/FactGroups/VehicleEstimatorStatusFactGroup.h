/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

class VehicleEstimatorStatusFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *goodAttitudeEstimate           READ goodAttitudeEstimate           CONSTANT)
    Q_PROPERTY(Fact *goodHorizVelEstimate           READ goodHorizVelEstimate           CONSTANT)
    Q_PROPERTY(Fact *goodVertVelEstimate            READ goodVertVelEstimate            CONSTANT)
    Q_PROPERTY(Fact *goodHorizPosRelEstimate        READ goodHorizPosRelEstimate        CONSTANT)
    Q_PROPERTY(Fact *goodHorizPosAbsEstimate        READ goodHorizPosAbsEstimate        CONSTANT)
    Q_PROPERTY(Fact *goodVertPosAbsEstimate         READ goodVertPosAbsEstimate         CONSTANT)
    Q_PROPERTY(Fact *goodVertPosAGLEstimate         READ goodVertPosAGLEstimate         CONSTANT)
    Q_PROPERTY(Fact *goodConstPosModeEstimate       READ goodConstPosModeEstimate       CONSTANT)
    Q_PROPERTY(Fact *goodPredHorizPosRelEstimate    READ goodPredHorizPosRelEstimate    CONSTANT)
    Q_PROPERTY(Fact *goodPredHorizPosAbsEstimate    READ goodPredHorizPosAbsEstimate    CONSTANT)
    Q_PROPERTY(Fact *gpsGlitch                      READ gpsGlitch                      CONSTANT)
    Q_PROPERTY(Fact *accelError                     READ accelError                     CONSTANT)
    Q_PROPERTY(Fact *velRatio                       READ velRatio                       CONSTANT)
    Q_PROPERTY(Fact *horizPosRatio                  READ horizPosRatio                  CONSTANT)
    Q_PROPERTY(Fact *vertPosRatio                   READ vertPosRatio                   CONSTANT)
    Q_PROPERTY(Fact *magRatio                       READ magRatio                       CONSTANT)
    Q_PROPERTY(Fact *haglRatio                      READ haglRatio                      CONSTANT)
    Q_PROPERTY(Fact *tasRatio                       READ tasRatio                       CONSTANT)
    Q_PROPERTY(Fact *horizPosAccuracy               READ horizPosAccuracy               CONSTANT)
    Q_PROPERTY(Fact *vertPosAccuracy                READ vertPosAccuracy                CONSTANT)

public:
    explicit VehicleEstimatorStatusFactGroup(QObject *parent = nullptr);

    Fact *goodAttitudeEstimate() { return &_goodAttitudeEstimateFact; }
    Fact *goodHorizVelEstimate() { return &_goodHorizVelEstimateFact; }
    Fact *goodVertVelEstimate() { return &_goodVertVelEstimateFact; }
    Fact *goodHorizPosRelEstimate() { return &_goodHorizPosRelEstimateFact; }
    Fact *goodHorizPosAbsEstimate() { return &_goodHorizPosAbsEstimateFact; }
    Fact *goodVertPosAbsEstimate() { return &_goodVertPosAbsEstimateFact; }
    Fact *goodVertPosAGLEstimate() { return &_goodVertPosAGLEstimateFact; }
    Fact *goodConstPosModeEstimate() { return &_goodConstPosModeEstimateFact; }
    Fact *goodPredHorizPosRelEstimate() { return &_goodPredHorizPosRelEstimateFact; }
    Fact *goodPredHorizPosAbsEstimate() { return &_goodPredHorizPosAbsEstimateFact; }
    Fact *gpsGlitch() { return &_gpsGlitchFact; }
    Fact *accelError() { return &_accelErrorFact; }
    Fact *velRatio() { return &_velRatioFact; }
    Fact *horizPosRatio() { return &_horizPosRatioFact; }
    Fact *vertPosRatio() { return &_vertPosRatioFact; }
    Fact *magRatio() { return &_magRatioFact; }
    Fact *haglRatio() { return &_haglRatioFact; }
    Fact *tasRatio() { return &_tasRatioFact; }
    Fact *horizPosAccuracy() { return &_horizPosAccuracyFact; }
    Fact *vertPosAccuracy() { return &_vertPosAccuracyFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    Fact _goodAttitudeEstimateFact = Fact(0, QStringLiteral("goodAttitudeEsimate"), FactMetaData::valueTypeBool);
    Fact _goodHorizVelEstimateFact = Fact(0, QStringLiteral("goodHorizVelEstimate"), FactMetaData::valueTypeBool);
    Fact _goodVertVelEstimateFact = Fact(0, QStringLiteral("goodVertVelEstimate"), FactMetaData::valueTypeBool);
    Fact _goodHorizPosRelEstimateFact = Fact(0, QStringLiteral("goodHorizPosRelEstimate"), FactMetaData::valueTypeBool);
    Fact _goodHorizPosAbsEstimateFact = Fact(0, QStringLiteral("goodHorizPosAbsEstimate"), FactMetaData::valueTypeBool);
    Fact _goodVertPosAbsEstimateFact = Fact(0, QStringLiteral("goodVertPosAbsEstimate"), FactMetaData::valueTypeBool);
    Fact _goodVertPosAGLEstimateFact = Fact(0, QStringLiteral("goodVertPosAGLEstimate"), FactMetaData::valueTypeBool);
    Fact _goodConstPosModeEstimateFact = Fact(0, QStringLiteral("goodConstPosModeEstimate"), FactMetaData::valueTypeBool);
    Fact _goodPredHorizPosRelEstimateFact = Fact(0, QStringLiteral("goodPredHorizPosRelEstimate"), FactMetaData::valueTypeBool);
    Fact _goodPredHorizPosAbsEstimateFact = Fact(0, QStringLiteral("goodPredHorizPosAbsEstimate"), FactMetaData::valueTypeBool);
    Fact _gpsGlitchFact = Fact(0, QStringLiteral("gpsGlitch"), FactMetaData::valueTypeBool);
    Fact _accelErrorFact = Fact(0, QStringLiteral("accelError"), FactMetaData::valueTypeBool);
    Fact _velRatioFact = Fact(0, QStringLiteral("velRatio"), FactMetaData::valueTypeFloat);
    Fact _horizPosRatioFact = Fact(0, QStringLiteral("horizPosRatio"), FactMetaData::valueTypeFloat);
    Fact _vertPosRatioFact = Fact(0, QStringLiteral("vertPosRatio"), FactMetaData::valueTypeFloat);
    Fact _magRatioFact = Fact(0, QStringLiteral("magRatio"), FactMetaData::valueTypeFloat);
    Fact _haglRatioFact = Fact(0, QStringLiteral("haglRatio"), FactMetaData::valueTypeFloat);
    Fact _tasRatioFact = Fact(0, QStringLiteral("tasRatio"), FactMetaData::valueTypeFloat);
    Fact _horizPosAccuracyFact = Fact(0, QStringLiteral("horizPosAccuracy"), FactMetaData::valueTypeFloat);
    Fact _vertPosAccuracyFact = Fact(0, QStringLiteral("vertPosAccuracy"), FactMetaData::valueTypeFloat);
};
