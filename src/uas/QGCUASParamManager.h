#ifndef QGCUASPARAMMANAGER_H
#define QGCUASPARAMMANAGER_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include <QVariant>

#include "UASParameterDataModel.h"

//forward declarations
class QTextStream;
class UASInterface;
class UASParameterCommsMgr;

class QGCUASParamManager : public QObject
{
    Q_OBJECT
public:
    QGCUASParamManager(QObject* parent = 0,UASInterface* uas = 0);

    /** @brief Get the known, confirmed value of a parameter */
    virtual bool getParameterValue(int component, const QString& parameter, QVariant& value) const;

    /** @brief Provide tooltips / user-visible descriptions for parameters */
    virtual void setParamDescriptions(const QMap<QString,QString>& paramDescs);

    /** @brief Get the UAS of this widget
     * @return The MAV of this mgr. Unless the MAV object has been destroyed, this is never null.
     */
    UASInterface* getUAS();

    /** @return The data model managed by this class */
    virtual UASParameterDataModel* dataModel();

protected:

    /** @brief Load parameter meta information from appropriate CSV file */
    virtual void loadParamMetaInfoCSV();

    void connectToCommsMgr();


signals:

    /** @brief We updated the parameter status message */
    void parameterStatusMsgUpdated(QString msg, int level);
    /** @brief We have received a complete list of all parameters onboard the MAV */
    void parameterListUpToDate();

public slots:
    /** @brief Send one parameter to the MAV: changes value in transient memory of MAV */
    virtual void setParameter(int component, QString parameterName, QVariant value);

    /** @brief Send all pending parameters to the MAV, for storage in transient (RAM) memory */
    virtual void sendPendingParameters();

    /** @brief Request list of parameters from MAV */
    virtual void requestParameterList();

    /** @brief Request a list of params onboard the MAV if the onboard param list we have is empty */
    virtual void requestParameterListIfEmpty();

    virtual void setPendingParam(int componentId,  QString& key,  const QVariant& value);

    /** @brief Request a single parameter by name from the MAV */
    virtual void requestParameterUpdate(int component, const QString& parameter);


    virtual void writeOnboardParamsToStream(QTextStream &stream, const QString& uasName);
    virtual void readPendingParamsFromStream(QTextStream &stream);

    virtual void requestRcCalibrationParamsUpdate();

    /** @brief Copy the current parameters in volatile RAM to persistent storage (EEPROM/HDD) */
    virtual void copyVolatileParamsToPersistent();
    /** @brief Copy the parameters from persistent storage to volatile RAM  */
    virtual void copyPersistentParamsToVolatile();

protected:

    // Parameter data model
    UASInterface*           mav;   ///< The MAV this manager is controlling
    UASParameterDataModel  paramDataModel;///< Shared data model of parameters
    UASParameterCommsMgr*   paramCommsMgr; ///< Shared comms mgr for parameters

};

#endif // QGCUASPARAMMANAGER_H
