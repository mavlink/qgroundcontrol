#ifndef UASPARAMETERDATAMODEL_H
#define UASPARAMETERDATAMODEL_H

#include <QMap>
#include <QObject>
#include <QVariant>

class UASParameterDataModel : public QObject
{
    Q_OBJECT
public:
    explicit UASParameterDataModel(QObject *parent = 0);
    

    /** @brief Write a new pending parameter value that may be eventually sent to the UAS */
    virtual void setPendingParameter(int componentId,  QString& paramKey,  QVariant& paramValue) = 0;

    QMap<int, QMap<QString, QVariant>* >  getPendingParameters() {
       return pendingParameters;
   }

    QMap<int, QMap<QString, QVariant>* > getOnboardParameters() {
       return onboardParameters;
   }


    void setUASID(int anId) {  this->uasId = anId; }

signals:
    
public slots:


protected:
    int     uasId; ///< The UAS / MAV to which this data model pertains
    QMap<int, QMap<QString, QVariant>* > pendingParameters; ///< Changed values that have not yet been transmitted to the UAS, by component ID
    QMap<int, QMap<QString, QVariant>* > onboardParameters; ///< All parameters confirmed to be stored onboard the UAS, by component ID

    
};

#endif // UASPARAMETERDATAMODEL_H
