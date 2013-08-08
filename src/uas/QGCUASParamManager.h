#ifndef QGCUASPARAMMANAGER_H
#define QGCUASPARAMMANAGER_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include <QVariant>

//forward declarations
class UASInterface;
class UASParameterCommsMgr;
class UASParameterDataModel;

class QGCUASParamManager : public QWidget
{
    Q_OBJECT
public:
    QGCUASParamManager(UASInterface* uas, QWidget *parent = 0);

    /** @brief Get the known, confirmed value of a parameter */
    virtual bool getParameterValue(int component, const QString& parameter, QVariant& value) const;

    /** @brief Provide tooltips / user-visible descriptions for parameters */
    virtual void setParamDescriptions(const QMap<QString,QString>& paramDescs);

    /** @brief Get the UAS of this widget
     * @return The MAV of this mgr. Unless the MAV object has been destroyed, this is never null.
     */
    UASInterface* getUAS();

protected:
    //TODO decouple this UI message display further
    virtual void setParameterStatusMsg(const QString& msg);
    /** @brief Load parameter meta information from appropriate CSV file */
    virtual void loadParamMetaInfoCSV();


signals:
    void parameterChanged(int component, QString parameter, QVariant value);
    void parameterChanged(int component, int parameterIndex, QVariant value);
//    void parameterUpdateRequested(int component, const QString& parameter);
//    void parameterUpdateRequestedById(int componentId, int paramId);


public slots:
    /** @brief Send one parameter to the MAV: changes value in transient memory of MAV */
    virtual void setParameter(int component, QString parameterName, QVariant value);

    /** @brief Request list of parameters from MAV */
    virtual void requestParameterList();

    /** @brief Request a single parameter by name from the MAV */
    virtual void requestParameterUpdate(int component, const QString& parameter);

    virtual void handleParameterUpdate(int component, const QString& parameterName, QVariant value) = 0;
    virtual void handleParameterListUpToDate() = 0;


protected:

    // Parameter data model
    UASInterface* mav;   ///< The MAV this widget is controlling
    UASParameterDataModel* paramDataModel;///< Shared data model of parameters
    UASParameterCommsMgr*   paramCommsMgr; ///< Shared comms mgr for parameters

    // Status
    QString parameterStatusMsg;

};

#endif // QGCUASPARAMMANAGER_H
