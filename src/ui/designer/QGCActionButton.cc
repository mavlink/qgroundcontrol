#include "QGCActionButton.h"
#include "ui_QGCActionButton.h"

QGCActionButton::QGCActionButton(QWidget *parent) :
    QGCToolWidgetItem(parent),
    ui(new Ui::QGCActionButton)
{
    ui->setupUi(this);
    connect(ui->editFinishButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
    endEditMode();
}

QGCActionButton::~QGCActionButton()
{
    delete ui;
}

void QGCActionButton::startEditMode()
{
    ui->editActionComboBox->show();
    ui->editActionsRefreshButton->show();
    ui->editFinishButton->show();
    isInEditMode = true;
}

void QGCActionButton::endEditMode()
{
    ui->editActionComboBox->hide();
    ui->editActionsRefreshButton->hide();
    ui->editFinishButton->hide();
    isInEditMode = false;
}
