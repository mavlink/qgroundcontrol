#include "QGCWelcomeWidget.h"
#include "ui_QGCWelcomeWidget.h"

QGCWelcomeWidget::QGCWelcomeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCWelcomeWidget)
{
    ui->setupUi(this);
}

QGCWelcomeWidget::~QGCWelcomeWidget()
{
    delete ui;
}
