#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonObject>

#include "BaseClasses/VehicleTest.h"

namespace TestFixtures {
class LocalHttpTestServer;
}
class CompInfo;
class ComponentInformationManager;
class RequestMetaDataTypeStateMachine;

class RequestMetaDataTypeStateMachineTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _typeToStringReflectsRequestedType();
    void _requestCompleteEmittedForGeneral();
    void _requestCompleteEmittedForParameter();
    void _sequentialRequestsReuseMachine();
    void _requestCompletesForArduPilot();
    void _requestSkipsCompInfoOnHighLatencyLink();
    void _slowFtpDownloadAbortsEarly();
    UT_PARAMETERIZED_TEST(_metadataUriFillsItsField);
    void _downloadEndsWithItsState();
    void _httpDownloadEndsWithItsState();
    void _newCrcIsNotTakenFromTheHttpCache();
    UT_PARAMETERIZED_TEST(_downloadsUseTheDirectoryOfTheirManager);
    void _downloadWaitsForTheOneInProgress();
    UT_PARAMETERIZED_TEST(_transferIsHeldToTheSizeLimit);
    UT_PARAMETERIZED_TEST(_fallbackUriIsUsedWhenPrimaryFails);
    UT_PARAMETERIZED_TEST(_managerGoneEndsItsRequest);
    UT_PARAMETERIZED_TEST(_metadataIsTranslatedForLocale);
    void _translationDownloadEndsWithItsState();
    void _answerAfterRequestTimeoutIsNotUsed();
    UT_PARAMETERIZED_TEST(_metadataIsParsedOnWorker);
    UT_PARAMETERIZED_TEST(_downloadIsCachedOnceRead);

private:
    /// Gives the parameter metadata of the vehicle a file to download from @a server, and a translation summary there
    /// that names @a translationUrl for the locale that is translated for, compressed with gzip if @a compressed. The
    /// server also serves @a files. @a directory takes the general metadata.
    void _setUpTranslatedParameterMetadata(const QString& directory, TestFixtures::LocalHttpTestServer& server,
                                           const QString& translationUrl, bool compressed = false,
                                           QHash<QString, QByteArray> files = {});
    /// Gives the parameter metadata of the vehicle the URIs that @a parameterType, an entry of the metadata types of a
    /// general metadata file written into @a directory, has for it: a type has its fallback and its translation from
    /// there only
    void _setParameterUrisFromGeneralMetadata(const QString& directory, const QJsonObject& parameterType);
    /// A path on a test server of its own: the network cache of an earlier request must not answer for it
    static QString _uniqueServerPath(const QString& fileName);
    /// Requests @a compInfo with @a machine @return false if the request did not complete in time
    static bool _requestAndWait(RequestMetaDataTypeStateMachine& machine, CompInfo* compInfo);
    /// Tag of the metadata file of @a compInfo with @a crc in the file cache
    static QString _cacheTag(const CompInfo* compInfo, uint32_t crc);

    /// Manager of the vehicle that init() connects, once the requests of its initial connect have ended
    ComponentInformationManager* _manager = nullptr;
};
