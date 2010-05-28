#include "WatchdogProcessView.h"
#include "ui_WatchdogProcessView.h"

WatchdogProcessView::WatchdogProcessView(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::WatchdogProcessView)
{
    m_ui->setupUi(this);
}

WatchdogProcessView::~WatchdogProcessView()
{
    delete m_ui;
}

void WatchdogProcessView::changeEvent(QEvent *e)
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
