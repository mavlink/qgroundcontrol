/****************************************************************************
 *
 * (c) 2023 Aviant As
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>

class PlanMasterController;

class AviantMissionTools : public QObject
{
    Q_OBJECT
    
public:
    AviantMissionTools(QObject* parent = nullptr);
    AviantMissionTools(PlanMasterController* masterController, QObject* parent = nullptr);
    ~AviantMissionTools();

    enum Operation {
        NoOperation,
        MissionValidation,
        RallyPointHeight
    };

    enum MissionType {
        NotSet,
        FixedWing,
        Winch,
        CustomWinch
    };

    Q_ENUM(Operation)
    Q_ENUM(MissionType)
    
    Q_PROPERTY(PlanMasterController* masterController  READ masterController  CONSTANT)
    Q_PROPERTY(bool                  requestInProgress READ requestInProgress                      NOTIFY   stateChanged)
    Q_PROPERTY(Operation             currentOperation  READ currentOperation                       NOTIFY   stateChanged)
    Q_PROPERTY(MissionType           missionType       READ missionType       WRITE setMissionType NOTIFY   stateChanged)
    Q_PROPERTY(QStringList           missionTypeList   READ missionTypeList   CONSTANT)
    Q_PROPERTY(QString               validationResult  READ validationResult                       NOTIFY   stateChanged)
    
    Q_INVOKABLE void requestOperation(Operation operation);
    Q_INVOKABLE void cancelOperation(Operation operation);

    PlanMasterController* masterController       (void) const { return _masterController; }
    bool                  requestInProgress      (void) const { return _currentOperation != NoOperation; }
    Operation             currentOperation       (void) const { return _currentOperation; }
    MissionType           missionType            (void) const { return _missionType; }
    QStringList           missionTypeList        (void) const;
    void                  setMissionType         (MissionType missionType);
    QString               validationResult       (void) const { return _validationResult; }

signals:
    void stateChanged          (void);
    void cancelPendingRequest  (void);

private slots:
    void _requestComplete (QNetworkReply *reply);

private:
    QUrl           _getUrl                      (Operation operation, QString base);
    void           _parseValidationResponse     (const QByteArray &bytes);
    void           _parseAndLoadMissionResponse (const QByteArray &bytes);
    static QString _getOperationName            (Operation operation);
    static QString _getMissionTypeName          (MissionType missionType);
    static bool    _missionTypeRequired(Operation operation);

    PlanMasterController*   _masterController;
    Operation               _currentOperation =     NoOperation;
    MissionType             _missionType =          NotSet;
    QNetworkAccessManager*  _networkAccessManager = nullptr;
    QNetworkRequest         _networkRequest;
    QString                 _validationResult =     "Not validated";
    bool                    _validationConcluded =  false;
    QJsonDocument           _lastValidatedJson;
};
