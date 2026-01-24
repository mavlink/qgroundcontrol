#pragma once

#include "UnitTest.h"

#include <QtCore/QString>

class MAVLinkLogFiles;
class MAVLinkLogManager;
class Vehicle;

/// Unit tests for MAVLinkLogManager and related classes.
/// Tests MAVLink log streaming, file management, and upload functionality.
class MAVLinkLogManagerTest : public UnitTest
{
    Q_OBJECT

public:
    MAVLinkLogManagerTest() = default;

private slots:
    void cleanup() override;

    // MAVLinkLogManager initialization tests
    void _testInitialization();
    void _testDefaultPropertyValues();

    // Property setter tests
    void _testSetEmailAddress();
    void _testSetDescription();
    void _testSetUploadURL();
    void _testSetUploadURLEmpty();
    void _testSetFeedback();
    void _testSetVideoURL();
    void _testSetEnableAutoUpload();
    void _testSetEnableAutoStart();
    void _testSetDeleteAfterUpload();
    void _testSetPublicLog();
    void _testSetWindSpeed();
    void _testSetRating();

    // State tests
    void _testLogRunningInitialState();
    void _testCanStartLog();
    void _testLogFilesModel();

    // MAVLinkLogFiles tests
    void _testLogFilesInitialState();
    void _testLogFilesSetSelected();
    void _testLogFilesSetUploading();
    void _testLogFilesSetUploaded();
    void _testLogFilesSetWriting();
    void _testLogFilesSetProgress();
    void _testLogFilesSetSize();

private:
    MAVLinkLogManager *_createManager();
    Vehicle *_vehicle = nullptr;
};
