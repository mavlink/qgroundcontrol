#include <QMenu>
#include <QScrollBar>

#include "QGCMessageView.h"
#include "GAudioOutput.h"
#include "QGCUnconnectedInfoWidget.h"
#include "UASManager.h"
#include "ui_QGCMessageView.h"

QGCMessageView::QGCMessageView(QWidget *parent) :
    QWidget(parent),
    activeUAS(NULL),
    ui(new Ui::QGCMessageView)
{
    setObjectName("QUICKVIEW_MESSAGE_CONSOLE")  ;

    ui->setupUi(this);
    setStyleSheet("QPlainTextEdit { border: 0px }");

    // Construct initial widget
    connectWidget = new QGCUnconnectedInfoWidget(this);
    ui->horizontalLayout->addWidget(connectWidget);
    ui->plainTextEdit->hide();

    // Enable the right-click menu for the text editor. This works because the plainTextEdit
    // widget has its context menu policy set to its actions list. So any actions we add
    // to this widget's action list will be automatically displayed.
    // We only have the clear action right now.
    QAction* clearAction = new QAction(tr("Clear Text"), this);
    connect(clearAction, SIGNAL(triggered()), ui->plainTextEdit, SLOT(clear()));
    ui->plainTextEdit->addAction(clearAction);

    // Connect to the currently active UAS.
    setActiveUAS(qgcApp()->singletonUASManager()->getActiveUAS());
    connect(qgcApp()->singletonUASManager(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
}

QGCMessageView::~QGCMessageView()
{
    // The only thing we need to delete is the ui because it's the only thing not cleaned up automatically
    // by the deletion of its parent.
    delete ui;
}

void QGCMessageView::setActiveUAS(UASInterface* uas)
{
    // If we were already attached to an autopilot, disconnect it, restoring
    // the widget to its initial state as needed.
    if (activeUAS)
    {
        disconnect(activeUAS, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
        ui->plainTextEdit->clear();
        activeUAS = NULL;
    }

    // And now if there's an autopilot to follow, set up the UI.
    if (uas)
    {
        // Make sure the UI is configured for showing messages.
        // Note that this call is NOT equivalent to `connectWidget->isVisible()`.
        if (!connectWidget->isHidden())
        {
            connectWidget->hide();
            ui->plainTextEdit->show();
        }

        // And connect to the new UAS.
        connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
        activeUAS = uas;
    }
    // But if there's no new autopilot, restore the connect button.
    else
    {
        connectWidget->show();
        ui->plainTextEdit->hide();
    }
}

void QGCMessageView::handleTextMessage(int uasid, int compId, int severity, QString text)
{
    QPlainTextEdit *msgWidget = ui->plainTextEdit;
    (void)uasid; // Unused variable voided.
    // Turn off updates while we're appending content to avoid breaking the autoscroll behavior
    msgWidget->setUpdatesEnabled(false);
    QScrollBar *scroller = msgWidget->verticalScrollBar();

    // Color the output depending on the message severity. We have 3 distinct cases:
    // 1: If we have an ERROR or worse, make it bigger, bolder, and highlight it red.
    // 2: If we have a warning or notice, just make it bold and color it orange.
    // 3: Otherwise color it the standard color, white.

    // So first deteremine the styling based on the severity.
    QString style;
    switch (severity)
    {
    case MAV_SEVERITY_EMERGENCY:
    case MAV_SEVERITY_ALERT:
    case MAV_SEVERITY_CRITICAL:
    case MAV_SEVERITY_ERROR:
        //Use set RGB values from given color from QGC
        style = QString("color: rgb(%1, %2, %3); font-weight:bold").arg(QGC::colorRed.red()).arg(QGC::colorRed.green()).arg(QGC::colorRed.blue());
        break;
    case MAV_SEVERITY_NOTICE:
    case MAV_SEVERITY_WARNING:
        style = QString("color: rgb(%1, %2, %3); font-weight:bold").arg(QGC::colorOrange.red()).arg(QGC::colorOrange.green()).arg(QGC::colorOrange.blue());
        break;
    default:
        style = QString("color:white; font-weight:bold");
        break;
    }

    // And determine the text for the severitie
    QString severityText("");
    switch (severity)
    {
    case MAV_SEVERITY_EMERGENCY:
        severityText = QString(tr(" EMERGENCY:"));
        break;
    case MAV_SEVERITY_ALERT:
        severityText = QString(tr(" ALERT:"));
        break;
    case MAV_SEVERITY_CRITICAL:
        severityText = QString(tr(" Critical:"));
        break;
    case MAV_SEVERITY_ERROR:
        severityText = QString(tr(" Error:"));
        break;
    case MAV_SEVERITY_WARNING:
        severityText = QString(tr(" Warning:"));
        break;
    case MAV_SEVERITY_NOTICE:
        severityText = QString(tr(" Notice:"));
        break;
    case MAV_SEVERITY_INFO:
        severityText = QString(tr(" Info:"));
        break;
    case MAV_SEVERITY_DEBUG:
        severityText = QString(tr(" Debug:"));
        break;
    default:
        severityText = QString(tr(""));
        break;
    }

    // Finally append the properly-styled text with a timestamp.
    QString dateString = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    msgWidget->appendHtml(QString("<p style=\"color:#CCCCCC\">[%2 - COMP:%3]<font style=\"%1\">%4 %5</font></p>").arg(style).arg(dateString).arg(compId).arg(severityText).arg(text));

    // Ensure text area scrolls correctly
    scroller->setValue(scroller->maximum());
    msgWidget->setUpdatesEnabled(true);
}
