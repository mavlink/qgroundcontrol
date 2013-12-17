#ifndef QGCPENDINGPARAMWIDGET_H
#define QGCPENDINGPARAMWIDGET_H


#include "QGCParamWidget.h"

class QGridLayout;

class QGCPendingParamWidget : public QGCParamWidget
{
    Q_OBJECT

public:
    explicit QGCPendingParamWidget(QObject* parent);

protected:
    virtual void connectToParamManager();
    virtual void disconnectFromParamManager();

    virtual void connectViewSignalsAndSlots();
    virtual void disconnectViewSignalsAndSlots();

    virtual void addActionButtonsToLayout(QGridLayout* layout);


signals:
    
public slots:
    virtual void handlePendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending);

};

#endif // QGCPENDINGPARAMWIDGET_H
