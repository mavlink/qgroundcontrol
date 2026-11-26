#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>

#include "UnitTest.h"

namespace TestFixtures {
class LocalHttpTestServer;
}

class ComponentInformationTranslationTest : public UnitTest
{
    Q_OBJECT

public:
    ComponentInformationTranslationTest() = default;
    virtual ~ComponentInformationTranslationTest() = default;

private slots:
    UT_PARAMETERIZED_TEST(_basic_test);
    void _downloadAndTranslateFromSummary_test();
    UT_PARAMETERIZED_TEST(_summaryWithoutTranslation_test);
    void _onDownloadCompletedFailurePropagatesError_test();
    void _onDownloadCompletedMissingTsPropagatesError_test();
    void _summaryIsReadOnWorker_test();
    UT_PARAMETERIZED_TEST(_translationOnWorkerEndsWithItsRequest_test);
    UT_PARAMETERIZED_TEST(_translationUrlMustBeHttp_test);

private:
    void readJson(const QByteArray& bytes, QJsonDocument& jsonDoc);
    /// Writes a translation summary that names @p url for @p locale
    static bool writeSummary(const QString& path, const QString& locale, const QString& url);
    /// Translated files in the temporary directory
    static QStringList translatedFiles();
    /// Serves the test translation on @p server @return Its URL
    static QString serveTranslation(TestFixtures::LocalHttpTestServer& server);
};
