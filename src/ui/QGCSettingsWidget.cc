#include "QGCSettingsWidget.h"
#include "ui_QGCSettingsWidget.h"

QGCSettingsWidget::QGCSettingsWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QGCSettingsWidget)
{
    ui->setupUi(this);
}

QGCSettingsWidget::~QGCSettingsWidget()
{
    delete ui;
}
