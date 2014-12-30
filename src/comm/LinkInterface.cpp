#include "LinkInterface.h"

LinkInterface::LinkInterface(QGCSettingsGroup *pparentGroup, QString groupName) :
    QThread(0),
    QGCSettingsGroup(pparentGroup, groupName),
    _ownedByLinkManager(false),
    _deletedByLinkManager(false)
{
    // Initialize everything for the data rate calculation buffers.
    inDataIndex = 0;
    outDataIndex = 0;
    link_id = LINK_INVALID_ID;    // link identifier set at invalid

    // Initialize our data rate buffers manually, cause C++<03 is dumb.
    for (int i = 0; i < dataRateBufferSize; ++i)
    {
        inDataWriteAmounts[i] = 0;
        inDataWriteTimes[i] = 0;
        outDataWriteAmounts[i] = 0;
        outDataWriteTimes[i] = 0;
    }

    qRegisterMetaType<LinkInterface*>("LinkInterface*");
}


int LinkInterface::getId() const{
    return link_id;
}

QString LinkInterface::getGroupName(void){
    return tr("LINK_%1").arg(getId());
}
