#include "QGCTabbedInfoView.h"
#include "QGCApplication.h"

QGCTabbedInfoView::QGCTabbedInfoView(const QString& title, QAction* action, QWidget *parent)
    : QGCDockWidget(title, action, parent)
{
    ui.setupUi(this);
    messageView = new UASMessageViewWidget(qgcApp()->toolbox()->uasMessageHandler(), this);
    quickView = new UASQuickView(this);
    ui.tabWidget->addTab(quickView,"Quick");
    ui.tabWidget->addTab(messageView,"Messages");
    
    loadSettings();
}
void QGCTabbedInfoView::addSource(MAVLinkDecoder *decoder)
{
    m_decoder = decoder;
    quickView->addSource(decoder);
}

QGCTabbedInfoView::~QGCTabbedInfoView()
{
}
