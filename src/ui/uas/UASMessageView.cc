/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QMenu>
#include <QScrollBar>

#include "UASMessageView.h"
#include "QGCUnconnectedInfoWidget.h"
#include "ui_UASMessageView.h"

/*-------------------------------------------------------------------------------------
  UASMessageView
-------------------------------------------------------------------------------------*/

UASMessageView::UASMessageView(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::UASMessageView)
{
    _ui->setupUi(this);
}

UASMessageView::~UASMessageView()
{
    delete _ui;
}

/*-------------------------------------------------------------------------------------
  UASMessageViewWidget
-------------------------------------------------------------------------------------*/

UASMessageViewWidget::UASMessageViewWidget(UASMessageHandler* uasMessageHandler, QWidget *parent)
    : UASMessageView(parent)
    , _unconnectedWidget(NULL)
    , _uasMessageHandler(uasMessageHandler)
{
    setStyleSheet("QPlainTextEdit { border: 0px }");

    // Enable the right-click menu for the text editor. This works because the plainTextEdit
    // widget has its context menu policy set to its actions list. So any actions we add
    // to this widget's action list will be automatically displayed.
    // We only have the clear action right now.
    QAction* clearAction = new QAction(tr("Clear Messages"), this);
    connect(clearAction, &QAction::triggered, this, &UASMessageViewWidget::clearMessages);
    ui()->plainTextEdit->addAction(clearAction);
    // Connect message handler
    connect(_uasMessageHandler, &UASMessageHandler::textMessageReceived, this, &UASMessageViewWidget::handleTextMessage);
}

UASMessageViewWidget::~UASMessageViewWidget()
{

}

void UASMessageViewWidget::clearMessages()
{
    ui()->plainTextEdit->clear();
    _uasMessageHandler->clearMessages();
}

void UASMessageViewWidget::handleTextMessage(UASMessage *message)
{
    // Reset
    if(!message) {
        ui()->plainTextEdit->clear();
    } else {
        QPlainTextEdit *msgWidget = ui()->plainTextEdit;
        // Turn off updates while we're appending content to avoid breaking the autoscroll behavior
        msgWidget->setUpdatesEnabled(false);
        QScrollBar *scroller = msgWidget->verticalScrollBar();
        QString messages = message->getFormatedText();
        messages = messages.replace("<#E>", "color: #f95e5e; font: monospace;");
        messages = messages.replace("<#I>", "color: #f9b55e; font: monospace;");
        messages = messages.replace("<#N>", "color: #ffffff; font: monospace;");
        msgWidget->appendHtml(messages);
        // Ensure text area scrolls correctly
        scroller->setValue(scroller->maximum());
        msgWidget->setUpdatesEnabled(true);
    }
}
