#pragma once

#include "UnitTest.h"

#include <QtCore/QDir>
#include <QtCore/QString>

class QTemporaryDir;

/// Unit tests for GeoTagController and GeoTagWorker.
/// Tests geo-tagging functionality for correlating images with flight logs.
class GeoTagControllerTest : public UnitTest
{
    Q_OBJECT

public:
    GeoTagControllerTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    // Controller tests
    void _controllerCreationTest();
    void _controllerConfigurationTest();
    void _controllerTaggingProgressTest();
    void _controllerInvalidLogFileTest();
    void _controllerInvalidImageDirTest();

    // Worker tests
    void _workerProcessTest();
    void _workerEmptyImageDirTest();

private:
    bool _setupTestImages(int imageCount);
    void _cleanupDirectory(const QString &dirPath);

    static constexpr const char* kTaggedDirName = "TAGGED";
    static constexpr int kDefaultImageCount = 58;

    QTemporaryDir *_tempDir = nullptr;
    QString _imageDirPath;
    QString _taggedDirPath;
};
