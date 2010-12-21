#include "QGCUnconnectedInfoWidget.h"
#include "ui_QGCUnconnectedInfoWidget.h"

QGCUnconnectedInfoWidget::QGCUnconnectedInfoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCUnconnectedInfoWidget)
{
    ui->setupUi(this);

    connect(ui->simulationButton, SIGNAL(clicked()), this, SIGNAL(simulation()));
    connect(ui->connectButton, SIGNAL(clicked()), this, SIGNAL(addLink()));
}

QGCUnconnectedInfoWidget::~QGCUnconnectedInfoWidget()
{
    delete ui;
}
