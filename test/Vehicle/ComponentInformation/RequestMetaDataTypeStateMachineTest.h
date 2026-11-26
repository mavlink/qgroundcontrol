#pragma once

#include <QtCore/QHash>

#include "BaseClasses/VehicleTest.h"

namespace TestFixtures {
class LocalHttpTestServer;
}

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
    UT_PARAMETERIZED_TEST(_downloadsUseTheDirectoryOfTheirManager);
    void _downloadWaitsForTheOneInProgress();
    UT_PARAMETERIZED_TEST(_metadataIsTranslatedForSystemLocale);
    void _translationDownloadEndsWithItsState();
    void _answerAfterRequestTimeoutIsNotUsed();
    UT_PARAMETERIZED_TEST(_metadataIsParsedOnWorker);

private:
    /// Gives the parameter metadata of the vehicle a file to download from @a server, and a translation summary there
    /// that names @a translationUrl for the system locale, compressed with gzip if @a compressed. The server also
    /// serves @a files. @a directory takes the general metadata.
    void _setUpTranslatedParameterMetadata(const QString& directory, TestFixtures::LocalHttpTestServer& server,
                                           const QString& translationUrl, bool compressed = false,
                                           QHash<QString, QByteArray> files = {});
    /// A path on a test server of its own: the network cache of an earlier request must not answer for it
    static QString _uniqueServerPath(const QString& fileName);
};
