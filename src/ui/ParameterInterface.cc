#include <QTreeWidget>

#include "ParameterInterface.h"
#include "ParamTreeModel.h"
#include "UASManager.h"
#include "ui_ParameterInterface.h"
#include "QGCSensorSettingsWidget.h"

#include <QDebug>

ParameterInterface::ParameterInterface(QWidget *parent) :
        QWidget(parent),
        paramWidgets(new QMap<int, QGCParamWidget*>()),
        curr(-1),
        m_ui(new Ui::parameterWidget)
{
    m_ui->setupUi(this);
    // Make sure the combo box is empty
    // because else indices get messed up
    m_ui->vehicleComboBox->clear();

    // Setup UI connections
    connect(m_ui->vehicleComboBox, SIGNAL(activated(int)), this, SLOT(selectUAS(int)));

    // Setup MAV connections
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
}

ParameterInterface::~ParameterInterface()
{
    delete m_ui;
}

void ParameterInterface::selectUAS(int index)
{
    // FIXME plus 2 shouldn't be there
    m_ui->stackedWidget->setCurrentIndex(index+2);
    m_ui->sensorSettings->setCurrentIndex(index+2);
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


    QGCSensorSettingsWidget* sensor = new QGCSensorSettingsWidget(uas, this);
    m_ui->sensorSettings->addWidget(sensor);

    // Set widgets as default
    if (curr == -1)
    {
        // Clear
        m_ui->sensorSettings->setCurrentWidget(sensor);
        m_ui->stackedWidget->setCurrentWidget(param);
        curr = 0;
    }
}

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
