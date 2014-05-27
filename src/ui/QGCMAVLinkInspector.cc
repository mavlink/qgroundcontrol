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
    _protocol(protocol),
    selectedSystemID(0),
    selectedComponentID(0),
    ui(new Ui::QGCMAVLinkInspector)
{
    ui->setupUi(this);

    // Make sure "All" is an option for both the system and components
    ui->systemComboBox->addItem(tr("All"), 0);
    ui->componentComboBox->addItem(tr("All"), 0);

	// Store metadata for all MAVLink messages.
	mavlink_message_info_t msg_infos[256] = MAVLINK_MESSAGE_INFO;
	memcpy(messageInfo, msg_infos, sizeof(mavlink_message_info_t)*256);

	// Initialize the received data for all messages to invalid (0xFF)
    memset(receivedMessages, 0xFF, sizeof(mavlink_message_t)*256);

	// Set up the column headers for the message listing
    QStringList header;
    header << tr("Name");
    header << tr("Value");
    header << tr("Type");
    ui->treeWidget->setHeaderLabels(header);

    // Set up the column headers for the rate listing
    QStringList rateHeader;
    rateHeader << tr("Name");
    rateHeader << tr("#ID");
    rateHeader << tr("Rate");
    ui->rateTreeWidget->setHeaderLabels(rateHeader);
    connect(ui->rateTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(rateTreeItemChanged(QTreeWidgetItem*,int)));
    ui->rateTreeWidget->hide();

    // Connect the UI
    connect(ui->systemComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectDropDownMenuSystem(int)));
    connect(ui->componentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectDropDownMenuComponent(int)));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clearView()));

    // Connect external connections
//    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addSystem(UASInterface*)));
    connect(protocol, SIGNAL(messageReceived(LinkInterface*,mavlink_message_t)), this, SLOT(receiveMessage(LinkInterface*,mavlink_message_t)));

    // Attach the UI's refresh rate to a timer.
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(refreshView()));
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

    if (selectedSystemID != 0 && selectedComponentID != 0) {
        ui->rateTreeWidget->show();
    } else {
        ui->rateTreeWidget->hide();
    }
}

void QGCMAVLinkInspector::selectDropDownMenuComponent(int dropdownid)
{
    selectedComponentID = ui->componentComboBox->itemData(dropdownid).toInt();

    if (selectedSystemID != 0 && selectedComponentID != 0) {
        ui->rateTreeWidget->show();
    } else {
        ui->rateTreeWidget->hide();
    }
}

void QGCMAVLinkInspector::rebuildComponentList()
{
    ui->componentComboBox->clear();
    components.clear();

    ui->componentComboBox->addItem(tr("All"), 0);
}

void QGCMAVLinkInspector::addComponent(int uas, int component, const QString& name)
{
    Q_UNUSED(component);
    Q_UNUSED(name);
    if (uas != selectedSystemID) return;

    rebuildComponentList();
}

/**
 * Reset the view. This entails clearing all data structures and resetting data from already-
 * received messages.
 */
void QGCMAVLinkInspector::clearView()
{
    memset(receivedMessages, 0xFF, sizeof(mavlink_message_t)*256);
    onboardMessageInterval.clear();
	lastMessageUpdate.clear();
	messagesHz.clear();
    treeWidgetItems.clear();
    ui->treeWidget->clear();
    ui->rateTreeWidget->clear();
}

void QGCMAVLinkInspector::refreshView()
{
    for (int i = 0; i < 256; ++i)//mavlink_message_t msg, receivedMessages)
    {
        mavlink_message_t* msg = receivedMessages+i;
        // Ignore NULL values
        if (msg->msgid == 0xFF) continue;
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

    if (selectedSystemID == 0 || selectedComponentID == 0)
    {
        return;
    }

    for (int i = 0; i < 256; ++i)//mavlink_message_t msg, receivedMessages)
    {
        const char* msgname = messageInfo[i].name;

        int namelen = strnlen(msgname, 5);

        if (namelen < 3) {
            continue;
        }

        if (!strcmp(msgname, "EMPTY")) {
            continue;
        }

        // Update the tree view
        QString messageName("%1");
        messageName = messageName.arg(msgname);
        if (!rateTreeWidgetItems.contains(i))
        {
            QStringList fields;
            fields << messageName;
            fields << QString("%1").arg(i);
            fields << "OFF / --- Hz";
            QTreeWidgetItem* widget = new QTreeWidgetItem(fields);
            widget->setFlags(widget->flags() | Qt::ItemIsEditable);
            rateTreeWidgetItems.insert(i, widget);
            ui->rateTreeWidget->addTopLevelItem(widget);
        }

        // Set Hz
        //QTreeWidgetItem* message = rateTreeWidgetItems.value(i);
        //message->setData(0, Qt::DisplayRole, QVariant(messageName));
    }
}

void QGCMAVLinkInspector::receiveMessage(LinkInterface* link,mavlink_message_t message)
{
    Q_UNUSED(link);
    if (selectedSystemID != 0 && selectedSystemID != message.sysid) return;
    if (selectedComponentID != 0 && selectedComponentID != message.compid) return;
    // Only overwrite if system filter is set
    memcpy(receivedMessages+message.msgid, &message, sizeof(mavlink_message_t));

    // Add not yet seen systems / components
    if (!systems.contains(message.sysid)) {
        systems.insert(message.sysid, message.sysid);
        ui->systemComboBox->addItem(tr("MAV%1").arg(message.sysid), message.sysid);
    }

    if (!components.contains(message.compid)) {
        components.insert(message.compid, message.compid);
        ui->componentComboBox->addItem(tr("COMP%1").arg(message.compid), message.compid);
    }

    quint64 receiveTime = QGC::groundTimeMilliseconds();
    if (lastMessageUpdate.contains(message.msgid))
    {
        int count = messageCount.value(message.msgid, 0);
        messageCount.insert(message.msgid, count+1);
    }

    lastMessageUpdate.insert(message.msgid, receiveTime);

    if (selectedSystemID == 0 || selectedComponentID == 0) {
        return;
    }

    switch (message.msgid) {
        case MAVLINK_MSG_ID_DATA_STREAM:
            {
                mavlink_data_stream_t stream;
                mavlink_msg_data_stream_decode(&message, &stream);
                onboardMessageInterval.insert(stream.stream_id, stream.message_rate);
            }
            break;

    }
}

void QGCMAVLinkInspector::changeStreamInterval(int msgid, int interval) {
    //REQUEST_DATA_STREAM
    if (selectedSystemID == 0 || selectedComponentID == 0) {
        return;
    }

    mavlink_request_data_stream_t stream;
    stream.target_system = selectedSystemID;
    stream.target_component = selectedComponentID;
    stream.req_stream_id = msgid;
    stream.req_message_rate = interval;
    stream.start_stop = (interval > 0);

    mavlink_message_t msg;
    mavlink_msg_request_data_stream_encode(_protocol->getSystemId(), _protocol->getComponentId(), &msg, &stream);

    _protocol->sendMessage(msg);
}

void QGCMAVLinkInspector::rateTreeItemChanged(QTreeWidgetItem* paramItem, int column)
{
    if (paramItem && column > 0) {

        int key = paramItem->data(1, Qt::DisplayRole).toInt();
        QVariant value = paramItem->data(2, Qt::DisplayRole);
        float interval = 1000 / value.toFloat();

        qDebug() << "Stream " << key << "interval" << interval;

        changeStreamInterval(key, interval);
    }
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
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
