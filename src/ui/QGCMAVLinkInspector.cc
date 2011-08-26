#include <QList>

#include "QGCMAVLinkInspector.h"
#include "ui_QGCMAVLinkInspector.h"

#include <QDebug>

QGCMAVLinkInspector::QGCMAVLinkInspector(MAVLinkProtocol* protocol, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCMAVLinkInspector)
{
    ui->setupUi(this);
    connect(protocol, SIGNAL(messageReceived(LinkInterface*,mavlink_message_t)), this, SLOT(receiveMessage(LinkInterface*,mavlink_message_t)));
    ui->treeWidget->setColumnCount(3);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(refreshView()));
    updateTimer.start(1000);
}

void QGCMAVLinkInspector::refreshView()
{
    foreach (mavlink_message_t msg, receivedMessages.values())
    {
        // Update the tree view
        if (!treeWidgetItems.contains(msg.msgid))
        {
            QString messageName("MSG (#%1)");
            messageName = messageName.arg(msg.msgid);
            QStringList fields;
            fields << messageName;
            fields << "";
            fields << "";
            QTreeWidgetItem* widget = new QTreeWidgetItem(fields);
            treeWidgetItems.insert(msg.msgid, widget);
            ui->treeWidget->addTopLevelItem(widget);
        }
    }

//    // List all available MAVLink messages
//    QList<QTreeWidgetItem *> items;
//    for (int i = 0; i < 256; ++i)
//    {
//        QStringList stringList;
//        stringList << QString("message: %1").arg(i) << QString("") << QString("Not received yet.");
//        items.append(new QTreeWidgetItem((QTreeWidget*)0, stringList));
//    }
//    ui->treeWidget->insertTopLevelItems(0, items);
}

void QGCMAVLinkInspector::receiveMessage(LinkInterface* link,mavlink_message_t message)
{
    Q_UNUSED(link);
    receivedMessages.insert(message.msgid, message);
//    Q_UNUSED(link);
//    qDebug() << __FILE__ << __LINE__ << "GOT MAVLINK MESSAGE";
//    // Check if children exist
//    const QObjectList& fields = ui->treeWidget->children().at(message.msgid)->children();

//    if (fields.length() == 0)
//    {
//        // Create children
//    }

//    for (int i = 0; i < fields.length(); ++i)
//    {
//        // Fill in field data
//    }
}

QGCMAVLinkInspector::~QGCMAVLinkInspector()
{
    delete ui;
}
