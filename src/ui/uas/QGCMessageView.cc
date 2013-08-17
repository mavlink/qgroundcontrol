#include "QGCMessageView.h"
#include "ui_QGCMessageView.h"

#include "UASManager.h"
#include "QGCUnconnectedInfoWidget.h"
#include <QMenu>
#include <QScrollBar>

QGCMessageView::QGCMessageView(QWidget *parent) :
    QWidget(parent),
    activeUAS(NULL),
    clearAction(new QAction(tr("Clear Text"), this)),
    ui(new Ui::QGCMessageView)
{
    setObjectName("QUICKVIEW_MESSAGE_CONSOLE");

    ui->setupUi(this);
    setStyleSheet("QScrollArea { border: 0px; } QPlainTextEdit { border: 0px }");

    // Construct initial widget
    connectWidget = new QGCUnconnectedInfoWidget(this);
    ui->horizontalLayout->addWidget(connectWidget);
    ui->plainTextEdit->hide();

    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
}

QGCMessageView::~QGCMessageView()
{
    delete ui;
}

void QGCMessageView::setActiveUAS(UASInterface* uas)
{
    if (!uas)
        return;

    if (activeUAS) {
        disconnect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
        ui->plainTextEdit->clear();
    } else {

        // First time UI setup, clear layout
        ui->horizontalLayout->removeWidget(connectWidget);
        connectWidget->deleteLater();
        ui->plainTextEdit->show();

        connect(clearAction, SIGNAL(triggered()), ui->plainTextEdit, SLOT(clear()));
    }

    connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
    activeUAS = uas;
}

void QGCMessageView::handleTextMessage(int uasid, int componentid, int severity, QString text)
{
    // XXX color messages according to severity

    QPlainTextEdit *msgWidget = ui->plainTextEdit;

    //turn off updates while we're appending content to avoid breaking the autoscroll behavior
    msgWidget->setUpdatesEnabled(false);
    QScrollBar *scroller = msgWidget->verticalScrollBar();

    UASInterface *uas = UASManager::instance()->getUASForId(uasid);
    msgWidget->appendHtml(QString("<font color=\"%1\">[%2:%3] %4</font>\n").arg(uas->getColor().name()).arg(uas->getUASName()).arg(componentid).arg(text));
    // Ensure text area scrolls correctly
    scroller->setValue(scroller->maximum());
    msgWidget->setUpdatesEnabled(true);

}

void QGCMessageView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(clearAction);
    menu.exec(event->globalPos());
}
