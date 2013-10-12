#include <QDockWidget>

#include "QGCTextLabel.h"
#include "ui_QGCTextLabel.h"

#include "MAVLinkProtocol.h"
#include "UASManager.h"

QGCTextLabel::QGCTextLabel(QWidget *parent) :
    QGCToolWidgetItem("Command Button", parent),
    ui(new Ui::QGCTextLabel)
{
    uas = 0;
    enabledNum = -1;
    ui->setupUi(this);

    connect(ui->editFinishButton, SIGNAL(clicked()), this, SLOT(endEditMode()));

    // Hide all edit items
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editLine1->hide();
}

QGCTextLabel::~QGCTextLabel()
{
    delete ui;
}

void QGCTextLabel::startEditMode()
{
    // Hide elements
    ui->nameLabel->hide();
    ui->editFinishButton->show();
    ui->editNameLabel->show();
    ui->editLine1->show();

    // Attempt to undock the dock widget
    QWidget* p = this;
    QDockWidget* dock;

    do {
        p = p->parentWidget();
        dock = dynamic_cast<QDockWidget*>(p);

        if (dock)
        {
            dock->setFloating(true);
            break;
        }
    } while (p && !dock);

    isInEditMode = true;
}

void QGCTextLabel::endEditMode()
{
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editLine1->hide();
    ui->nameLabel->show();

    // Write to settings
    emit editingFinished();

    // Attempt to dock the dock widget
    QWidget* p = this;
    QDockWidget* dock;

    do {
        p = p->parentWidget();
        dock = dynamic_cast<QDockWidget*>(p);

        if (dock)
        {
            dock->setFloating(false);
            break;
        }
    } while (p && !dock);

    isInEditMode = false;
}

void QGCTextLabel::writeSettings(QSettings& settings)
{
    qDebug() << "COMMAND BUTTON WRITING SETTINGS";
    settings.setValue("TYPE", "COMMANDBUTTON");
    settings.setValue("QGC_COMMAND_BUTTON_DESCRIPTION", ui->nameLabel->text());

    settings.sync();
}
void QGCTextLabel::readSettings(const QString& pre,const QVariantMap& settings)
{
    ui->isMavCommand->setChecked(settings.value(pre + "QGC_TEXT_SOURCE", "NONE").toString() == "MAV");
    if (!ui->isMavCommand->isChecked())
    {
        ui->editNameLabel->setText(settings.value(pre + "QGC_TEXT_TEXT","").toString());
        ui->nameLabel->setText(ui->editNameLabel->text());
    }
    else
    {
        //MAV command text
        connect(uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(textMessageReceived(int,int,int,QString)));
    }
}
void QGCTextLabel::textMessageReceived(int uasid, int component, int severity, QString message)
{
    Q_UNUSED(uasid);
    Q_UNUSED(component);
    Q_UNUSED(severity);
    if (enabledNum != -1)
    {
        //SUCCESS: Executed CMD: 241
        if (message.contains("SUCCESS"))
        {
            if (message.trimmed().endsWith(QString::number(enabledNum)))
            {
                enabledNum = -1;
                ui->textLabel->setText(ui->textLabel->text() + " Complete");
            }
        }
        else
        {
            ui->textLabel->setText(message);
        }
    }
}

void QGCTextLabel::readSettings(const QSettings& settings)
{
    ui->isMavCommand->setChecked(settings.value("QGC_TEXT_SOURCE", "NONE").toString() == "MAV");
    ui->editNameLabel->setText(settings.value("QGC_TEXT_TEXT","").toString());
    if (!ui->isMavCommand->isChecked())
    {
        ui->textLabel->setText(ui->editNameLabel->text());
        ui->nameLabel->setText("");
    }
    else
    {
        //MAV command text
        ui->nameLabel->setText(ui->editNameLabel->text());
        ui->textLabel->setText("");
        connect(uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(textMessageReceived(int,int,int,QString)));
    }
}
void QGCTextLabel::enableText(int num)
{
    enabledNum = num;

}

void QGCTextLabel::setActiveUAS(UASInterface *uas)
{
    this->uas = uas;
}
