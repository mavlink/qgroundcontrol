#include "QGCTabbedInfoView.h"
#include <UASActionsWidget.h>
#include <UASQuickView.h>

QGCTabbedInfoView::QGCTabbedInfoView(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    ui.tabWidget->addTab(new UASQuickView(this),"Quick");
    ui.tabWidget->addTab(new UASActionsWidget(this),"Actions");
}

QGCTabbedInfoView::~QGCTabbedInfoView()
{
}
