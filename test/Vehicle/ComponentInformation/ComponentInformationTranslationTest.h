#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>

#include "UnitTest.h"

class ComponentInformationTranslationTest : public UnitTest
{
    Q_OBJECT

public:
    ComponentInformationTranslationTest() = default;
    virtual ~ComponentInformationTranslationTest() = default;

private slots:
    void _basic_test();
    void _downloadAndTranslateFromSummary_test();
    void _downloadAndTranslateMissingLocale_test();
    void _onDownloadCompletedFailurePropagatesError_test();

private:
    void readJson(const QByteArray& bytes, QJsonDocument& jsonDoc);
};
