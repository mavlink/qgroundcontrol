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
    

    int getTotalOnboardParams() { return totalOnboardParameters; }
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

    //TODO make this method protected?
     /** @brief Ensure that the data model is aware of this component
      * @param compId Id of the component
      */
    virtual void addComponent(int compId);

    /** @brief Write a new pending parameter value that may be eventually sent to the UAS */
    virtual void setPendingParameter(int componentId,  QString& key,  const QVariant& value);
    virtual void setOnboardParameter(int componentId, QString& key, const QVariant& value);

    /** @brief Save the onboard parameter with a the type specified in the QVariant as fixed */
    virtual void setOnboardParameterWithType(int componentId, QString& key, QVariant& value);

    /** @brief clears every parameter for every loaded component */
    virtual void forgetAllOnboardParameters();

    /**
     * @return true if the parameter has changed
    */
    virtual bool checkParameterChanged(int componentId, const QString& key,  const QVariant &value);


    /** @brief add this parameter to pending list iff it has changed from onboard value
     * @return true if the parameter has changed
    */
    virtual bool addPendingIfParameterChanged(int componentId, QString& key,  QVariant& value);


    virtual void handleParameterUpdate(int componentId, QString& key, QVariant& value);

    virtual bool getOnboardParameterValue(int componentId, const QString& key, QVariant& value) const;

    QMap<QString , QVariant>* getPendingParametersForComponent(int componentId) {
        return pendingParameters.value(componentId);
    }

    QMap<QString , QVariant>* getOnbardParametersForComponent(int componentId) {
        return onboardParameters.value(componentId);
    }

    QMap<int, QMap<QString, QVariant>* >  getPendingParameters() {
       return pendingParameters;
   }

    QMap<int, QMap<QString, QVariant>* > getOnboardParameters() {
       return onboardParameters;
   }


    virtual void writeOnboardParametersToStream(QTextStream &stream, const QString& uasName);
    virtual void readUpdateParametersFromStream(QTextStream &stream);

    virtual void loadParamMetaInfoFromStream(QTextStream& stream);

    void setUASID(int anId) {  this->uasId = anId; }

signals:
    
    void parameterUpdated(int compId, int paramId, QString paramName, QVariant value);

public slots:


protected:
    int     uasId; ///< The UAS / MAV to which this data model pertains
    QMap<int, QMap<QString, QVariant>* > pendingParameters; ///< Changed values that have not yet been transmitted to the UAS, by component ID
    QMap<int, QMap<QString, QVariant>* > onboardParameters; ///< All parameters confirmed to be stored onboard the UAS, by component ID
    int totalOnboardParameters;///< The known count of onboard parameters, may not match onboardParameters until all params are received

    // Tooltip data structures
    QMap<QString, QString> paramDescriptions; ///< Tooltip values

    // Min / Default / Max data structures
    QMap<QString, double> paramMin; ///< Minimum param values
    QMap<QString, double> paramDefault; ///< Default param values
    QMap<QString, double> paramMax; ///< Minimum param values

    
};

#endif // UASPARAMETERDATAMODEL_H
