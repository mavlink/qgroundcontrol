/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMVideoStreamingComponentController_H
#define APMVideoStreamingComponentController_H

#include <QObject>
#include <QList>

#include "FactPanelController.h"

class APMVideoStreamingComponentController : public FactPanelController
{
    Q_OBJECT

public:
    APMVideoStreamingComponentController(void);
    ~APMVideoStreamingComponentController();

    Q_PROPERTY(QString targetIp        READ targetIp        NOTIFY addressChanged)
    Q_PROPERTY(int     targetPort      READ targetPort      NOTIFY addressChanged)
    Q_PROPERTY(Fact*   videoStatusFact READ videoStatusFact CONSTANT)
    Q_PROPERTY(Fact*   resolutionFact  READ resolutionFact  CONSTANT)
    Q_PROPERTY(Fact*   bitRateFact     READ bitRateFact     CONSTANT)
    Q_PROPERTY(Fact*   fpsFact         READ fpsFact         CONSTANT)
    Q_PROPERTY(Fact*   rotationFact    READ rotationFact    CONSTANT)

    Q_INVOKABLE void startVideo(void);
    Q_INVOKABLE void stopVideo(void);
    Q_INVOKABLE void saveAddress(const QString ip, const int port);

    QString targetIp(void)   const { return _targetIp; }
    int     targetPort(void) const { return _targetPort; }

    Fact* videoStatusFact(void) { return &_videoStatusFact; }
    Fact* resolutionFact(void)  { return &_resolutionFact; }
    Fact* bitRateFact(void)     { return &_bitRateFact; }
    Fact* fpsFact(void)         { return &_fpsFact; }
    Fact* rotationFact(void)    { return &_rotationFact; }

signals:
    void addressChanged();

private slots:
    void _handleCameraCaptureStatus(mavlink_message_t message);
    void _handleVideoStreamTarget(mavlink_message_t message);

private:
    void _getResolution();

    MAVLinkProtocol* _mavlink;

    QString     _targetIp;
    int         _targetPort;
    QStringList _resolutionList;

    Fact         _videoStatusFact;
    Fact         _resolutionFact;
    Fact         _resolutionHorizontalFact;
    Fact         _resolutionVerticalFact;
    Fact         _bitRateFact;
    Fact         _fpsFact;
    Fact         _rotationFact;

    FactMetaData _resolutionMetaData;
    FactMetaData _bitRateMetaData;
    FactMetaData _fpsMetaData;
    FactMetaData _rotationMetaData;
};

#endif
