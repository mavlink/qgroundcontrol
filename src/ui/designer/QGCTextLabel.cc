#include <QDockWidget>

#include "QGCTextLabel.h"
#include "ui_QGCTextLabel.h"

#include "MAVLinkProtocol.h"
#include "UASManager.h"

QGCTextLabel::QGCTextLabel(QWidget *parent) :
    QGCToolWidgetItem("Text Label", parent),
    ui(new Ui::QGCTextLabel)
{
    uas = 0;
    enabledNum = -1;
    ui->setupUi(this);

    connect(ui->editFinishButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
    connect(ui->isMavCommand, SIGNAL(toggled(bool)), this, SLOT(update_isMavCommand()));

    // Hide all edit items
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editLine1->hide();
    ui->editLine2->hide();
    ui->isMavCommand->hide();
    ui->textLabel->setText(QString());

    init();
}

QGCTextLabel::~QGCTextLabel()
{
    delete ui;
}

void QGCTextLabel::setEditMode(bool editMode)
{
    if(!editMode)
        update_isMavCommand();
    ui->editFinishButton->setVisible(editMode);
    ui->editNameLabel->setVisible(editMode);
    ui->editLine1->setVisible(editMode);
    ui->editLine2->setVisible(editMode);
    ui->isMavCommand->setVisible(editMode);

    QGCToolWidgetItem::setEditMode(editMode);
}

void QGCTextLabel::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "TEXT");
    settings.setValue("QGC_TEXT_TEXT", ui->editNameLabel->text());
    settings.setValue("QGC_TEXT_SOURCE", ui->isMavCommand->isChecked()?"MAV":"NONE");

    settings.sync();
}
void QGCTextLabel::readSettings(const QString& pre,const QVariantMap& settings)
{
    ui->editNameLabel->setText(settings.value(pre + "QGC_TEXT_TEXT","").toString());
    ui->isMavCommand->setChecked(settings.value(pre + "QGC_TEXT_SOURCE", "NONE").toString() == "MAV");
    update_isMavCommand();
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
    ui->editNameLabel->setText(settings.value("QGC_TEXT_TEXT","").toString()); //Place this before setting isMavCommand
    ui->isMavCommand->setChecked(settings.value("QGC_TEXT_SOURCE", "NONE").toString() == "MAV");
    update_isMavCommand();
}

void QGCTextLabel::enableText(int num)
{
    enabledNum = num;
}

void QGCTextLabel::setActiveUAS(UASInterface *uas)
{
    if(this->uas)
        this->uas->disconnect(this);
    this->uas = uas;
    update_isMavCommand(); //Might need to update the signal connections
}

void QGCTextLabel::update_isMavCommand()
{
    ui->textLabel->setText("");
    if (!ui->isMavCommand->isChecked())
    {
        ui->nameLabel->setText(ui->editNameLabel->text());
        if(this->uas)
            disconnect(uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(textMessageReceived(int,int,int,QString)));
        if(ui->nameLabel->text().isEmpty())
            ui->nameLabel->setText(tr("Text Label")); //Show something, so that we don't end up with just an empty label
    }
    else
    {
        //MAV command text
        ui->nameLabel->setText(ui->editNameLabel->text());
        if(this->uas)
        connect(uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(textMessageReceived(int,int,int,QString)));
    }
}
