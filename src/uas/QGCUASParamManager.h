#ifndef QGCUASPARAMMANAGER_H
#define QGCUASPARAMMANAGER_H

#include <QWidget>
#include <QMap>
#include <QTimer>

class UASInterface;

class QGCUASParamManager : public QWidget
{
    Q_OBJECT
public:
    QGCUASParamManager(UASInterface* uas, QWidget *parent = 0);

    QList<QString> getParameterNames(int component) const { return parameters.value(component)->keys(); }
    QList<float> getParameterValues(int component) const { return parameters.value(component)->values(); }
    float getParameterValue(int component, const QString& parameter) const { return parameters.value(component)->value(parameter); }

    /** @brief Request an update for the parameter list */
    void requestParameterListUpdate(int component = 0);
    /** @brief Request an update for this specific parameter */
    void requestParameterUpdate(int component, const QString& parameter);

signals:
    void parameterChanged(int component, QString parameter, float value);
    void parameterChanged(int component, int parameterIndex, float value);
    void parameterListUpToDate(int component);

public slots:

protected:
    UASInterface* mav;   ///< The MAV this widget is controlling
    QMap<int, QMap<QString, float>* > changedValues; ///< Changed values
    QMap<int, QMap<QString, float>* > parameters; ///< All parameters
    QVector<bool> received; ///< Successfully received parameters
    QMap<int, QList<int>* > transmissionMissingPackets; ///< Missing packets
    QMap<int, QMap<QString, float>* > transmissionMissingWriteAckPackets; ///< Missing write ACK packets
    bool transmissionListMode;       ///< Currently requesting list
    QMap<int, bool> transmissionListSizeKnown;  ///< List size initialized?
    bool transmissionActive;         ///< Missing packets, working on list?
    quint64 transmissionTimeout;     ///< Timeout
    QTimer retransmissionTimer;      ///< Timer handling parameter retransmission
    int retransmissionTimeout; ///< Retransmission request timeout, in milliseconds
    int rewriteTimeout; ///< Write request timeout, in milliseconds
    int retransmissionBurstRequestSize; ///< Number of packets requested for retransmission per burst

};

#endif // QGCUASPARAMMANAGER_H
