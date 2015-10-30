#include "QGCTabbedInfoView.h"
#include "QGCApplication.h"

QGCTabbedInfoView::QGCTabbedInfoView(const QString& title, QAction* action, QWidget *parent)
    : QGCDockWidget(title, action, parent)
{
    ui.setupUi(this);
    messageView = new UASMessageViewWidget(qgcApp()->toolbox()->uasMessageHandler(), this);
    //actionsWidget = new UASActionsWidget(this);
    quickView = new UASQuickView(this);
    //rawView = new UASRawStatusView(this);
    ui.tabWidget->addTab(quickView,"Quick");
    //ui.tabWidget->addTab(actionsWidget,"Actions");
    //ui.tabWidget->addTab(rawView,"Status");
    ui.tabWidget->addTab(messageView,"Messages");
    
    loadSettings();
}
void QGCTabbedInfoView::addSource(MAVLinkDecoder *decoder)
{
    m_decoder = decoder;
    //rawView->addSource(decoder);
    quickView->addSource(decoder);
}

QGCTabbedInfoView::~QGCTabbedInfoView()
{
}
