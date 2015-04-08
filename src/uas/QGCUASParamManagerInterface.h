#ifndef QGCUASPARAMMANAGERINTERFACE_H
#define QGCUASPARAMMANAGERINTERFACE_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include <QVariant>
#include <QTextStream>

#include "UASParameterDataModel.h"

/**
 * @brief This is the abstract base class for QGCUASParamManager. Although there is 
 * normally only a single QGCUASParamManager we still use an abstract base interface 
 * since this allows us to create mock versions.
 *
 * See QGCUASParamManager.h for method documentation
 **/

//forward declarations
class QTextStream;
class UASInterface;
class UASParameterCommsMgr;

class QGCUASParamManagerInterface : public QObject
{
public:
    QGCUASParamManagerInterface(QObject* parent = NULL) : QObject(parent) { }
    virtual bool getParameterValue(int component, const QString& parameter, QVariant& value) const = 0;
    virtual int getDefaultComponentId() = 0;
    virtual QList<int> getComponentForParam(const QString& parameter) const = 0;
    virtual void setParamDescriptions(const QMap<QString,QString>& paramDescs) = 0;
    virtual int countPendingParams() = 0;
    virtual int countOnboardParams() = 0;
    virtual UASParameterDataModel* dataModel() = 0;
    virtual bool parametersReady(void) = 0;
    
public slots:
    virtual void setParameter(int component, QString parameterName, QVariant value) = 0;
    virtual void sendPendingParameters(bool persistAfterSend = false, bool forceSend = false) = 0;
    virtual void requestParameterList() = 0;
    virtual void requestParameterListIfEmpty() = 0;
    virtual void setPendingParam(int componentId,  const QString& key,  const QVariant& value, bool forceSend = false) = 0;
    virtual void clearAllPendingParams() = 0;
    virtual void requestParameterUpdate(int component, const QString& parameter) = 0;
    virtual void writeOnboardParamsToStream(QTextStream &stream, const QString& uasName) = 0;
    virtual void readPendingParamsFromStream(QTextStream &stream) = 0;
    virtual void requestRcCalibrationParamsUpdate() = 0;
    virtual void copyVolatileParamsToPersistent() = 0;
    virtual void copyPersistentParamsToVolatile() = 0;
    
signals:
    void parameterStatusMsgUpdated(QString msg, int level);
    void parameterListUpToDate();
    void parameterListProgress(float percentComplete);
    void parameterUpdated(int compId, QString paramName, QVariant value);
    void pendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending);
};

#endif // QGCUASPARAMMANAGER_H
