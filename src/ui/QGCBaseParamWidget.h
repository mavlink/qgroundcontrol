#ifndef QGCBASEPARAMWIDGET_H
#define QGCBASEPARAMWIDGET_H

#include <QVariant>
#include <QWidget>




//forward declarations
class QGCUASParamManagerInterface;
class UASInterface;


class QGCBaseParamWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QGCBaseParamWidget(QWidget *parent = 0);
    virtual QGCBaseParamWidget* initWithUAS(UASInterface* uas);///< Two-stage construction: initialize this object
    virtual void setUAS(UASInterface* uas);///< Allows swapping the underlying UAS

protected:
    virtual void setParameterStatusMsg(const QString& msg) = 0;
    virtual void layoutWidget() = 0;///< Layout the appearance of this widget
    virtual void connectViewSignalsAndSlots() = 0;///< Connect view signals/slots as needed
    virtual void disconnectViewSignalsAndSlots() = 0;///< Disconnect view signals/slots as needed
    virtual void connectToParamManager(); ///>Connect to any required param manager signals
    virtual void disconnectFromParamManager(); ///< Disconnect from any connected param manager signals

signals:

public slots:
    virtual void handleOnboardParamUpdate(int component,const QString& parameterName, QVariant value) = 0;
    virtual void handlePendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending) = 0;
    virtual void handleOnboardParameterListUpToDate() = 0;
    virtual void handleParamStatusMsgUpdate(QString msg, int level) = 0;
    /** @brief Clear the rendering of onboard parameters */
    virtual void clearOnboardParamDisplay() = 0;
    /** @brief Clear the rendering of pending parameters */
    virtual void clearPendingParamDisplay() = 0;

    /** @brief Request list of parameters from MAV */
    virtual void requestOnboardParamsUpdate();

    /** @brief Request single parameter from MAV */
    virtual void requestOnboardParamUpdate(QString parameterName);


    /** @brief Store parameters to a file */
    virtual void saveParametersToFile();
    /** @brief Load parameters from a file */
    virtual void loadParametersFromFile();

protected:
    QGCUASParamManagerInterface* paramMgr;
    UASInterface* mav;
    QString     updatingParamNameLock; ///< Name of param currently being updated-- used for reducing echo on param change

};

#endif // QGCBASEPARAMWIDGET_H
