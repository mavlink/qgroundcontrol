/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MissionItem_H
#define MissionItem_H

#include <QObject>
#include <QString>
#include <QtQml>
#include <QTextStream>
#include <QJsonObject>
#include <QGeoCoordinate>

#include "QGCMAVLink.h"
#include "QGC.h"
#include "QmlObjectListModel.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

class SurveyComplexItem;
class SimpleMissionItem;
class MissionController;
#ifdef UNITTEST_BUILD
    class MissionItemTest;
#endif

// Represents a Mavlink mission command.
class MissionItem : public QObject
{
    Q_OBJECT
    
public:
    MissionItem(QObject* parent = nullptr);

    MissionItem(int             sequenceNumber,
                MAV_CMD         command,
                MAV_FRAME       frame,
                double          param1,
                double          param2,
                double          param3,
                double          param4,
                double          param5,
                double          param6,
                double          param7,
                bool            autoContinue,
                bool            isCurrentItem,
                QObject*        parent = nullptr);

    MissionItem(const MissionItem& other, QObject* parent = nullptr);

    ~MissionItem();

    const MissionItem& operator=(const MissionItem& other);
    
    MAV_CMD         command         (void) const { return (MAV_CMD)_commandFact.rawValue().toInt(); }
    bool            isCurrentItem   (void) const { return _isCurrentItem; }
    int             sequenceNumber  (void) const { return _sequenceNumber; }
    MAV_FRAME       frame           (void) const { return (MAV_FRAME)_frameFact.rawValue().toInt(); }
    bool            autoContinue    (void) const { return _autoContinueFact.rawValue().toBool(); }
    double          param1          (void) const { return _param1Fact.rawValue().toDouble(); }
    double          param2          (void) const { return _param2Fact.rawValue().toDouble(); }
    double          param3          (void) const { return _param3Fact.rawValue().toDouble(); }
    double          param4          (void) const { return _param4Fact.rawValue().toDouble(); }
    double          param5          (void) const { return _param5Fact.rawValue().toDouble(); }
    double          param6          (void) const { return _param6Fact.rawValue().toDouble(); }
    double          param7          (void) const { return _param7Fact.rawValue().toDouble(); }
    QGeoCoordinate  coordinate      (void) const;
    int             doJumpId        (void) const { return _doJumpId; }

    /// @return Flight speed change value if this item supports it. If not it returns NaN.
    double specifiedFlightSpeed(void) const;

    /// @return Flight gimbal yaw change value if this item supports it. If not it returns NaN.
    double specifiedGimbalYaw(void) const;

    /// @return Flight gimbal pitch change value if this item supports it. If not it returns NaN.
    double specifiedGimbalPitch(void) const;

    void setCommand         (MAV_CMD command);
    void setSequenceNumber  (int sequenceNumber);
    void setIsCurrentItem   (bool isCurrentItem);
    void setFrame           (MAV_FRAME frame);
    void setAutoContinue    (bool autoContinue);
    void setParam1          (double param1);
    void setParam2          (double param2);
    void setParam3          (double param3);
    void setParam4          (double param4);
    void setParam5          (double param5);
    void setParam6          (double param6);
    void setParam7          (double param7);
    
    void save(QJsonObject& json) const;
    bool load(QTextStream &loadStream);
    bool load(const QJsonObject& json, int sequenceNumber, QString& errorString);

    bool relativeAltitude(void) const { return frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT; }

signals:
    void isCurrentItemChanged       (bool isCurrentItem);
    void sequenceNumberChanged      (int sequenceNumber);
    void specifiedFlightSpeedChanged(double flightSpeed);
    void specifiedGimbalYawChanged  (double gimbalYaw);
    void specifiedGimbalPitchChanged(double gimbalPitch);

private slots:
    void _param1Changed(QVariant value);
    void _param2Changed(QVariant value);
    void _param3Changed(QVariant value);

private:
    bool _convertJsonV1ToV2(const QJsonObject& json, QJsonObject& v2Json, QString& errorString);
    bool _convertJsonV2ToV3(QJsonObject& json, QString& errorString);

    int     _sequenceNumber;
    int     _doJumpId;
    bool    _isCurrentItem;

    Fact    _autoContinueFact;
    Fact    _commandFact;
    Fact    _frameFact;
    Fact    _param1Fact;
    Fact    _param2Fact;
    Fact    _param3Fact;
    Fact    _param4Fact;
    Fact    _param5Fact;
    Fact    _param6Fact;
    Fact    _param7Fact;
    
    // Keys for Json save
    static const char*  _jsonFrameKey;
    static const char*  _jsonCommandKey;
    static const char*  _jsonAutoContinueKey;
    static const char*  _jsonParamsKey;
    static const char*  _jsonDoJumpIdKey;

    // Deprecated V2 format keys
    static const char*  _jsonCoordinateKey;

    // Deprecated V1 format keys
    static const char*  _jsonParam1Key;
    static const char*  _jsonParam2Key;
    static const char*  _jsonParam3Key;
    static const char*  _jsonParam4Key;

    friend class SurveyComplexItem;
    friend class SimpleMissionItem;
    friend class MissionController;
#ifdef UNITTEST_BUILD
    friend class MissionItemTest;
#endif
};

#endif
