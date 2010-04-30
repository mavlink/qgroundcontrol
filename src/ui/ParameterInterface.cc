#include <QTreeWidget>

#include "ParameterInterface.h"
#include "ParamTreeModel.h"
#include "UASManager.h"
#include "ui_ParameterInterface.h"

#include <QDebug>

ParameterInterface::ParameterInterface(QWidget *parent) :
        QWidget(parent),
        m_ui(new Ui::parameterWidget)
{
    m_ui->setupUi(this);
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));

    tree = new ParamTreeModel();

    treeView = new QTreeView(this);
    treeView->setModel(tree);

    QStackedWidget* stack = m_ui->stackedWidget;
    stack->addWidget(treeView);
    stack->setCurrentWidget(treeView);

}

ParameterInterface::~ParameterInterface()
{
    delete m_ui;
}

/**
 *
 * @param uas System to add to list
 */
void ParameterInterface::addUAS(UASInterface* uas)
{
    m_ui->vehicleComboBox->addItem(uas->getUASName());

    mav = uas;

    // Setup UI connections
    connect(m_ui->readParamsButton, SIGNAL(clicked()), this, SLOT(requestParameterList()));

    // Connect signals
    connect(uas, SIGNAL(parameterChanged(int,int,QString,float)), this, SLOT(receiveParameter(int,int,QString,float)));
    //if (!paramViews.contains(uas))
    //{
    //uasViews.insert(uas, new UASView(uas, this));
    //listLayout->addWidget(uasViews.value(uas));

    //}
}

void ParameterInterface::requestParameterList()
{
    mav->requestParameters();
}

/**
 *
 * @param uas System which has the component
 * @param component id of the component
 * @param componentName human friendly name of the component
 */
void ParameterInterface::addComponent(UASInterface* uas, int component, QString componentName)
{
    Q_UNUSED(uas);
}

void ParameterInterface::receiveParameter(int uas, int component, QString parameterName, float value)
{
    Q_UNUSED(uas);
    // Insert parameter into map
    tree->appendParam(component, parameterName, value);
    // Refresh view
    treeView->update();
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
