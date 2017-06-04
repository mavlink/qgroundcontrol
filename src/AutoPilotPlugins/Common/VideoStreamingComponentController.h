/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef VideoStreamingComponentController_H
#define VideoStreamingComponentController_H

#include <QObject>
#include <QList>

#include "Vehicle.h"
#include "FactPanelController.h"

class VideoStreamingFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VideoStreamingFactGroup(QString fileName, QObject* parent = NULL);

    Q_PROPERTY(Fact* videoStatusFact          READ videoStatusFact          CONSTANT)
    Q_PROPERTY(Fact* resolutionFact           READ resolutionFact           CONSTANT)
    Q_PROPERTY(Fact* resolutionHorizontalFact READ resolutionHorizontalFact CONSTANT)
    Q_PROPERTY(Fact* resolutionVerticalFact   READ resolutionVerticalFact   CONSTANT)
    Q_PROPERTY(Fact* bitRateFact              READ bitRateFact              CONSTANT)
    Q_PROPERTY(Fact* fpsFact                  READ fpsFact                  CONSTANT)
    Q_PROPERTY(Fact* rotationFact             READ rotationFact             CONSTANT)
    Q_PROPERTY(Fact* uriFact                  READ uriFact                  CONSTANT)
    Q_PROPERTY(Fact* protocolFact             READ protocolFact             CONSTANT)

    Fact* videoStatusFact(void)          { return &_videoStatusFact; }
    Fact* resolutionFact(void)           { return &_resolutionFact; }
    Fact* resolutionHorizontalFact(void) { return &_resolutionHorizontalFact; }
    Fact* resolutionVerticalFact(void)   { return &_resolutionVerticalFact; }
    Fact* bitRateFact(void)              { return &_bitRateFact; }
    Fact* fpsFact(void)                  { return &_fpsFact; }
    Fact* rotationFact(void)             { return &_rotationFact; }
    Fact* uriFact(void)                  { return &_uriFact; }
    Fact* protocolFact(void)             { return &_protocolFact; }


    static const char* _videoStatusFactName;
    static const char* _resolutionFactName;
    static const char* _resolutionHorizontalFactName;
    static const char* _resolutionVerticalFactName;
    static const char* _bitRateFactName;
    static const char* _fpsFactName;
    static const char* _rotationFactName;
    static const char* _uriFactName;
    static const char* _protocolFactName;

private:
    Fact _videoStatusFact;
    Fact _resolutionFact;
    Fact _resolutionHorizontalFact;
    Fact _resolutionVerticalFact;
    Fact _bitRateFact;
    Fact _fpsFact;
    Fact _rotationFact;
    Fact _uriFact;
    Fact _protocolFact;
};

class VideoStreamingComponentController : public FactPanelController
{
    Q_OBJECT

public:
    VideoStreamingComponentController(void);

    Q_PROPERTY(QVariantList cameras     MEMBER _cameras     CONSTANT)
    Q_PROPERTY(QStringList  cameraNames MEMBER _cameraNames CONSTANT)

    Q_INVOKABLE void saveSettings(int index);
    Q_INVOKABLE void startVideo(int index);
    Q_INVOKABLE void stopVideo(int index);

private slots:
    void _handleVideoStreamInformation(mavlink_message_t message);

private:
    QVariantList _cameras;
    QStringList  _cameraNames;
    QList<int>   _cameraIds;

    MAVLinkProtocol* _mavlink;

    QStringList _resolutionList;

    FactMetaData _resolutionMetaData;
    FactMetaData _bitRateMetaData;
    FactMetaData _fpsMetaData;
    FactMetaData _rotationMetaData;
};

#endif
