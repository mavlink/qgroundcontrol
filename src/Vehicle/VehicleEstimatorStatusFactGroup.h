/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class Vehicle;

class VehicleEstimatorStatusFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleEstimatorStatusFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* goodAttitudeEstimate           READ goodAttitudeEstimate           CONSTANT)
    Q_PROPERTY(Fact* goodHorizVelEstimate           READ goodHorizVelEstimate           CONSTANT)
    Q_PROPERTY(Fact* goodVertVelEstimate            READ goodVertVelEstimate            CONSTANT)
    Q_PROPERTY(Fact* goodHorizPosRelEstimate        READ goodHorizPosRelEstimate        CONSTANT)
    Q_PROPERTY(Fact* goodHorizPosAbsEstimate        READ goodHorizPosAbsEstimate        CONSTANT)
    Q_PROPERTY(Fact* goodVertPosAbsEstimate         READ goodVertPosAbsEstimate         CONSTANT)
    Q_PROPERTY(Fact* goodVertPosAGLEstimate         READ goodVertPosAGLEstimate         CONSTANT)
    Q_PROPERTY(Fact* goodConstPosModeEstimate       READ goodConstPosModeEstimate       CONSTANT)
    Q_PROPERTY(Fact* goodPredHorizPosRelEstimate    READ goodPredHorizPosRelEstimate    CONSTANT)
    Q_PROPERTY(Fact* goodPredHorizPosAbsEstimate    READ goodPredHorizPosAbsEstimate    CONSTANT)
    Q_PROPERTY(Fact* gpsGlitch                      READ gpsGlitch                      CONSTANT)
    Q_PROPERTY(Fact* accelError                     READ accelError                     CONSTANT)
    Q_PROPERTY(Fact* velRatio                       READ velRatio                       CONSTANT)
    Q_PROPERTY(Fact* horizPosRatio                  READ horizPosRatio                  CONSTANT)
    Q_PROPERTY(Fact* vertPosRatio                   READ vertPosRatio                   CONSTANT)
    Q_PROPERTY(Fact* magRatio                       READ magRatio                       CONSTANT)
    Q_PROPERTY(Fact* haglRatio                      READ haglRatio                      CONSTANT)
    Q_PROPERTY(Fact* tasRatio                       READ tasRatio                       CONSTANT)
    Q_PROPERTY(Fact* horizPosAccuracy               READ horizPosAccuracy               CONSTANT)
    Q_PROPERTY(Fact* vertPosAccuracy                READ vertPosAccuracy                CONSTANT)

    Fact* goodAttitudeEstimate          () { return &_goodAttitudeEstimateFact; }
    Fact* goodHorizVelEstimate          () { return &_goodHorizVelEstimateFact; }
    Fact* goodVertVelEstimate           () { return &_goodVertVelEstimateFact; }
    Fact* goodHorizPosRelEstimate       () { return &_goodHorizPosRelEstimateFact; }
    Fact* goodHorizPosAbsEstimate       () { return &_goodHorizPosAbsEstimateFact; }
    Fact* goodVertPosAbsEstimate        () { return &_goodVertPosAbsEstimateFact; }
    Fact* goodVertPosAGLEstimate        () { return &_goodVertPosAGLEstimateFact; }
    Fact* goodConstPosModeEstimate      () { return &_goodConstPosModeEstimateFact; }
    Fact* goodPredHorizPosRelEstimate   () { return &_goodPredHorizPosRelEstimateFact; }
    Fact* goodPredHorizPosAbsEstimate   () { return &_goodPredHorizPosAbsEstimateFact; }
    Fact* gpsGlitch                     () { return &_gpsGlitchFact; }
    Fact* accelError                    () { return &_accelErrorFact; }
    Fact* velRatio                      () { return &_velRatioFact; }
    Fact* horizPosRatio                 () { return &_horizPosRatioFact; }
    Fact* vertPosRatio                  () { return &_vertPosRatioFact; }
    Fact* magRatio                      () { return &_magRatioFact; }
    Fact* haglRatio                     () { return &_haglRatioFact; }
    Fact* tasRatio                      () { return &_tasRatioFact; }
    Fact* horizPosAccuracy              () { return &_horizPosAccuracyFact; }
    Fact* vertPosAccuracy               () { return &_vertPosAccuracyFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static constexpr const char* _goodAttitudeEstimateFactName          = "goodAttitudeEsimate";
    static constexpr const char* _goodHorizVelEstimateFactName          = "goodHorizVelEstimate";
    static constexpr const char* _goodVertVelEstimateFactName           = "goodVertVelEstimate";
    static constexpr const char* _goodHorizPosRelEstimateFactName       = "goodHorizPosRelEstimate";
    static constexpr const char* _goodHorizPosAbsEstimateFactName       = "goodHorizPosAbsEstimate";
    static constexpr const char* _goodVertPosAbsEstimateFactName        = "goodVertPosAbsEstimate";
    static constexpr const char* _goodVertPosAGLEstimateFactName        = "goodVertPosAGLEstimate";
    static constexpr const char* _goodConstPosModeEstimateFactName      = "goodConstPosModeEstimate";
    static constexpr const char* _goodPredHorizPosRelEstimateFactName   = "goodPredHorizPosRelEstimate";
    static constexpr const char* _goodPredHorizPosAbsEstimateFactName   = "goodPredHorizPosAbsEstimate";
    static constexpr const char* _gpsGlitchFactName                     = "gpsGlitch";
    static constexpr const char* _accelErrorFactName                    = "accelError";
    static constexpr const char* _velRatioFactName                      = "velRatio";
    static constexpr const char* _horizPosRatioFactName                 = "horizPosRatio";
    static constexpr const char* _vertPosRatioFactName                  = "vertPosRatio";
    static constexpr const char* _magRatioFactName                      = "magRatio";
    static constexpr const char* _haglRatioFactName                     = "haglRatio";
    static constexpr const char* _tasRatioFactName                      = "tasRatio";
    static constexpr const char* _horizPosAccuracyFactName              = "horizPosAccuracy";
    static constexpr const char* _vertPosAccuracyFactName               = "vertPosAccuracy";

private:
    Fact _goodAttitudeEstimateFact;
    Fact _goodHorizVelEstimateFact;
    Fact _goodVertVelEstimateFact;
    Fact _goodHorizPosRelEstimateFact;
    Fact _goodHorizPosAbsEstimateFact;
    Fact _goodVertPosAbsEstimateFact;
    Fact _goodVertPosAGLEstimateFact;
    Fact _goodConstPosModeEstimateFact;
    Fact _goodPredHorizPosRelEstimateFact;
    Fact _goodPredHorizPosAbsEstimateFact;
    Fact _gpsGlitchFact;
    Fact _accelErrorFact;
    Fact _velRatioFact;
    Fact _horizPosRatioFact;
    Fact _vertPosRatioFact;
    Fact _magRatioFact;
    Fact _haglRatioFact;
    Fact _tasRatioFact;
    Fact _horizPosAccuracyFact;
    Fact _vertPosAccuracyFact;
};
