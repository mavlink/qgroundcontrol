#include "QGCMAVLinkMessageSender.h"
#include "ui_QGCMAVLinkMessageSender.h"
#include "MAVLinkProtocol.h"

QGCMAVLinkMessageSender::QGCMAVLinkMessageSender(MAVLinkProtocol* mavlink, QWidget *parent) :
    QWidget(parent),
    protocol(mavlink),
    ui(new Ui::QGCMAVLinkMessageSender)
{
    ui->setupUi(this);
    mavlink_message_info_t msg[256] = MAVLINK_MESSAGE_INFO;
    memcpy(messageInfo, msg, sizeof(mavlink_message_info_t)*256);

    QStringList header;
    header << tr("Name");
    header << tr("Value");
    header << tr("Type");
    ui->treeWidget->setHeaderLabels(header);
    createTreeView();
    connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    //refreshTimer.start(1000); // Refresh at 1 Hz interval

    connect(ui->sendButton, SIGNAL(pressed()), this, SLOT(sendMessage()));
}

void QGCMAVLinkMessageSender::refresh()
{
    // Send messages
    foreach (unsigned int i, managementItems.keys())
    {
        if (!sendTimers.contains(i))
        {
            //sendTimers.insert(i, new QTimer())
        }
    }

    // ui->treeWidget->topLevelItem(0)->children();
}

bool QGCMAVLinkMessageSender::sendMessage()
{
    return sendMessage(ui->messageIdSpinBox->value());
}

