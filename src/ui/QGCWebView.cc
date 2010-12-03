#include "QGCWebView.h"
#include "ui_QGCWebView.h"

QGCWebView::QGCWebView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCWebView)
{
    ui->setupUi(this);
    ui->webView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    ui->webView->load(QUrl("http://qgroundcontrol.org"));
}

QGCWebView::~QGCWebView()
{
    delete ui;
}

void QGCWebView::changeEvent(QEvent *e)
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
