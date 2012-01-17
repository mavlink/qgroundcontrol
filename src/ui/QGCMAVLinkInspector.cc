#include <QList>

#include "QGCMAVLink.h"
#include "QGCMAVLinkInspector.h"
#include "UASManager.h"
#include "ui_QGCMAVLinkInspector.h"

#include <QDebug>

const float QGCMAVLinkInspector::updateHzLowpass = 0.2f;
const unsigned int QGCMAVLinkInspector::updateInterval = 1000U;

QGCMAVLinkInspector::QGCMAVLinkInspector(MAVLinkProtocol* protocol, QWidget *parent) :
    QWidget(parent),
    selectedSystemID(0),
    selectedComponentID(0),
    ui(new Ui::QGCMAVLinkInspector)
{
    ui->setupUi(this);

    /* Insert system */
    ui->systemComboBox->addItem(tr("All Systems"), 0);
    ui->componentComboBox->addItem(tr("All Components"), 0);

    mavlink_message_info_t msg[256] = MAVLINK_MESSAGE_INFO;
    memcpy(messageInfo, msg, sizeof(mavlink_message_info_t)*256);
    memset(receivedMessages, 0, sizeof(mavlink_message_t)*256);

    connect(protocol, SIGNAL(messageReceived(LinkInterface*,mavlink_message_t)), this, SLOT(receiveMessage(LinkInterface*,mavlink_message_t)));
    QStringList header;
    header << tr("Name");
    header << tr("Value");
    header << tr("Type");
    ui->treeWidget->setHeaderLabels(header);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(refreshView()));


    // ARM UI
    connect(ui->systemComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectDropDownMenuSystem(int)));
    connect(ui->componentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectDropDownMenuComponent(int)));

    // ARM external connections
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addSystem(UASInterface*)));

    // Start
    updateTimer.start(updateInterval);
}

void QGCMAVLinkInspector::addSystem(UASInterface* uas)
{
    ui->systemComboBox->addItem(uas->getUASName(), uas->getUASID());
}

void QGCMAVLinkInspector::selectDropDownMenuSystem(int dropdownid)
{
    selectedSystemID = ui->systemComboBox->itemData(dropdownid).toInt();
    rebuildComponentList();
}

void QGCMAVLinkInspector::selectDropDownMenuComponent(int dropdownid)
{
    selectedComponentID = ui->componentComboBox->itemData(dropdownid).toInt();
}

void QGCMAVLinkInspector::rebuildComponentList()
{
    ui->componentComboBox->clear();

    // Fill
    UASInterface* uas = UASManager::instance()->getUASForId(selectedSystemID);
    if (uas)
    {
        QMap<int, QString> components = uas->getComponents();

        foreach (int id, components.keys())
        {
            QString name = components.value(id);
            ui->componentComboBox->addItem(name, id);
        }
    }
}

void QGCMAVLinkInspector::addComponent(int uas, int component, const QString& name)
{
    Q_UNUSED(component);
    Q_UNUSED(name);
    if (uas != selectedSystemID) return;

    rebuildComponentList();
}

void QGCMAVLinkInspector::refreshView()
{
    for (int i = 0; i < 256; ++i)//mavlink_message_t msg, receivedMessages)
    {
        mavlink_message_t* msg = receivedMessages+i;
        // Ignore NULL values
        if (!msg) continue;
        // Update the tree view
        QString messageName("%1 (%2 Hz, #%3)");
        float msgHz = (1.0f-updateHzLowpass)*messagesHz.value(msg->msgid, 0) + updateHzLowpass*((float)messageCount.value(msg->msgid, 0))/((float)updateInterval/1000.0f);
        messagesHz.insert(msg->msgid, msgHz);
        messageName = messageName.arg(messageInfo[msg->msgid].name).arg(msgHz, 3, 'f', 1).arg(msg->msgid);
        messageCount.insert(msg->msgid, 0);
        if (!treeWidgetItems.contains(msg->msgid))
        {
            QStringList fields;
            fields << messageName;
            QTreeWidgetItem* widget = new QTreeWidgetItem(fields);
            widget->setFirstColumnSpanned(true);

            for (unsigned int i = 0; i < messageInfo[msg->msgid].num_fields; ++i)
            {
                QTreeWidgetItem* field = new QTreeWidgetItem();
                widget->addChild(field);
            }

            treeWidgetItems.insert(msg->msgid, widget);
            ui->treeWidget->addTopLevelItem(widget);
        }

        // Set Hz
        QTreeWidgetItem* message = treeWidgetItems.value(msg->msgid);
        message->setFirstColumnSpanned(true);
        message->setData(0, Qt::DisplayRole, QVariant(messageName));
        for (unsigned int i = 0; i < messageInfo[msg->msgid].num_fields; ++i)
        {
            updateField(msg->msgid, i, message->child(i));
        }
    }
}

