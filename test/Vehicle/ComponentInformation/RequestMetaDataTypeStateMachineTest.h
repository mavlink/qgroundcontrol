#pragma once

#include "BaseClasses/VehicleTest.h"

class RequestMetaDataTypeStateMachineTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _typeToStringReflectsRequestedType();
    void _requestCompleteEmittedForGeneral();
    void _requestCompleteEmittedForParameter();
    void _sequentialRequestsReuseMachine();
    void _requestCompletesForArduPilot();
    void _requestSkipsCompInfoOnHighLatencyLink();
    void _requestUsesCachedMetadataForParameter();
    void _slowFtpDownloadAbortsEarly();
    UT_PARAMETERIZED_TEST(_metadataUriFillsItsField);
    void _downloadEndsWithItsState();
    void _httpDownloadEndsWithItsState();
    void _downloadThatCannotStartLeavesTheOneInProgress();
    UT_PARAMETERIZED_TEST(_metadataIsTranslatedForSystemLocale);
    void _translationDownloadEndsWithItsState();
    void _answerAfterRequestTimeoutIsNotUsed();

private:
    /// Gives the parameter metadata of the vehicle a file to download from @a directory, and a translation summary
    /// that names @a translationUrl for the system locale
    void _setUpTranslatedParameterMetadata(const QString& directory, const QString& translationUrl);
};
