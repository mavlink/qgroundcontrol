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
    // widget has its context menu policy set to its actions list. So we any actions we add
    // to this widget's action list will be automatically displayed.
    // We only have the clear action right now.
    QAction* clearAction = new QAction(tr("Clear Text"), this);
    connect(clearAction, SIGNAL(triggered()), ui->plainTextEdit, SLOT(clear()));
    ui->plainTextEdit->addAction(clearAction);

    // Connect to the currently active UAS.
    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
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

    // Turn off updates while we're appending content to avoid breaking the autoscroll behavior
    msgWidget->setUpdatesEnabled(false);
    QScrollBar *scroller = msgWidget->verticalScrollBar();

    // Get all the UAS info.
    UASInterface *uas = UASManager::instance()->getUASForId(uasid);
    QString uasName(uas->getUASName());
    QString colorName(uas->getColor().name());

    // Color the output depending on the message severity. We have 3 distinct cases:
    // 1: If we have an ERROR or worse, make it bigger, bolder, and highlight it.
    // 2: If we have a warning or notice, just make it bold.
    // 3: Otherwise color it the standard color.

    // So first deteremine the styling based on the severity.
    QString style;
    switch (severity)
    {
    case MAV_SEVERITY_EMERGENCY:
    case MAV_SEVERITY_ALERT:
    case MAV_SEVERITY_CRITICAL:
    case MAV_SEVERITY_ERROR:
        // TODO: Move this audio output to UAS.cc, as it doesn't make sense to put audio output in a message logger widget.
        GAudioOutput::instance()->say(text.toLower());
        style = QString("color:#DC143C;font-size:large;font-weight:bold");
        break;
    case MAV_SEVERITY_WARNING:
    case MAV_SEVERITY_NOTICE:
        style = QString("color:%1;font-weight:bold").arg(colorName);
        break;
    default:
        style = QString("color:%1;").arg(colorName);
        break;
    }

    // And determine the text for the severities.
    QString severityText("");
    switch (severity)
    {
    case MAV_SEVERITY_EMERGENCY:
        severityText = QString(tr("EMERGENCY"));
        break;
    case MAV_SEVERITY_ALERT:
        severityText = QString(tr("ALERT"));
        break;
    case MAV_SEVERITY_CRITICAL:
        severityText = QString(tr("Critical"));
        break;
    case MAV_SEVERITY_ERROR:
        severityText = QString(tr("Error"));
        break;
    case MAV_SEVERITY_WARNING:
        severityText = QString(tr("Warning"));
        break;
    case MAV_SEVERITY_NOTICE:
        severityText = QString(tr("Notice"));
        break;
    case MAV_SEVERITY_INFO:
        severityText = QString(tr("Info"));
        break;
    case MAV_SEVERITY_DEBUG:
        severityText = QString(tr("Debug"));
        break;
    default:
        severityText = QString(tr("Unknown"));
        break;
    }

    // Finally append the properly-styled text with a timestamp.
    QString dateString = QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate);
    msgWidget->appendHtml(QString("<p style=\"%1\">[%2][%3:%4] %5 - %6</p>").arg(style).arg(dateString).arg(uasName).arg(compId).arg(severityText).arg(text));
    qDebug() << msgWidget->document()->toHtml();

    // Ensure text area scrolls correctly
    scroller->setValue(scroller->maximum());
    msgWidget->setUpdatesEnabled(true);
}