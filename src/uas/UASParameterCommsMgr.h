#ifndef UASPARAMETERCOMMSMGR_H
#define UASPARAMETERCOMMSMGR_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QLoggingCategory>

class UASInterface;
class UASParameterDataModel;

Q_DECLARE_LOGGING_CATEGORY(UASParameterCommsMgrLog)

class UASParameterCommsMgr : public QObject
{
    Q_OBJECT


public:
    explicit UASParameterCommsMgr(QObject *parent = 0);
    UASParameterCommsMgr* initWithUAS(UASInterface* model);///< Two-stage constructor

    ~UASParameterCommsMgr();

    typedef enum ParamCommsStatusLevel {
        ParamCommsStatusLevel_OK = 0,
        ParamCommsStatusLevel_Warning = 2,
        ParamCommsStatusLevel_Error = 4,
        ParamCommsStatusLevel_Count
    } ParamCommsStatusLevel_t;


protected:

    /** @brief activate the silence timer if there are unack'd reads or writes */
    virtual void updateSilenceTimer();

    virtual void setParameterStatusMsg(const QString& msg, ParamCommsStatusLevel_t level=ParamCommsStatusLevel_OK);

    /** @brief Load settings that control eg retransmission timeouts */
    void loadParamCommsSettings();

    /** @brief clear transmissionMissingPackets and transmissionMissingWriteAckPackets */
    void clearRetransmissionLists(int& missingReadCount, int& missingWriteCount );

    /** @brief we are waiting for a response to this read param request */
    virtual void markReadParamWaiting(int compId, int paramId);

    /** @brief we are waiting for a response to this write param request */
    void markWriteParamWaiting(int compId, QString paramName, QVariant value);

    void resendReadWriteRequests();
    void resetAfterListReceive();

    void emitPendingParameterCommit(int compId, const QString& key, QVariant& value);

signals:
    void commitPendingParameter(int component, QString parameter, QVariant value);
    void parameterChanged(int component, int parameterIndex, QVariant value);
    void parameterValueConfirmed(int uas, int component,int paramCount, int paramId, QString parameter, QVariant value);

    /** @brief We have received a complete list of all parameters onboard the MAV */
    void parameterListUpToDate();
    
    void parameterListProgress(float percentComplete);

    void parameterUpdateRequested(int component, const QString& parameter);
    void parameterUpdateRequestedById(int componentId, int paramId);

    /** @brief We updated the parameter status message */
    void parameterStatusMsgUpdated(QString msg, int level);
    
    
    /// @brief We signal this to ourselves in order to get timer started/stopped on our own thread.
    void _startSilenceTimer(void);
    void _stopSilenceTimer(void);

public slots:
    /** @brief  Iterate through all components, through all pending parameters and send them to UAS */
    virtual void sendPendingParameters(bool copyToPersistent = false, bool forceSend = false);

    /** @brief  Write the current onboard parameters from transient RAM into persistent storage, e.g. EEPROM or harddisk */
    virtual void writeParamsToPersistentStorage();

    /** @brief Write one parameter to the MAV */
    virtual void setParameter(int component, QString parameterName, QVariant value, bool forceSend = false);

    /** @brief Request list of parameters from MAV */
    virtual void requestParameterList();

    /** @brief The max silence time expired */
    virtual void silenceTimerExpired();

    /** @brief Request a single parameter update by name */
    virtual void requestParameterUpdate(int component, const QString& parameter);

    /** @brief Request an update of RC parameters */
    virtual void requestRcCalibrationParamsUpdate();

    virtual void receivedParameterUpdate(int uas, int compId, int paramCount, int paramId, QString paramName, QVariant value);

protected:


    QMap<int, int> knownParamListSize;///< The known param list size for each component, by component ID
    quint64 lastReceiveTime; ///< The last time we received anything from our partner
    quint64 lastSilenceTimerReset;
    UASInterface* mav;   ///< The MAV we're talking to

    int maxSilenceTimeout; ///< If nothing received within this period of time, abandon resends

    UASParameterDataModel* paramDataModel;

    bool persistParamsAfterSend; ///< Copy all parameters to persistent storage after sending
    QMap<int, QSet<int>*> readsWaiting; ///< All reads that have not yet been received, by component ID
    int retransmitBurstLimit; ///< Number of packets requested for retransmission per burst
    int silenceTimeout; ///< If nothing received within this period of time, start resends
    QTimer silenceTimer;      ///< Timer handling parameter retransmission
    bool transmissionListMode;       ///< Currently requesting list
    QMap<int, QMap<QString, QVariant>* > writesWaiting; ///< All writes that have not yet been ack'd, by component ID

private slots:
    /// @brief We signal this to ourselves in order to get timer started/stopped on our own thread.
    void _startSilenceTimerOnThisThread(void);
    void _stopSilenceTimerOnThisThread(void);
    
private:
    void _sendParamRequestListMsg(void);
};
    

#endif // UASPARAMETERCOMMSMGR_H
