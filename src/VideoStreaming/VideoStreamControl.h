/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>

#include "MAVLinkProtocol.h"
#include "VideoSettings.h"
#include "SettingsManager.h"

Q_DECLARE_LOGGING_CATEGORY(VideoStreamControlLog)

class VideoStreamControl : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString          videoResolution         READ videoResolution        NOTIFY videoResolutionChanged)
    Q_PROPERTY(uint             cameraCount             READ cameraCount            NOTIFY cameraCountChanged)
    Q_PROPERTY(bool             settingInProgress       READ settingInProgress      NOTIFY settingInProgressChanged)

public:
    VideoStreamControl();
    ~VideoStreamControl();

    QString videoStreamUrl();
    QString videoResolution();
    uint cameraCount();
    bool settingInProgress();

public slots:
    void fhdEnabledChanged(bool fhdEnabled);

signals:
    void videoStreamUrlChanged();
    void videoResolutionChanged();
    void cameraCountChanged();
    void settingInProgressChanged();

private slots:
    void _mavlinkMessageReceived(LinkInterface *link, mavlink_message_t message);
    void _connectionLostTimeout();
    void _settingInProgressTimeout();
    void _cameraIdChanged();
    void _videoShareChanged();

private:
    int _systemId;
    LinkInterface *_linkInterface;
    MAVLinkProtocol *_mavlinkProtocol;
    VideoSettings *_videoSettings;
    QString _videoStreamUrl;
    QString _videoResolution;
    QTimer _connectionLostTimer;
    QTimer _settingInProgressTimer;
    int _videoStreamSetupTimeout;
    int _checkResolutionTimeout;
    uint32_t _cameraServiceUid;
    uint32_t _cameraCount;
    uint32_t _cameraIdSetting;
    uint32_t _cameraIdInUse;
    bool _settingInProgress;

    void _handleHeartbeatInfo(LinkInterface* link, mavlink_message_t& message);
    void _handleVideoStreamInfo(mavlink_message_t& message);
    void _setupVideoStream();
    void _resetVideoStreamInfo();
    void _requestVideoStreamInfo();
    void _checkCameraId();
    void _setCameraId();
    void _setCameraIdLockUi(bool lockUi);
    void _startVideoStreaming();
    void _checkResolution();
    void _setFhdEnabledLockUi(bool fhdEnabled, bool lockUi);
    void _setVideoResolution(int h, int v);
    void _setSettingInProgress(bool inProgress);
    void _connectionActive();
};
