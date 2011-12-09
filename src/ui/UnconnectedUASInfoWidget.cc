#include "UnconnectedUASInfoWidget.h"
#include "ui_UnconnectedUASInfoWidget.h"

UnconnectedUASInfoWidget::UnconnectedUASInfoWidget(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::UnconnectedUASInfoWidget)
{
    ui->setupUi(this);
}

UnconnectedUASInfoWidget::~UnconnectedUASInfoWidget()
{
    delete ui;
}
