#include "AdvParameterList.h"


AdvParameterList::AdvParameterList(QWidget *parent) :
    AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    initConnections();
}

AdvParameterList::~AdvParameterList()
{
}

void AdvParameterList::activeUASSet(UASInterface *uas)
{
    // FIXME: Can't handle uas coming and going
    if (uas) {
        QList<FactTableWidget*> tables = findChildren<FactTableWidget*>();
        for (int i=0; i<tables.count(); i++) {
            FactTableWidget* table = tables[i];
            table->setup(uas);
        }
    }
}