bool QGCMAVLinkMessageSender::sendMessage(unsigned int msgid)
{
    QString msgname(messageInfo[msgid].name);
    if (msgid == 0 || msgid > 255 || messageInfo[msgid].name == NULL || msgname.compare(QString("EMPTY")))
    {
        return false;
    }
    bool result = true;

    if (treeWidgetItems.contains(msgid))
    {
        // Fill message fields

        mavlink_message_t msg;
        QList<QTreeWidgetItem*> fields;// = treeWidgetItems.value(msgid)->;

        for (unsigned int i = 0; i < messageInfo[msgid].num_fields; ++i)
        {
            QTreeWidgetItem* field = fields.at(i);
            int fieldid = i;
            uint8_t* m = ((uint8_t*)(&msg))+8;
            switch (messageInfo[msgid].fields[fieldid].type)
            {
            case MAVLINK_TYPE_CHAR:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    char* str = (char*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    // Copy data
                    QString string = field->data(1, Qt::DisplayRole).toString();
                    // Copy string size
                    int len = qMin((unsigned int)string.length(), messageInfo[msgid].fields[fieldid].array_length);
                    memcpy(str, string.toStdString().c_str(), len);
                    // Enforce null termination
                    str[len-1] = '\0';
                }
                else
                {
                    // Single char
                    char* b = ((char*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
                    *b = field->data(1, Qt::DisplayRole).toChar().toAscii();
                }
                break;
            case MAVLINK_TYPE_UINT8_T:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    uint8_t* nums = m+messageInfo[msgid].fields[fieldid].wire_offset;
                    for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
                    {
                        if ((unsigned int)(field->data(1, Qt::DisplayRole).toString().split(" ").size()) > j)
                        {
                            nums[j] = field->data(1, Qt::DisplayRole).toString().split(" ").at(j).toInt();
                        }
                    }
                }
                else
                {
                    // Single value
                    uint8_t* u = (m+messageInfo[msgid].fields[fieldid].wire_offset);
                    *u = field->data(1, Qt::DisplayRole).toChar().toAscii();
                }
                break;
            case MAVLINK_TYPE_INT8_T:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    int8_t* nums = reinterpret_cast<int8_t*>((m+messageInfo[msgid].fields[fieldid].wire_offset));
                    for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
                    {
                        if ((unsigned int)(field->data(1, Qt::DisplayRole).toString().split(" ").size()) > j)
                        {
                            nums[j] = field->data(1, Qt::DisplayRole).toString().split(" ").at(j).toInt();
                        }
                    }
                }
                else
                {
                    // Single value
                    int8_t* u = reinterpret_cast<int8_t*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    *u = field->data(1, Qt::DisplayRole).toChar().toAscii();
                }
                break;
            case MAVLINK_TYPE_INT16_T:
            case MAVLINK_TYPE_UINT16_T:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    uint16_t* nums = reinterpret_cast<uint16_t*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
                    {
                        if ((unsigned int)(field->data(1, Qt::DisplayRole).toString().split(" ").size()) > j)
                        {
                            nums[j] = field->data(1, Qt::DisplayRole).toString().split(" ").at(j).toUInt();
                        }
                    }
                }
                else
                {
                    // Single value
                    uint16_t* u = reinterpret_cast<uint16_t*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    *u = field->data(1, Qt::DisplayRole).toUInt();
                }
                break;
            case MAVLINK_TYPE_INT32_T:
            case MAVLINK_TYPE_UINT32_T:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    int32_t* nums = reinterpret_cast<int32_t*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
                    {
                        if ((unsigned int)(field->data(1, Qt::DisplayRole).toString().split(" ").size()) > j)
                        {
                            nums[j] = field->data(1, Qt::DisplayRole).toString().split(" ").at(j).toUInt();
                        }
                    }
                }
                else
                {
                    // Single value
                    int32_t* u = reinterpret_cast<int32_t*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    *u = field->data(1, Qt::DisplayRole).toUInt();
                }
                break;
            case MAVLINK_TYPE_INT64_T:
            case MAVLINK_TYPE_UINT64_T:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    int64_t* nums = reinterpret_cast<int64_t*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
                    {
                        if ((unsigned int)(field->data(1, Qt::DisplayRole).toString().split(" ").size()) > j)
                        {
                            nums[j] = field->data(1, Qt::DisplayRole).toString().split(" ").at(j).toULongLong();
                        }
                    }
                }
                else
                {
                    // Single value
                    int64_t* u = reinterpret_cast<int64_t*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    *u = field->data(1, Qt::DisplayRole).toULongLong();
                }
                break;
            case MAVLINK_TYPE_FLOAT:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    float* nums = reinterpret_cast<float*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
                    {
                        if ((unsigned int)(field->data(1, Qt::DisplayRole).toString().split(" ").size()) > j)
                        {
                            nums[j] = field->data(1, Qt::DisplayRole).toString().split(" ").at(j).toFloat();
                        }
                    }
                }
                else
                {
                    // Single value
                    float* u = reinterpret_cast<float*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    *u = field->data(1, Qt::DisplayRole).toFloat();
                }
                break;
            case MAVLINK_TYPE_DOUBLE:
                if (messageInfo[msgid].fields[fieldid].array_length > 0)
                {
                    double* nums = reinterpret_cast<double*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
                    {
                        if ((unsigned int)(field->data(1, Qt::DisplayRole).toString().split(" ").size()) > j)
                        {
                            nums[j] = field->data(1, Qt::DisplayRole).toString().split(" ").at(j).toDouble();
                        }
                    }
                }
                else
                {
                    // Single value
                    double* u = reinterpret_cast<double*>(m+messageInfo[msgid].fields[fieldid].wire_offset);
                    *u = field->data(1, Qt::DisplayRole).toDouble();
                }
                break;
            }
        }

        // Send message
        protocol->sendMessage(msg);
    }
    else
    {
        result = false;
    }

    return result;
}

QGCMAVLinkMessageSender::~QGCMAVLinkMessageSender()
{
    delete ui;
}

