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

    components = new QMap<int, QTreeWidgetItem*>();
    tree = new ParamTreeModel();

    //treeView = new QTreeView(this);
    //treeView->setModel(tree);

    treeWidget = new QTreeWidget(this);
    QStringList headerItems;
    headerItems.append("Parameter");
    headerItems.append("Value");
    treeWidget->setHeaderLabels(headerItems);

    QStackedWidget* stack = m_ui->stackedWidget;
    //stack->addWidget(treeView);
    //stack->setCurrentWidget(treeView);
    stack->addWidget(treeWidget);
    stack->setCurrentWidget(treeWidget);

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
    // Clear view
    treeWidget->clear();
}

/**
 *
 * @param uas System which has the component
 * @param component id of the component
 * @param componentName human friendly name of the component
 */
void ParameterInterface::addComponent(int uas, int component, QString componentName)
{
    Q_UNUSED(uas);
    QStringList list;
    list.append(componentName);
    list.append(QString::number(component));
    QTreeWidgetItem* comp = new QTreeWidgetItem(list);
    bool updated = false;
    if (components->contains(component)) updated = true;
    components->insert(component, comp);
    if (!updated) treeWidget->addTopLevelItem(comp);
}

void ParameterInterface::receiveParameter(int uas, int component, QString parameterName, float value)
{
    Q_UNUSED(uas);
    // Insert parameter into map
    //tree->appendParam(component, parameterName, value);
    QStringList plist;
    plist.append(parameterName);
    plist.append(QString::number(value));
    QTreeWidgetItem* item = new QTreeWidgetItem(plist);

    // Get component
    if (!components->contains(component))
    {
        addComponent(uas, component, "Component #" + QString::number(component));
    }
    components->value(component)->addChild(item);
    //treeWidget->addTopLevelItem(new QTreeWidgetItem(list));
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
