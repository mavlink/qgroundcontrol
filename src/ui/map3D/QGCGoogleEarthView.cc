#include "QGCGoogleEarthView.h"
#include "ui_QGCGoogleEarthView.h"

QGCGoogleEarthView::QGCGoogleEarthView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCGoogleEarthView)
{
    ui->setupUi(this);
    ui->webView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    ui->webView->load(QUrl("earth.html"));
}

QGCGoogleEarthView::~QGCGoogleEarthView()
{
    delete ui;
}

void QGCGoogleEarthView::changeEvent(QEvent *e)
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
