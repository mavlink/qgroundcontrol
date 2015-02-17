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

#include "QGCToolBar.h"
#include "QGCMessageView.h"
#include "QGCUnconnectedInfoWidget.h"
#include "QGCMessageHandler.h"
#include "ui_QGCMessageView.h"

/*-------------------------------------------------------------------------------------
  QGCMessageView
-------------------------------------------------------------------------------------*/

QGCMessageView::QGCMessageView(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::QGCMessageView)
{
    _ui->setupUi(this);
}

QGCMessageView::~QGCMessageView()
{
    delete _ui;
}

/*-------------------------------------------------------------------------------------
  QGCMessageViewWidget
-------------------------------------------------------------------------------------*/

QGCMessageViewWidget::QGCMessageViewWidget(QWidget *parent)
    : QGCMessageView(parent)
    , _unconnectedWidget(NULL)
{
    setStyleSheet("QPlainTextEdit { border: 0px }");
    // Construct initial widget
    _unconnectedWidget = new QGCUnconnectedInfoWidget(this);
    ui()->horizontalLayout->addWidget(_unconnectedWidget);
    ui()->plainTextEdit->hide();
    // Enable the right-click menu for the text editor. This works because the plainTextEdit
    // widget has its context menu policy set to its actions list. So any actions we add
    // to this widget's action list will be automatically displayed.
    // We only have the clear action right now.
    QAction* clearAction = new QAction(tr("Clear Text"), this);
    connect(clearAction, SIGNAL(triggered()), ui()->plainTextEdit, SLOT(clear()));
    ui()->plainTextEdit->addAction(clearAction);
    // Connect message handler
    connect(QGCMessageHandler::instance(), SIGNAL(textMessageReceived(QGCUasMessage*)), this, SLOT(handleTextMessage(QGCUasMessage*)));
}

QGCMessageViewWidget::~QGCMessageViewWidget()
{

}

void QGCMessageViewWidget::handleTextMessage(QGCUasMessage *message)
{
    // Reset
    if(!message) {
        ui()->plainTextEdit->clear();
        _unconnectedWidget->show();
        ui()->plainTextEdit->hide();
    } else {
        // Make sure the UI is configured for showing messages.
        // Note that this call is NOT equivalent to `_unconnectedWidget->isVisible()`.
        if (!_unconnectedWidget->isHidden())
        {
            _unconnectedWidget->hide();
            ui()->plainTextEdit->show();
        }
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

/*-------------------------------------------------------------------------------------
  QGCMessageViewRollDown
-------------------------------------------------------------------------------------*/

QGCMessageViewRollDown::QGCMessageViewRollDown(QWidget *parent, QGCToolBar *toolBar)
    : QGCMessageView(parent)
{
    _toolBar = toolBar;
    setStyleSheet("QPlainTextEdit { border: 1px }");
    QPlainTextEdit *msgWidget = ui()->plainTextEdit;
    QAction* clearAction = new QAction(tr("Clear Text"), this);
    connect(clearAction, SIGNAL(triggered()), msgWidget, SLOT(clear()));
    msgWidget->addAction(clearAction);
    // Init Messages
    QGCMessageHandler::instance()->lockAccess();
    msgWidget->setUpdatesEnabled(false);
    QVector<QGCUasMessage*> messages = QGCMessageHandler::instance()->messages();
    for(int i = 0; i < messages.count(); i++) {
        msgWidget->appendHtml(messages.at(i)->getFormatedText());
    }
    QScrollBar *scroller = msgWidget->verticalScrollBar();
    scroller->setValue(scroller->maximum());
    msgWidget->setUpdatesEnabled(true);
    connect(QGCMessageHandler::instance(), SIGNAL(textMessageReceived(QGCUasMessage*)), this, SLOT(handleTextMessage(QGCUasMessage*)));
    QGCMessageHandler::instance()->unlockAccess();
}

QGCMessageViewRollDown::~QGCMessageViewRollDown()
{

}

void QGCMessageViewRollDown::handleTextMessage(QGCUasMessage *message)
{
    // Reset
    if(message) {
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

void QGCMessageViewRollDown::leaveEvent(QEvent * event)
{
    Q_UNUSED(event);
    _toolBar->leaveMessageView();
    close();
}
