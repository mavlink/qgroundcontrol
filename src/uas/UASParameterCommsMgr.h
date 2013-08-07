#ifndef UASPARAMETERCOMMSMGR_H
#define UASPARAMETERCOMMSMGR_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QVariant>
#include <QVector>

class UASInterface;
class UASParameterDataModel;

class UASParameterCommsMgr : public QObject
{
    Q_OBJECT
public:
    explicit UASParameterCommsMgr(QObject *parent = 0, UASInterface* uas = NULL);
    

protected:
    /** @brief Activate / deactivate parameter retransmission */
    virtual void setRetransmissionGuardEnabled(bool enabled);

    virtual void setParameterStatusMsg(const QString& msg);

signals:
    void parameterChanged(int component, QString parameter, QVariant value);
    void parameterChanged(int component, int parameterIndex, QVariant value);
    void parameterValueConfirmed(int component, QString parameter, QVariant value);

    void parameterListUpToDate(int component);
    void parameterUpdateRequested(int component, const QString& parameter);
    void parameterUpdateRequestedById(int componentId, int paramId);


public slots:
    /** @brief Write one parameter to the MAV */
    virtual void setParameter(int component, QString parameterName, QVariant value);
    /** @brief Request list of parameters from MAV */
    virtual void requestParameterList();
    /** @brief Check for missing parameters */
    virtual void retransmissionGuardTick();

    /** @brief Request a single parameter update by name */
    virtual void requestParameterUpdate(int component, const QString& parameter);

protected:

    UASInterface* mav;   ///< The MAV we're talking to

    UASParameterDataModel* paramDataModel;

    // Communications management
    QVector<bool> receivedParamsList; ///< Successfully received parameters
    QMap<int, QList<int>* > transmissionMissingPackets; ///< Missing packets
    QMap<int, QMap<QString, QVariant>* > transmissionMissingWriteAckPackets; ///< Missing write ACK packets
    bool transmissionListMode;       ///< Currently requesting list
    QMap<int, bool> transmissionListSizeKnown;  ///< List size initialized?
    bool transmissionActive;         ///< Missing packets, working on list?
    quint64 transmissionTimeout;     ///< Timeout
    QTimer retransmissionTimer;      ///< Timer handling parameter retransmission
    int retransmissionTimeout; ///< Retransmission request timeout, in milliseconds
    int rewriteTimeout; ///< Write request timeout, in milliseconds
    int retransmissionBurstRequestSize; ///< Number of packets requested for retransmission per burst

    // Status
    QString parameterStatusMsg;

};
    

#endif // UASPARAMETERCOMMSMGR_H
