#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>

#include "TempDirectoryTest.h"

class ComponentInformationTranslationTest : public TempDirectoryTest
{
    Q_OBJECT

public:
    ComponentInformationTranslationTest() = default;
    virtual ~ComponentInformationTranslationTest() = default;

private slots:
    void _basic_test();
    void _downloadAndTranslateFromSummary_test();
    void _downloadAndTranslateMissingLocale_test();
    void _downloadAndTranslateMissingUrl_test();
    void _downloadAndTranslateInvalidSummaryJson_test();
    void _onDownloadCompletedFailurePropagatesError_test();
    void _onDownloadCompletedMissingTsPropagatesError_test();

private:
    void readJson(const QByteArray& bytes, QJsonDocument& jsonDoc);
};
