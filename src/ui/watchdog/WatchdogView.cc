#include "WatchdogView.h"
#include "ui_WatchdogView.h"

WatchdogView::WatchdogView(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::WatchdogView)
{
    m_ui->setupUi(this);
}

WatchdogView::~WatchdogView()
{
    delete m_ui;
}

void WatchdogView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
