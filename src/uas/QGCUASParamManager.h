#ifndef QGCUASPARAMMANAGER_H
#define QGCUASPARAMMANAGER_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include <QVariant>

class UASInterface;
class UASParameterDataModel;

class QGCUASParamManager : public QWidget
{
    Q_OBJECT
public:
    QGCUASParamManager(UASInterface* uas, QWidget *parent = 0);

    virtual bool getParameterValue(int component, const QString& parameter, QVariant& value) const;

    virtual bool isParamMinKnown(const QString& param) = 0;
    virtual bool isParamMaxKnown(const QString& param) = 0;
    virtual bool isParamDefaultKnown(const QString& param) = 0;
    virtual double getParamMin(const QString& param) = 0;
    virtual double getParamMax(const QString& param) = 0;
    virtual double getParamDefault(const QString& param) = 0;
    virtual QString getParamInfo(const QString& param) = 0;
    virtual void setParamInfo(const QMap<QString,QString>& param) = 0;

    /** @brief Request an update for the parameter list */
    void requestParameterListUpdate(int component = 0);
    /** @brief Request an update for this specific parameter */
    virtual void requestParameterUpdate(int component, const QString& parameter) = 0;

protected:
    /** @brief Check for missing parameters */
    virtual void retransmissionGuardTick();
    /** @brief Activate / deactivate parameter retransmission */
    virtual void setRetransmissionGuardEnabled(bool enabled);

    //TODO decouple this UI message display further
    virtual void setParameterStatusMsg(const QString& msg);

signals:
    void parameterChanged(int component, QString parameter, QVariant value);
    void parameterChanged(int component, int parameterIndex, QVariant value);
    void parameterListUpToDate(int component);
    /** @brief Request a single parameter */
    void requestParameter(int component, int parameter);
    /** @brief Request a single parameter by name */
    void requestParameter(int component, const QString& parameter);

public slots:
    /** @brief Write one parameter to the MAV */
    virtual void setParameter(int component, QString parameterName, QVariant value) = 0;
    /** @brief Request list of parameters from MAV */
    virtual void requestParameterList();

protected:

    // Parameter data model
    UASInterface* mav;   ///< The MAV this widget is controlling
    UASParameterDataModel* paramDataModel;///< Shared data model of parameters

    // Communications management
    QVector<bool> received; ///< Successfully received parameters
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
    QString parameterStatusMsg;

};

#endif // QGCUASPARAMMANAGER_H
