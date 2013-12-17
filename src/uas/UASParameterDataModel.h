#ifndef UASPARAMETERDATAMODEL_H
#define UASPARAMETERDATAMODEL_H

#include <QMap>
#include <QObject>
#include <QVariant>

class QTextStream;

class UASParameterDataModel : public QObject
{
    Q_OBJECT
public:
    explicit UASParameterDataModel(QObject *parent = 0);
    

    //Parameter meta info
    bool isParamMinKnown(const QString& param) { return paramMin.contains(param); }
    virtual bool isValueLessThanParamMin(const QString& param, double dblVal);

    bool isParamMaxKnown(const QString& param) { return paramMax.contains(param); }
    virtual bool isValueGreaterThanParamMax(const QString& param, double dblVal);

    bool isParamDefaultKnown(const QString& param) { return paramDefault.contains(param); }
    double getParamMin(const QString& param) { return paramMin.value(param, 0.0f); }
    double getParamMax(const QString& param) { return paramMax.value(param, 0.0f); }
    double getParamDefault(const QString& param) { return paramDefault.value(param, 0.0f); }
    virtual QString getParamDescription(const QString& param) { return paramDescriptions.value(param, ""); }
    virtual void setParamDescriptions(const QMap<QString,QString>& paramInfo);

    /** @brief Get the default component ID for the UAS */
    virtual int getDefaultComponentId();

    //TODO make this method protected?
     /** @brief Ensure that the data model is aware of this component
      * @param compId Id of the component
      */
    virtual void addComponent(int compId);

    /**
     * @brief Return a list of all components for this parameter name
     * @param parameter The parameter string to search for
     * @return A list with all components, can be potentially empty
     */
    virtual QList<int> getComponentForOnboardParam(const QString& parameter) const;


    /** @brief clears every parameter for every loaded component */
    virtual void forgetAllOnboardParams();



    /** @brief add this parameter to pending list iff it has changed from onboard value
     * @return true if the parameter is now pending
    */
    virtual bool updatePendingParamWithValue(int componentId, const QString &key,  const QVariant &value, bool forceSend = false);
    virtual void handleParamUpdate(int componentId, const QString& key, const QVariant& value);
    virtual bool getOnboardParamValue(int componentId, const QString& key, QVariant& value) const;

    virtual bool isParamChangePending(int componentId,const QString& key);

    QMap<QString , QVariant>* getPendingParamsForComponent(int componentId) {
        return pendingParameters.value(componentId);
    }

    QMap<QString , QVariant>* getOnboardParamsForComponent(int componentId) {
        return onboardParameters.value(componentId);
    }

    QMap<int, QMap<QString, QVariant>* >*  getAllPendingParams() {
       return &pendingParameters;
   }

    QMap<int, QMap<QString, QVariant>* >* getAllOnboardParams() {
       return &onboardParameters;
   }

    /** @brief return a count of all pending parameters */
    virtual int countPendingParams();

    /** @brief return a count of all onboard parameters we've received */
    virtual int countOnboardParams();

    virtual void writeOnboardParamsToStream(QTextStream &stream, const QString& uasName);
    virtual void readUpdateParamsFromStream(QTextStream &stream);

    virtual void loadParamMetaInfoFromStream(QTextStream& stream);

    void setUASID(int anId) {  this->uasId = anId; }

protected:
    /** @brief set the confirmed value of a parameter in the onboard params list */
    virtual void setOnboardParam(int componentId, const QString& key, const QVariant& value);

    /** @brief Save the parameter with a the type specified in the QVariant as fixed */
    void setParamWithTypeInMap(int compId, const QString& key, const QVariant &value, QMap<int, QMap<QString, QVariant>* >& map);

    /** @brief Write a new pending parameter value that may be eventually sent to the UAS */
    virtual void setPendingParam(int componentId,  const QString &key,  const QVariant& value);
    /** @brief remove a parameter from the pending list */
    virtual void removePendingParam(int compId, const QString &key);


signals:
    
    /** @brief We've received an update of a parameter's value */
    void parameterUpdated(int compId, QString paramName, QVariant value);

    /** @brief Notifies listeners that a param was added to or removed from the pending list */
    void pendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending);

    void allPendingParamsCommitted(); ///< All pending params have been committed to the MAV

public slots:

    virtual void clearAllPendingParams();

protected:
    int             defaultComponentId; ///< Cached default component ID

    int     uasId; ///< The UAS / MAV to which this data model pertains
    QMap<int, QMap<QString, QVariant>* > pendingParameters; ///< Changed values that have not yet been transmitted to the UAS, by component ID
    QMap<int, QMap<QString, QVariant>* > onboardParameters; ///< All parameters confirmed to be stored onboard the UAS, by component ID

    // Tooltip data structures
    QMap<QString, QString> paramDescriptions; ///< Tooltip values

    // Min / Default / Max data structures
    QMap<QString, double> paramMin; ///< Minimum param values
    QMap<QString, double> paramDefault; ///< Default param values
    QMap<QString, double> paramMax; ///< Minimum param values

    
};

#endif // UASPARAMETERDATAMODEL_H