void QGCMAVLinkInspector::receiveMessage(LinkInterface* link,mavlink_message_t message)
{
    Q_UNUSED(link);
    if (selectedSystemID != 0 && selectedSystemID != message.sysid) return;
    if (selectedComponentID != 0 && selectedComponentID != message.compid) return;
    // Only overwrite if system filter is set
    memcpy(receivedMessages+message.msgid, &message, sizeof(mavlink_message_t));

    quint64 receiveTime = QGC::groundTimeMilliseconds();
    if (lastMessageUpdate.contains(message.msgid))
    {
        int count = messageCount.value(message.msgid, 0);
        messageCount.insert(message.msgid, count+1);
    }

    lastMessageUpdate.insert(message.msgid, receiveTime);
}

QGCMAVLinkInspector::~QGCMAVLinkInspector()
{
    delete ui;
}

void QGCMAVLinkInspector::updateField(int msgid, int fieldid, QTreeWidgetItem* item)
{
    // Add field tree widget item
    item->setData(0, Qt::DisplayRole, QVariant(messageInfo[msgid].fields[fieldid].name));
    uint8_t* m = ((uint8_t*)(receivedMessages+msgid))+8;
    switch (messageInfo[msgid].fields[fieldid].type)
    {
    case MAVLINK_TYPE_CHAR:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            char* str = (char*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            str[messageInfo[msgid].fields[fieldid].array_length-1] = '\0';
            QString string(str);
            item->setData(2, Qt::DisplayRole, "char");
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single char
            char b = *((char*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, QString("char[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, b);
        }
        break;
    case MAVLINK_TYPE_UINT8_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint8_t* nums = m+messageInfo[msgid].fields[fieldid].wire_offset;
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("uint8_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            uint8_t u = *(m+messageInfo[msgid].fields[fieldid].wire_offset);
            item->setData(2, Qt::DisplayRole, "uint8_t");
            item->setData(1, Qt::DisplayRole, u);
        }
        break;
    case MAVLINK_TYPE_INT8_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int8_t* nums = (int8_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("int8_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            int8_t n = *((int8_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "int8_t");
            item->setData(1, Qt::DisplayRole, n);
        }
        break;
    case MAVLINK_TYPE_UINT16_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint16_t* nums = (uint16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("uint16_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            uint16_t n = *((uint16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "uint16_t");
            item->setData(1, Qt::DisplayRole, n);
        }
        break;
    case MAVLINK_TYPE_INT16_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int16_t* nums = (int16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("int16_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            int16_t n = *((int16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "int16_t");
            item->setData(1, Qt::DisplayRole, n);
        }
        break;
    case MAVLINK_TYPE_UINT32_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint32_t* nums = (uint32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("uint32_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            float n = *((uint32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "uint32_t");
            item->setData(1, Qt::DisplayRole, n);
        }
        break;
    case MAVLINK_TYPE_INT32_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int32_t* nums = (int32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("int32_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            int32_t n = *((int32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "int32_t");
            item->setData(1, Qt::DisplayRole, n);
        }
        break;
    case MAVLINK_TYPE_FLOAT:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            float* nums = (float*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("float[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            float f = *((float*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "float");
            item->setData(1, Qt::DisplayRole, f);
        }
        break;
    case MAVLINK_TYPE_DOUBLE:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            double* nums = (double*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("double[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            double f = *((double*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "double");
            item->setData(1, Qt::DisplayRole, f);
        }
        break;
    case MAVLINK_TYPE_UINT64_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint64_t* nums = (uint64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("uint64_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            uint64_t n = *((uint64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "uint64_t");
            item->setData(1, Qt::DisplayRole, (quint64) n);
        }
        break;
    case MAVLINK_TYPE_INT64_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int64_t* nums = (int64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(nums[j]);
            }
            item->setData(2, Qt::DisplayRole, QString("int64_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            int64_t n = *((int64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            item->setData(2, Qt::DisplayRole, "int64_t");
            item->setData(1, Qt::DisplayRole, (qint64) n);
        }
        break;
    }
}