void QGCMAVLinkMessageSender::createTreeView()
{
    for (int i = 0; i < 256; ++i)//mavlink_message_t msg, receivedMessages)
    {
        // Update the tree view
        QString messageName("%1 (%2 Hz, #%3)");
        float msgHz = messagesHz.value(i, 0);

        // Ignore non-existent messages
        if (QString(messageInfo[i].name) == "EMPTY") continue;

        messageName = messageName.arg(messageInfo[i].name).arg(msgHz, 3, 'f', 1).arg(i);
        if (!treeWidgetItems.contains(i))
        {
            QStringList fields;
            fields << messageName;
            QTreeWidgetItem* widget = new QTreeWidgetItem(fields);
            widget->setFirstColumnSpanned(true);

            for (unsigned int j = 0; j < messageInfo[i].num_fields; ++j)
            {
                QTreeWidgetItem* field = new QTreeWidgetItem();
                widget->addChild(field);
            }

            treeWidgetItems.insert(i, widget);
            ui->treeWidget->addTopLevelItem(widget);


            QTreeWidgetItem* message = widget;//treeWidgetItems.value(msg->msgid);
            message->setFirstColumnSpanned(true);
            message->setData(0, Qt::DisplayRole, QVariant(messageName));
            for (unsigned int j = 0; j < messageInfo[i].num_fields; ++j)
            {
                createField(i, j, message->child(j));
            }
            // Add management fields, such as update rate and send button
            //            QTreeWidgetItem* management = new QTreeWidgetItem();
            //            widget->addChild(management);
            //            management->setData(0, Qt::DisplayRole, "Rate:");
            //            management->setData(1, Qt::DisplayRole, 0);
            //            management->setData(2, Qt::DisplayRole, "Hz");
            //            managementItems.insert(i, management);
        }
    }
}

void QGCMAVLinkMessageSender::createField(int msgid, int fieldid, QTreeWidgetItem* item)
{
    // Add field tree widget item
    item->setData(0, Qt::DisplayRole, QVariant(messageInfo[msgid].fields[fieldid].name));
    //uint8_t* m = ((uint8_t*)(receivedMessages+msgid))+8;
    switch (messageInfo[msgid].fields[fieldid].type)
    {
    case MAVLINK_TYPE_CHAR:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            item->setData(2, Qt::DisplayRole, "char");
            item->setData(1, Qt::DisplayRole, "");
        }
        else
        {
            // Single char
            item->setData(2, Qt::DisplayRole, QString("char[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, "");
        }
        break;
    case MAVLINK_TYPE_UINT8_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("uint8_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "uint8_t");
            item->setData(1, Qt::DisplayRole, 0);
        }
        break;
    case MAVLINK_TYPE_INT8_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("int8_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "int8_t");
            item->setData(1, Qt::DisplayRole, 0);
        }
        break;
    case MAVLINK_TYPE_UINT16_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("uint16_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "uint16_t");
            item->setData(1, Qt::DisplayRole, 0);
        }
        break;
    case MAVLINK_TYPE_INT16_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("int16_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "int16_t");
            item->setData(1, Qt::DisplayRole, 0);
        }
        break;
    case MAVLINK_TYPE_UINT32_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("uint32_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "uint32_t");
            item->setData(1, Qt::DisplayRole, 0);
        }
        break;
    case MAVLINK_TYPE_INT32_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("int32_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "int32_t");
            item->setData(1, Qt::DisplayRole, 0);
        }
        break;
    case MAVLINK_TYPE_FLOAT:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0.0f);
            }
            item->setData(2, Qt::DisplayRole, QString("float[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "float");
            item->setData(1, Qt::DisplayRole, 0.0f);
        }
        break;
    case MAVLINK_TYPE_DOUBLE:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("double[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "double");
            item->setData(1, Qt::DisplayRole, 0.0);
        }
        break;
    case MAVLINK_TYPE_UINT64_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("uint64_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "uint64_t");
            item->setData(1, Qt::DisplayRole, (quint64) 0);
        }
        break;
    case MAVLINK_TYPE_INT64_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            // Enforce null termination
            QString tmp("%1, ");
            QString string;
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                string += tmp.arg(0);
            }
            item->setData(2, Qt::DisplayRole, QString("int64_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length));
            item->setData(1, Qt::DisplayRole, string);
        }
        else
        {
            // Single value
            item->setData(2, Qt::DisplayRole, "int64_t");
            item->setData(1, Qt::DisplayRole, (qint64) 0);
        }
        break;
    }
}
