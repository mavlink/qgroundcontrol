#include "WatchdogProcessView.h"
#include "ui_WatchdogProcessView.h"

WatchdogProcessView::WatchdogProcessView(int processid, QWidget *parent) :
    QWidget(parent),
    processid(processid),
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
