#include <QTreeWidget>

#include "ParameterInterface.h"
#include "ParamTreeModel.h"
#include "UASManager.h"
#include "ui_ParameterInterface.h"

#include <QDebug>

ParameterInterface::ParameterInterface(QWidget *parent) :
        QWidget(parent),
        paramWidgets(new QMap<int, QGCParamWidget*>()),
        curr(-1),
        m_ui(new Ui::parameterWidget)
{
    m_ui->setupUi(this);

    // Setup UI connections
    connect(m_ui->vehicleComboBox, SIGNAL(activated(int)), this, SLOT(selectUAS(int)));
    connect(m_ui->readParamsButton, SIGNAL(clicked()), this, SLOT(requestParameterList()));

    // Setup MAV connections
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
}

ParameterInterface::~ParameterInterface()
{
    delete m_ui;
}

void ParameterInterface::selectUAS(int index)
{
    m_ui->stackedWidget->setCurrentIndex(index);
    curr = index;
}

/**
 *
 * @param uas System to add to list
 */
void ParameterInterface::addUAS(UASInterface* uas)
{
    m_ui->vehicleComboBox->addItem(uas->getUASName());

    QGCParamWidget* param = new QGCParamWidget(uas, this);
    paramWidgets->insert(uas->getUASID(), param);
    m_ui->stackedWidget->addWidget(param);
    if (curr == -1)
    {
        m_ui->stackedWidget->setCurrentWidget(param);
        curr = uas->getUASID();
        qDebug() << "first widget";
    }

    // Connect signals
    connect(uas, SIGNAL(parameterChanged(int,int,QString,float)), this, SLOT(addParameter(int,int,QString,float)));
}

void ParameterInterface::requestParameterList()
{
    UASInterface* mav;
    QGCParamWidget* widget = paramWidgets->value(curr);
    if (widget != NULL)
    {
        mav = widget->getUAS();
        mav->requestParameters();
        // Clear view
        widget->clear();
    }
}

/**
 *
 * @param uas System which has the component
 * @param component id of the component
 * @param componentName human friendly name of the component
 */
void ParameterInterface::addComponent(int uas, int component, QString componentName)
{
    QGCParamWidget* widget = paramWidgets->value(uas);
    if (widget != NULL)
    {
        widget->addComponent(component, componentName);
    }
}

void ParameterInterface::addParameter(int uas, int component, QString parameterName, float value)
{
    QGCParamWidget* widget = paramWidgets->value(uas);
    if (widget != NULL)
    {
        widget->addParameter(component, parameterName, value);
    }
}

/**
 * @param uas system
 * @param component the subsystem which has the parameter
 * @param parameterName name of the parameter, as delivered by the system
 * @param value value of the parameter
 */
void ParameterInterface::setParameter(UASInterface* uas, int component, QString parameterName, float value)
{
    Q_UNUSED(uas);
}

/**
 * @param
 */
void ParameterInterface::commitParameter(UASInterface* uas, int component, QString parameterName, float value)
{

}

/*
void ParameterInterface::commitParameters(UASInterface* uas)
{

}*/

/**
 *
 */


void ParameterInterface::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
