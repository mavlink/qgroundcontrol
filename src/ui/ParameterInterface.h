#ifndef PARAMETERINTERFACE_H
#define PARAMETERINTERFACE_H

#include <QtGui/QWidget>
#include "ui_ParameterInterface.h"
#include "UASInterface.h"

namespace Ui {
    class ParameterInterface;
}

class ParameterInterface : public QWidget {
    Q_OBJECT
public:
    explicit ParameterInterface(QWidget *parent = 0);
    virtual ~ParameterInterface();

public slots:
    void addUAS(UASInterface* uas);
    void addComponent(UASInterface* uas, int component, QString componentName);
    void receiveParameter(UASInterface* uas, int component, QString parameterName, float value);
    void setParameter(UASInterface* uas, int component, QString parameterName, float value);
    void commitParameter(UASInterface* uas, int component, QString parameterName, float value);

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::parameterWidget *m_ui;
};

#endif // PARAMETERINTERFACE_H
