#ifndef QGCUASPARAMMANAGER_H
#define QGCUASPARAMMANAGER_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include <QVariant>

#include "UASParameterDataModel.h"
#include "QGCUASParamManagerInterface.h"
#include "UASParameterCommsMgr.h"

//forward declarations
class QTextStream;
class UASInterface;

class QGCUASParamManager : public QGCUASParamManagerInterface
{
    Q_OBJECT
public:
    QGCUASParamManager(QObject* parent = 0);
    QGCUASParamManager* initWithUAS(UASInterface* uas);

    /** @brief Get the known, confirmed value of a parameter */
    virtual bool getParameterValue(int component, const QString& parameter, QVariant& value) const;

    /** @brief determine which component is the root component for the UAS and return its ID or 0 if unknown */
    virtual int getDefaultComponentId();

    /**
     * @brief Get a list of all component IDs using this parameter name
     * @param parameter The string encoding the parameter name
     * @return A list with all components using this parameter name. Can be empty.
     */
    virtual QList<int> getComponentForParam(const QString& parameter) const;

    /** @brief Provide tooltips / user-visible descriptions for parameters */
    virtual void setParamDescriptions(const QMap<QString,QString>& paramDescs);

    /**
     * @brief Count the pending parameters in the current transmission
     * @return The number of pending parameters
     */
    virtual int countPendingParams() {
        return paramDataModel.countPendingParams();
    }

    /**
     * @brief Count the number of onboard parameters
     * @return The number of onboard parameters
     */
    virtual int countOnboardParams() {
        return paramDataModel.countOnboardParams();
    }

    /** @brief Get the UAS of this widget
     * @return The MAV of this mgr. Unless the MAV object has been destroyed, this is never null.
     */
    UASInterface* getUAS();

    /** @return The data model managed by this class */
    virtual UASParameterDataModel* dataModel();
    
    /// @return true: first full set of parameters received
    virtual bool parametersReady(void) { return _parametersReady; }

protected:

    /** @brief Load parameter meta information from appropriate CSV file */
    virtual void loadParamMetaInfoCSV();

    void connectToModelAndComms();


signals:

    /** @brief We updated the parameter status message */
    void parameterStatusMsgUpdated(QString msg, int level);
    /** @brief We have received a complete list of all parameters onboard the MAV */
    void parameterListUpToDate();
    void parameterListProgress(float percentComplete);

    /** @brief We've received an update of a parameter's value */
    void parameterUpdated(int compId, QString paramName, QVariant value);

    /** @brief Notifies listeners that a param was added to or removed from the pending list */
    void pendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending);



public slots:
    /** @brief Send one parameter to the MAV: changes value in transient memory of MAV */
    virtual void setParameter(int component, QString parameterName, QVariant value);

    /** @brief Send all pending parameters to the MAV, for storage in transient (RAM) memory
     * @param persistAfterSend  If true, all parameters will be written to persistent storage as well
    */
    virtual void sendPendingParameters(bool persistAfterSend = false, bool forceSend = false);


    /** @brief Request list of parameters from MAV */
    virtual void requestParameterList();

    /** @brief Request a list of params onboard the MAV if the onboard param list we have is empty */
    virtual void requestParameterListIfEmpty();

    /** @brief queue a pending parameter for sending to the MAV */
    virtual void setPendingParam(int componentId,  const QString& key,  const QVariant& value, bool forceSend = false);

    /** @brief remove all params from the pending list */
    virtual void clearAllPendingParams();

    /** @brief Request a single parameter by name from the MAV */
    virtual void requestParameterUpdate(int component, const QString& parameter);


    virtual void writeOnboardParamsToStream(QTextStream &stream, const QString& uasName);
    virtual void readPendingParamsFromStream(QTextStream &stream);

    virtual void requestRcCalibrationParamsUpdate();

    /** @brief Copy the current parameters in volatile RAM to persistent storage (EEPROM/HDD) */
    virtual void copyVolatileParamsToPersistent();
    /** @brief Copy the parameters from persistent storage to volatile RAM  */
    virtual void copyPersistentParamsToVolatile();
    
private slots:
    void _parameterListUpToDate(void);

protected:

    // Parameter data model
    UASInterface*           mav;   ///< The MAV this manager is controlling
    UASParameterDataModel  paramDataModel;///< Shared data model of parameters
    UASParameterCommsMgr   paramCommsMgr; ///< Shared comms mgr for parameters
    
    bool _parametersReady;

};

#endif // QGCUASPARAMMANAGER_H
