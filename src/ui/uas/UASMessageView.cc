/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

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
        msgWidget->appendHtml(message->getFormatedText());
        // Ensure text area scrolls correctly
        scroller->setValue(scroller->maximum());
        msgWidget->setUpdatesEnabled(true);
    }
}
