#ifndef PARAMETERINTERFACE_H
#define PARAMETERINTERFACE_H

#include <QtGui/QWidget>

#include "ui_ParameterInterface.h"
#include "UASInterface.h"
#include "ParamTreeModel.h"
#include "QGCParamWidget.h"

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
    void selectUAS(int index);

protected:
    virtual void changeEvent(QEvent *e);
    QMap<int, QGCParamWidget*>* paramWidgets;
    int curr;

private:
    Ui::parameterWidget *m_ui;
};

#endif // PARAMETERINTERFACE_H
