#include "QGCParamSlider.h"
#include "ui_QGCParamSlider.h"

QGCParamSlider::QGCParamSlider(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCParamSlider)
{
    ui->setupUi(this);
}

QGCParamSlider::~QGCParamSlider()
{
    delete ui;
}

void QGCParamSlider::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
